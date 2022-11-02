#pragma once

#include <map>
#include <string>
#include <memory>
#include <vector>

#include "Vec2.h"
#include "Box.h"

class Texture;
class PaletteSet;

class Anim
{
public:
	Anim(std::shared_ptr<Texture> in_texture, std::shared_ptr<PaletteSet> in_paletteSet, const std::vector<Box2i>& in_sprites, float in_frameRate, const Vec2i& in_origin = Vec2i::Zero);

	std::shared_ptr<Texture> GetTexture() const;
	std::shared_ptr<PaletteSet> GetPaletteSet() const;
	const Vec2i& GetOrigin() const;
	const Box2i& GetFrame(int32_t in_frameIdx) const;
	size_t GetNumFrames() const;
	float GetFrameRate() const;

private:
	std::shared_ptr<Texture> m_texture;
	std::shared_ptr<PaletteSet> m_paletteSet;
	std::vector<Box2i> m_sprites;
	float m_frameRate = 60.0f;
	Vec2i m_origin = Vec2i::Zero;
};
