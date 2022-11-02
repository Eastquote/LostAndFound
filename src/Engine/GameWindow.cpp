#include "GameWindow.h"

#include <SFML/Graphics.hpp>
#include "InputSystem.h"
#include "RenderTexture.h"
#include "Game.h"
#include "LayerManager.h"
#include "Engine/Editor/ImguiIntegration.h"

GameWindow::GameWindow(const Vec2i& in_windowSize, const Vec2i& in_renderSize, const std::wstring& in_title)
{
	m_window = std::make_shared<sf::RenderWindow>(sf::VideoMode(in_windowSize.x, in_windowSize.y, 32), sf::String(in_title));
	m_inputSys = std::make_shared<InputSystem>();
	m_layerManager = std::make_shared<LayerManager>();

	SetRenderSize(in_renderSize);
	RefreshWindowView(in_windowSize);
	SetWorldView(Box2f::FromCenter({ 0.0f, 0.0f }, Vec2f(in_renderSize)));

	m_imgui = std::make_shared<ImguiIntegration>(*this);
}
GameWindow::~GameWindow()
{
}
void GameWindow::SetTitle(const std::wstring& in_title)
{
	m_window->setTitle(in_title);
}
void GameWindow::Update()
{
	sf::Event e;
	while(m_window->pollEvent(e))
	{
		switch(e.type)
		{
		case sf::Event::Closed:
		{
			m_window->close();
			break;
		}
		case sf::Event::Resized:
		{
			RefreshWindowView({ (float)e.size.width, (float)e.size.height });
			break;
		}
		default:
		{
			// Forward input events on to the input system and imgui
			m_inputSys->HandleEvent(&e);
			m_imgui->HandleEvent(e);
			break;
		}
		}
	}

	// Update input system and imgui
	m_inputSys->Update();
	m_imgui->Update(*this);
}
void GameWindow::Clear()
{
	m_renderTexture->Clear(m_viewClearColor);
	m_postProcessTexture->Clear(m_viewClearColor);
	m_window->clear(m_windowClearColor);
	m_layerManager->Clear();
}
bool GameWindow::IsOpen() const
{
	return m_window->isOpen();
}

void GameWindow::ApplyPostProcessing()
{
	// just draw the render texture into the post-process texture, using the specified shader
	if (m_postProcessShader)
	{
		m_layerManager->BindShaderUniforms(m_postProcessShader);
		m_renderTexture->Draw(Transform::Identity, m_postProcessTexture, m_postProcessShader);
	}
}

void GameWindow::Swap()
{
	// calculate transform to draw the game texture scaled up to fit the window
	const Vec2f pixelAspectScale = m_pixelAspectRatio > 1.0f ? Vec2f{ m_pixelAspectRatio, 1.0f } : Vec2f{ 1.0f, 1.0f / m_pixelAspectRatio };

	// post-processing is enabled, m_postProcessTexture will be the final non-scaled view. if it isn't, use m_renderTexture directly
	std::shared_ptr<RenderTexture> postPostProcessTexture = m_postProcessShader ? m_postProcessTexture : m_renderTexture;

	// draw game render tex to window
	if (m_scalingMode == eWindowScalingMode::PixelSmooth)
	{
		// this mode splits a ConserveAspect view transform into two parts:
		// first does an intermediate pixel-perfect draw as close as possible to the final size, then smoothly scales that to the final size

		const Transform baseGameTexTM = GetWindowToRenderTextureTransform();
		const Vec2f texScale = baseGameTexTM.scale;

		// scale+size of the intermediate texture
		const Vec2f intermediateScale = Math::Max(Vec2f::One, Math::Round(texScale * pixelAspectScale));
		const Vec2f intermediateSize = Vec2f(postPostProcessTexture->GetSize()) * intermediateScale;

		// allocate/resize intermediate texture as necessary
		if (!m_pixelConserveAspectRenderTexture || m_pixelConserveAspectRenderTexture->GetSize() != intermediateSize)
		{
			m_pixelConserveAspectRenderTexture = std::make_shared<RenderTexture>(intermediateSize);
		}
		m_pixelConserveAspectRenderTexture->Clear(m_viewClearColor);

		// set texture filtering
		postPostProcessTexture->SetSmooth(false);
		m_pixelConserveAspectRenderTexture->SetSmooth(true);

		// draw render texture at integer scale into our intermediate texture
		const Transform gameTexTM{ Vec2f::Zero, 0.0f, intermediateScale };
		postPostProcessTexture->Draw(gameTexTM, m_pixelConserveAspectRenderTexture);

		// draw intermediate texture onto window, scaled and texPositioned the rest of the way
		const Transform finalTexTM{ baseGameTexTM.pos, 0.0f, texScale / intermediateScale };
		m_pixelConserveAspectRenderTexture->Draw(finalTexTM);
	}
	else
	{
		postPostProcessTexture->Draw(GetWindowToRenderTextureTransform());
	}

	m_imgui->Draw(*this);

	// draw the view texture to the window, then display the window
	m_window->display();
}

