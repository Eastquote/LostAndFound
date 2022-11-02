#include "SpriteComponent.h"

#include "Engine/Game.h"
#include "Engine/GameWindow.h"
#include "Engine/AssetCache.h"
#include "Engine/Texture.h"
#include "Engine/SpriteSheet.h"
#include "Engine/PaletteSet.h"
#include "Engine/Shader.h"
#include "Engine/Anim.h"
#include "Engine/StringUtils.h"
#include <cmath>

//--- SpriteComponent ---//
void SpriteComponent::Destroy()
{
	m_texture = {};
	m_animTask = {};

	DrawComponent::Destroy();
}
void SpriteComponent::Update()
{
	DrawComponent::Update();

	// Resume anim task
	if(m_animTask && m_animTask.Resume() == eTaskStatus::Done)
	{
		m_animTask = {};
	}
}
void SpriteComponent::SetColor(uint8_t in_red, uint8_t in_green, uint8_t in_blue, uint8_t in_alpha) {
	auto color = sf::Color(in_red, in_green, in_blue, in_alpha);
	m_sprite.setColor(color);
}
sf::Color SpriteComponent::GetColor() {
	return m_sprite.getColor();
}
void SpriteComponent::SetTexture(std::shared_ptr<Texture> in_texture)
{
	m_texture = in_texture;
	m_sprite.setTexture(m_texture->GetSFMLTexture());
}
void SpriteComponent::SetSubTexture(const Box2i& in_subTex)
{
	m_sprite.setTextureRect({ in_subTex.x, in_subTex.y, in_subTex.w, in_subTex.h });
}
void SpriteComponent::SetPalette(const std::string& in_paletteName)
{
	m_palette = in_paletteName;
	UpdatePaletteIdx();
}
void SpriteComponent::SetPaletteSet(std::shared_ptr<PaletteSet> in_paletteSet)
{
	m_paletteSet = in_paletteSet;
	UpdatePaletteIdx();
}
void SpriteComponent::UpdatePaletteIdx()
{
	m_paletteIdx = m_paletteSet ? m_paletteSet->GetPaletteIdx(m_palette) : 0;
}
void SpriteComponent::SetOrigin(const Vec2f& in_origin)
{
	m_sprite.setOrigin(in_origin.x, in_origin.y);
}
void SpriteComponent::SetFlipHori(bool in_flipH)
{
	m_flipH = in_flipH;
}
void SpriteComponent::SetFlipVert(bool in_flipV)
{
	m_flipV = in_flipV;
}
void SpriteComponent::Draw()
{
	const auto& worldTransform = GetWorldTransform();
	m_sprite.setPosition(std::round(worldTransform.pos.x), std::round(worldTransform.pos.y));
	m_sprite.setRotation(worldTransform.rot);
	m_sprite.setScale(m_flipH ? worldTransform.scale.x * -1.0f : worldTransform.scale.x, m_flipV ? worldTransform.scale.y : -worldTransform.scale.y);
	std::shared_ptr<Shader> shader = m_paletteSet ? m_paletteSet->GetPaletteShader() : nullptr;
	if(shader)
	{
		auto& sfmlShader = shader->GetSFMLShader();
		sfmlShader.setUniform("texture", m_texture->GetSFMLTexture());
		auto paletteTexture = m_paletteSet->GetPaletteTexture(m_paletteIdx);
		sfmlShader.setUniform("palette", paletteTexture->GetSFMLTexture());
		GetTargetSFMLRenderTexture()->draw(m_sprite, &sfmlShader);
	}
	else
	{
		GetTargetSFMLRenderTexture()->draw(m_sprite);
	}
}
TaskHandle<> SpriteComponent::PlayAnim(const std::string& in_animName, bool in_loop, int32_t in_startFrame)
{
	auto tokens = Split(in_animName, "/");
	if(auto spriteSheet = AssetCache<SpriteSheet>::Get()->LoadAsset(std::string() + "data/anims/" + tokens[0]))
	{
		if(auto anim = spriteSheet->GetAnim(tokens[1]))
		{
			m_animTask = PlayAnimTask(anim, in_loop, in_startFrame);
			return m_animTask;
		}
	}
	SQUID_RUNTIME_ERROR("Invalid anim name string");
	return {};
}
void SpriteComponent::Stop()
{
	m_animTask.Kill();
	m_animTask = {};
}
void SpriteComponent::Clear()
{
	Stop();
	m_sprite = {};
}
Task<> SpriteComponent::PlayAnimTask(std::shared_ptr<Anim> in_anim, bool in_loop, int32_t in_startFrame = 0)
{
	TASK_NAME(__FUNCTION__);

	// Setup per-anim state
	SetTexture(in_anim->GetTexture());
	SetPaletteSet(in_anim->GetPaletteSet());
	SetOrigin(in_anim->GetOrigin());

	// Play the frames
	const int32_t numFrames = (int32_t)in_anim->GetNumFrames();
	int32_t frameIdx = in_startFrame;
	float frameAccum = 0.0f;
	do
	{
		while(frameIdx < numFrames)
		{
			// Set current frame
			const auto& frame = in_anim->GetFrame(frameIdx);
			SetSubTexture(frame);
			while(true)
			{
				co_await Suspend();
				auto frameDur = 1.0f / in_anim->GetFrameRate();
				frameAccum += DT() * m_playRate;
				if(frameAccum >= frameDur)
				{
					do
					{
						frameAccum -= frameDur;
						++frameIdx;
					} while(frameAccum >= frameDur);
					break;
				}
			}
		}
		frameIdx -= numFrames;
	} while(in_loop);
}
