#include "Anim.h"

//--- Anim ---//
Anim::Anim(std::shared_ptr<Texture> in_texture, std::shared_ptr<PaletteSet> in_paletteSet, const std::vector<Box2i>& in_sprites, float in_frameRate, const Vec2i& in_origin)
	: m_texture(in_texture)
	, m_paletteSet(in_paletteSet)
	, m_sprites(in_sprites)
	, m_frameRate(in_frameRate)
	, m_origin(in_origin)
{
}
std::shared_ptr<Texture> Anim::GetTexture() const
{
	return m_texture;
}
std::shared_ptr<PaletteSet> Anim::GetPaletteSet() const
{
	return m_paletteSet;
}
const Vec2i& Anim::GetOrigin() const
{
	return m_origin;
}
const Box2i& Anim::GetFrame(int32_t in_frameIdx) const
{
	return m_sprites[in_frameIdx];
}
size_t Anim::GetNumFrames() const
{
	return m_sprites.size();
}
float Anim::GetFrameRate() const
{
	return m_frameRate;
}
