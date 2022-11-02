#pragma once

#include "Engine/Components/SceneComponent.h"

class RenderTexture;

namespace sf
{
	class RenderTexture;
}

class DrawComponent : public SceneComponent
{
public:
	virtual void Initialize() override;
	virtual void Update() {}
	virtual void Draw() {}

	void SetComponentDrawOrder(int32_t in_drawOrder) { m_drawOrder = in_drawOrder; }
	int32_t GetDrawOrder() const { return m_drawOrder; }

	void SetHidden(bool in_bHidden) { m_bHidden = in_bHidden; }
	bool GetHidden() const { return m_bHidden; }

	void SetRenderLayer(const std::string& in_layerName);
	void SetTargetRenderTexture(std::shared_ptr<RenderTexture> in_targetRenderTexture);

protected:
	// these return the target we're actually using, so they'll return the default viewport render texture if we don't have one set
	std::shared_ptr<RenderTexture> GetTargetRenderTexture() const;
	sf::RenderTexture* GetTargetSFMLRenderTexture() const;

private:
	int32_t m_drawOrder = 0;
	bool m_bHidden = false;
	std::shared_ptr<RenderTexture> m_targetRenderTexture;
};