Transform GameWindow::GetWindowToRenderTextureTransform() const
{
	// calculate transform to draw the game texture scaled up to fit the window
	const Vec2f pixelAspectScale = m_pixelAspectRatio > 1.0f ? Vec2f{ m_pixelAspectRatio, 1.0f } : Vec2f{ 1.0f, 1.0f / m_pixelAspectRatio };
	const Vec2f cameraSize = Vec2f(m_renderTexture->GetSize());
	const Vec2f viewSize = cameraSize * pixelAspectScale;
	const Vec2f windowSize = Vec2f(GetWindowSize());

	Vec2f texPosition = Vec2f::Zero;
	Vec2f texScale = Vec2f::One;

	if (m_scalingMode == eWindowScalingMode::FillWindow)
	{
		texScale = windowSize / cameraSize;
	}
	else
	{
		float unifScale = 1.0f;

		const float windowAspect = windowSize.x / windowSize.y;
		const float viewAspect = viewSize.x / viewSize.y;

		// if the viewport is wider than our expected aspect ratio, black bars will be added on the sides, so the camera width will be based on the viewport height
		if (windowAspect > viewAspect)
		{
			unifScale = windowSize.y / viewSize.y;
		}
		else
		{
			unifScale = windowSize.x / viewSize.x;
		}

		if ((m_scalingMode == eWindowScalingMode::PixelPerfect) && unifScale > 1.0f)
		{
			unifScale = Math::Floor(unifScale);
		}

		texScale = { unifScale, unifScale };

		texPosition = (windowSize - viewSize * texScale) * 0.5f;
	}

	return Transform{ texPosition, 0.0f, texScale * pixelAspectScale };
}

void GameWindow::SetWorldView(const Box2f& in_view, float in_angle)
{
	m_renderTexture->SetView(in_view, in_angle);
	m_layerManager->SetWorldView(in_view, in_angle);
}
Box2f GameWindow::GetWorldView()
{
	return m_renderTexture->GetView();
}

void GameWindow::SetSmoothFiltering(bool in_smooth)
{
	m_renderTexture->SetSmooth(in_smooth);
}

bool GameWindow::GetSmoothFiltering() const
{
	return m_renderTexture->GetSmooth();
}

void GameWindow::SetScalingMode(eWindowScalingMode in_mode)
{
	if (in_mode != m_scalingMode)
	{
		if (in_mode != eWindowScalingMode::PixelSmooth)
		{
			m_pixelConserveAspectRenderTexture = {};
		}
	}

	m_scalingMode = in_mode;
}

void GameWindow::RefreshWindowView(Vec2f in_windowSize)
{
	// set the window to use pixel coordinates, with (0, 0) in the bottom left
	sf::View view(sf::FloatRect(0.0f, in_windowSize.y, in_windowSize.x, -in_windowSize.y));
	m_window->setView(view);
}

Vec2u GameWindow::GetWindowSize() const
{
	auto windowSize = m_window->getSize();
	return { windowSize.x, windowSize.y };
}

Vec2u GameWindow::GetRenderSize() const
{
	return m_renderTexture->GetSize();
}

void GameWindow::SetRenderSize(Vec2u in_size)
{
	Box2f prevView{};
	bool bHadPrevView = false;
	if (m_renderTexture)
	{
		bHadPrevView = true;
		prevView = m_renderTexture->GetView();
	}

	m_renderTexture = std::make_shared<RenderTexture>(in_size);
	m_renderTexture->SetView(prevView);

	m_postProcessTexture = std::make_shared<RenderTexture>(in_size);
	
	m_layerManager->SetRenderSize(in_size);
}

Vec2f GameWindow::WindowToWorld(const Vec2f& in_windowPos) const
{
	const Transform gameTexTM = GetWindowToRenderTextureTransform();
	const Vec2f texPos = gameTexTM.InvTransformPoint(in_windowPos);
	
	return m_renderTexture->ViewToWorld(texPos);
}
Vec2f GameWindow::WorldToWindow(const Vec2f& in_worldPos) const
{
	Vec2f texPos = m_renderTexture->WorldToView( in_worldPos );
	
	const Transform gameTexTM = GetWindowToRenderTextureTransform();
	return gameTexTM.TransformPoint(texPos);
}

std::shared_ptr<RenderTexture> GameWindow::GetRenderTarget() const
{
	return m_renderTexture;
}

std::shared_ptr<RenderTexture> GameWindow::GetPostProcessRenderTarget() const
{
	return m_postProcessShader ? m_postProcessTexture : m_renderTexture;
}

sf::RenderWindow* GameWindow::GetSFMLWindow() const
{
	return m_window.get();
}
InputSystem* GameWindow::GetInputSystem() const
{
	return m_inputSys.get();
}

//--- Helper Functions ---//
std::shared_ptr<GameWindow> GetWindow()
{
	return GameBase::Get()->GetWindow();
}
Vec2u GetWindowSize()
{
	auto window = GetWindow();
	return window->GetWindowSize();
}

Vec2u GetRenderSize()
{
	auto window = GetWindow();
	return window->GetRenderSize();
}

Vec2f WindowToWorld(const Vec2i& in_windowPos)
{
	auto window = GetWindow();
	return window->WindowToWorld(in_windowPos);
}
Vec2i WorldToWindow(const Vec2f& in_worldPos)
{
	auto window = GetWindow();
	return window->WorldToWindow(in_worldPos);
}
