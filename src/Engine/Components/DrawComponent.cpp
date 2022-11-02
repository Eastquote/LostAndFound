#include "Engine/Components/DrawComponent.h"
#include "Engine/Game.h"
#include "Engine/GameWindow.h"
#include "Engine/LayerManager.h"
#include "Engine/RenderTexture.h"

void DrawComponent::Initialize()
{
	SceneComponent::Initialize();
	SetRenderLayer("fgGameplay");
}

void DrawComponent::SetRenderLayer(const std::string& in_layerName)
{
	SetTargetRenderTexture(GetWindow()->GetLayerManager()->GetLayer(in_layerName)->GetRenderTexture());
}

void DrawComponent::SetTargetRenderTexture(std::shared_ptr<RenderTexture> in_targetRenderTexture)
{
	m_targetRenderTexture = in_targetRenderTexture;
}

std::shared_ptr<RenderTexture> DrawComponent::GetTargetRenderTexture() const
{
	if (m_targetRenderTexture)
		return m_targetRenderTexture;

	return GameBase::Get()->GetWindow()->GetRenderTarget();
}

sf::RenderTexture* DrawComponent::GetTargetSFMLRenderTexture() const
{
	return GetTargetRenderTexture()->GetSFMLRenderTexture();
}
