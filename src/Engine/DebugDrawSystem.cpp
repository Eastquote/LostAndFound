#include "DebugDrawSystem.h"

#include "Engine/AssetCache.h"
#include "Engine/Font.h"

std::shared_ptr<Font> DebugDrawSystem::DDrawText::m_font = {};

static DebugDrawSystem* s_debugDrawSystem = {};

DebugDrawSystem* DebugDrawSystem::Get()
{
	return s_debugDrawSystem;
}

DebugDrawSystem::DebugDrawSystem()
{
	s_debugDrawSystem = this;
}

DebugDrawSystem::~DebugDrawSystem()
{
	s_debugDrawSystem = {};
}


void DrawDebugLine(Vec2f in_p0, Vec2f in_p1, sf::Color in_color /*= sf::Color::White*/, Transform in_TM /*= {}*/, float in_lifetime /*= -1.f*/)
{
	DebugDrawSystem::Get()->DrawLine(in_p0, in_p1, in_color, in_TM, in_lifetime);
}

void DrawDebugCircle(Vec2f in_p0, float in_r, sf::Color in_color /*= sf::Color::White*/, Transform in_TM/*={}*/, float in_lifetime /*= -1.f*/)
{
	const int numVerts = Math::Clamp<int>((int)(in_r / 5.0f), 12, 24);
	DebugDrawSystem::Get()->DrawCircle(in_p0, in_r, in_color, in_TM, in_lifetime);
}

void DrawDebugPoint(Vec2f in_p0, sf::Color in_color /*= sf::Color::White*/, Transform in_TM/*={}*/, float in_lifetime /*= -1.f*/)
{
	DebugDrawSystem::Get()->DrawPoint(in_p0, in_color, in_TM, in_lifetime);
}

void DrawDebugBox(Box2f in_box, sf::Color in_color /*= sf::Color::White*/, Transform in_TM/*={}*/, float in_lifetime /*= -1.f*/)
{
	DebugDrawSystem::Get()->DrawBox(in_box, in_color, in_TM, in_lifetime);
}

void DrawDebugPolygon(const Polygon& in_poly, sf::Color in_color /*= sf::Color::White*/, Transform in_TM/*={}*/, float in_lifetime /*= -1.f*/)
{
	DebugDrawSystem::Get()->DrawPolygon(in_poly, in_color, in_TM, in_lifetime);
}

void DrawDebugString(Vec2f in_p0, const std::string& in_str, sf::Color in_color /*= sf::Color::White*/, Transform in_TM/*={}*/, float in_lifetime /*= -1.f*/)
{
	DebugDrawSystem::Get()->DrawString(in_p0, in_str, in_color, in_TM, in_lifetime);
}


DebugDrawSystem::DDrawText::DDrawText(Vec2f in_p0, const std::string& in_str, sf::Color in_color, Transform in_TM, float in_lifetime)
{
	m_lifetime = in_lifetime;

	if (!m_font)
	{
		InitializeFont();
	}

	if (m_font)
	{
		m_text.setFont(m_font->GetSFMLFont());
	}
	m_text.setString(in_str);
	m_text.setFillColor(in_color);

	m_text.setOutlineColor(sf::Color::Black);
	m_text.setOutlineThickness(0);

	m_text.setCharacterSize(16);

	// Set origin based on alignment, and remove any padding in the top left (sfml likes to add ~10px at the top)
	const auto bounds = m_text.getLocalBounds();
	const Vec2f alignment = Vec2f::Zero;
	m_text.setOrigin(bounds.width * alignment.x + bounds.left, bounds.height * alignment.y + bounds.top);

	// set transform
	const Vec2f worldPos = in_TM.TransformPoint(in_p0);
	m_text.setPosition(worldPos.x, worldPos.y);
	m_text.setRotation(in_TM.rot);
	m_text.setScale(in_TM.scale.x, -in_TM.scale.y);
}

void DebugDrawSystem::DDrawText::InitializeFont()
{
	// load+set debug text font
	if (auto font = AssetCache<Font>::Get()->LoadAsset(std::string() + "data/fonts/m5x7.ttf"))
	{
		DDrawText::m_font = font;

		DDrawText::m_font->SetSmooth(false);
		const sf::Texture& texture = DDrawText::m_font->GetSFMLFont().getTexture(16);
		const_cast<sf::Texture*>(&texture)->setSmooth(false);
	}
}
