#pragma once

#include "Engine/Components/DrawComponent.h"
#include "Engine/Box.h"
#include "Engine/Vec2.h"

#include "Task.h"

#include <SFML/Graphics.hpp>

class Texture;
class PaletteSet;
class SpriteSheet;
class Anim;

// Sprite Component
class SpriteComponent : public DrawComponent
{
public:
	virtual void Update() override;
	virtual void Destroy() override;
	virtual void Draw() override;

	// Texture
	void SetTexture(std::shared_ptr<Texture> in_texture);
	void SetSubTexture(const Box2i& in_subTex);
	std::string GetPalette() { return m_palette; }
	sf::Color GetColor();
	void SetColor(uint8_t in_red, uint8_t in_green, uint8_t in_blue, uint8_t in_alpha = 255);
	void SetPalette(const std::string& in_palette);
	void SetPaletteSet(std::shared_ptr<PaletteSet> in_paletteSet);
	void SetOrigin(const Vec2f& in_origin);
	void SetFlipHori(bool in_flipH);
	bool GetFlipHori() const { return m_flipH; }
	void SetFlipVert(bool in_flipV);
	bool GetFlipVert() const { return m_flipV; }

	// Playback
	void SetPlayRate(float in_playRate) { m_playRate = in_playRate; }
	float GetPlayRate() const { return m_playRate; }
	TaskHandle<> PlayAnim(const std::string& in_animName, bool in_loop, int32_t in_startFrame = 0);
	void Stop();
	void Clear();

protected:
	void UpdatePaletteIdx();

	sf::Sprite m_sprite;
	std::shared_ptr<Texture> m_texture;
	std::shared_ptr<PaletteSet> m_paletteSet;
	std::string m_palette;
	int32_t m_paletteIdx = 0;
	float m_playRate = 1.0f;
	bool m_flipH = false;
	bool m_flipV = false;

	Task<> m_animTask;
	Task<> PlayAnimTask(std::shared_ptr<Anim> in_anim, bool in_loop, int32_t in_startFrame);
};
