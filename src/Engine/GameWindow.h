#pragma once

#include <memory>
#include <string>

#include <SFML/Graphics/Color.hpp>

// Forward declarations
namespace sf
{
	class RenderWindow;
	class RenderTarget;
}
class InputSystem;
class RenderTexture;
class Shader;
class ImguiIntegration;
class LayerManager;

#include "Vec2.h"
#include "Box.h"

enum class eWindowScalingMode
{
	PixelPerfect,
	ConserveAspect,
	PixelSmooth, // does pixel-perfect scaling as close as possible, then smooth scaling the rest of the way
	FillWindow,
};

// Game Window
class GameWindow
{
public:
	GameWindow(const Vec2i& in_windowSize, const Vec2i& in_renderSize, const std::wstring& in_title);
	~GameWindow();

	void Update();
	void Clear();
	void ApplyPostProcessing();
	void Swap();

	bool IsOpen() const;
	void SetTitle(const std::wstring& in_title);

	void SetWorldView(const Box2f& in_view, float in_angle = 0.0f);
	Box2f GetWorldView();
	void SetSmoothFiltering(bool in_smooth);
	bool GetSmoothFiltering() const;
	void SetScalingMode(eWindowScalingMode in_mode);
	eWindowScalingMode GetScalingMode() const { return m_scalingMode; }
	void SetPixelAspectRatio(float in_ratio) { m_pixelAspectRatio = in_ratio; }
	float GetPixelAspectRatio() const { return m_pixelAspectRatio; }

	void SetPostProcessShader(std::shared_ptr<Shader> in_shader) { m_postProcessShader = in_shader; }

	void SetWindowClearColor(sf::Color in_color) { m_windowClearColor = in_color; }
	sf::Color GetWindowClearColor() const { return m_windowClearColor; }                                                     
	void SetViewClearColor(sf::Color in_color) { m_viewClearColor = in_color; }
	sf::Color GetViewClearColor() const { return m_viewClearColor; }

	Vec2u GetWindowSize() const;
	Vec2u GetRenderSize() const;
	void SetRenderSize(Vec2u in_size);
	Vec2f WindowToWorld(const Vec2f& in_windowPos) const;
	Vec2f WorldToWindow(const Vec2f& in_worldPos) const;

	std::shared_ptr<LayerManager> GetLayerManager() const { return m_layerManager; }
	std::shared_ptr<RenderTexture> GetRenderTarget() const;
	std::shared_ptr<RenderTexture> GetPostProcessRenderTarget() const;
	sf::RenderWindow* GetSFMLWindow() const;
	InputSystem* GetInputSystem() const;

private:
	Transform GetWindowToRenderTextureTransform() const;
	void RefreshWindowView(Vec2f in_windowSize);

	float m_pixelAspectRatio = 1.0f;
	eWindowScalingMode m_scalingMode = eWindowScalingMode::PixelSmooth;
	sf::Color m_windowClearColor = sf::Color(32, 32, 32);
	sf::Color m_viewClearColor = sf::Color::Black;

	std::shared_ptr<sf::RenderWindow> m_window;
	std::shared_ptr<InputSystem> m_inputSys;

	std::shared_ptr<LayerManager> m_layerManager;
	std::shared_ptr<Shader> m_postProcessShader;

	// DrawComponents render to m_renderTexture by default
	std::shared_ptr<RenderTexture> m_renderTexture;
	// if post-processing shader is set, m_renderTexture is drawn into m_postProcessTexture using it. otherwise, it's just copied
	std::shared_ptr<RenderTexture> m_postProcessTexture;
	// if using PixelSmooth scaling mode, m_postProcessTexture is draw into m_pixelConserveAspectRenderTexture with pixel-perfect scaling as close to the final window size as possible, then m_pixelConserveAspectRenderTexture is bilinear scaled to the final window size
	// otherwise, m_postProcessTexture is rendered directly to the final window with scaling based on the selected scaling mode
	std::shared_ptr<RenderTexture> m_pixelConserveAspectRenderTexture;

	std::shared_ptr<ImguiIntegration> m_imgui;
};

//--- Helper Functions ---//
std::shared_ptr<GameWindow> GetWindow();
Vec2u GetWindowSize();
Vec2u GetRenderSize();
Vec2f WindowToWorld(const Vec2i& in_windowPos);
Vec2i WorldToWindow(const Vec2f& in_worldPos);
