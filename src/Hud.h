#pragma once

#include "GameActor.h"

// Full-screen black quad that can be used to fade the scene in/out.
class FadeQuad : public GameActor {
public:
	FadeQuad();
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	std::shared_ptr<SpriteComponent> GetSprite() { return m_sprite; }

private:
	std::shared_ptr<SpriteComponent> m_sprite;
};

// Lays out and updates in-game HUD elements
class Hud : public GameActor {
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual Task<> ManageActor() override;

	// Slides upper and/or lower HUD elements on- and off-screen
	TaskHandle<> InterpHudVisiblity(bool in_show, float in_duration = 0.0f, bool in_topZone = true, bool in_bottomZone = true);

	// Slides upper and/or lower letterbox bars on- or off-screen
	Task<> InterpLetterboxVisibility(bool in_show = true, bool in_topBar = true, bool in_bottomBar = true, float in_duration = 0.0f);

	// Slides upper and/or lower letterbox bars OFF-screen
	Task<> FadeScreenToSolid(float in_duration = 2.0f, std::optional<sf::Color> in_color = {});

	// Fades full screen "in" from solid color (defaults to black)
	Task<> FadeScreenFromSolid(float in_duration = 2.0f, std::optional<sf::Color> in_color = {});

	void SetSkyboxWorldTransform(const Transform& in_transform);
	
private:
	Task<> HudTransitionManager(bool in_show, float in_duration, bool in_topZone, bool in_bottomZone);

	// Smoothly interps pos to targetPos over in_duration period using EaseInOutSmoothest function
	Task<> EasePos(Vec2i& in_currentPos, const Vec2i& in_targetPos, float in_duration);

	Task<> AnimateSpritePos(std::shared_ptr<SpriteComponent> in_sprite, Vec2f in_targetPos, float in_duration);
	
	TaskHandle<> m_transitionHudTask;
	Vec2i m_topRowAnchor = Vec2f::Zero;
	Vec2i m_bottomRowAnchor = Vec2f::Zero;

	// Textcomps
	std::shared_ptr<TextComponent> m_hudTextComponent;
	std::shared_ptr<TextComponent> m_missileCountTextComp;
	std::shared_ptr<TextComponent> m_grenadeCountTextComp;
	std::shared_ptr<TextComponent> m_zoneName;

	// Sprites
	std::shared_ptr<SpriteComponent> m_zoneNameBkg;
	std::shared_ptr<SpriteComponent> m_hudBkgComponent;
	std::shared_ptr<SpriteComponent> m_hudLabelComponent;
	std::shared_ptr<SpriteComponent> m_missileSpriteComponent;
	std::shared_ptr<SpriteComponent> m_ltButtonComponent;
	std::shared_ptr<SpriteComponent> m_rtButtonComponent;
	std::shared_ptr<SpriteComponent> m_xButtonComponent;
	std::shared_ptr<SpriteComponent> m_yButtonComponent;
	std::shared_ptr<SpriteComponent> m_letterboxTop;
	std::shared_ptr<SpriteComponent> m_letterboxBottom;
	std::shared_ptr<FadeQuad> m_fadeQuad;
	std::shared_ptr<SpriteComponent> m_skyboxQuad;

	// Sprite lists
	std::vector<std::shared_ptr<SpriteComponent>> m_hudEnergyTankSprites;
	std::vector<std::shared_ptr<SpriteComponent>> m_healthNumberSprites;
	std::vector<std::shared_ptr<SpriteComponent>> m_missileNumberSprites;
	std::vector<std::shared_ptr<SpriteComponent>> m_primaryWeaponSprites;
	std::vector<std::shared_ptr<SpriteComponent>> m_secondaryWeaponSprites;
};
