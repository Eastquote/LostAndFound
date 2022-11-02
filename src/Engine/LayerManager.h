#pragma once

#include <memory>
#include <string>
#include <map>

#include "Vec2.h"
#include "Box.h"

// Forward declarations
class RenderTexture;
class Shader;

enum class eRenderLayerViewMode
{
	World,
	Manual,
};


class RenderLayer
{
public:
	RenderLayer(Vec2u in_renderSize);

	eRenderLayerViewMode m_viewMode = eRenderLayerViewMode::World;

	std::shared_ptr<RenderTexture> GetRenderTexture() const { return m_renderTexture; }
	void SetView(const Box2f& in_view, float in_angle = 0.0f);

private:
	friend class LayerManager;

	std::shared_ptr<RenderTexture> m_renderTexture;
	void Resize(Vec2u in_size);
};


class LayerManager
{
public:
	void AddLayer(const std::string& in_name);
	RenderLayer* GetLayer(const std::string& in_name);
	
	void BindShaderUniforms(std::shared_ptr<Shader> in_shader) const;
	void SetWorldView(const Box2f& in_view, float in_angle = 0.0f);
	void Clear();
	void SetRenderSize(Vec2u in_size);

private:
	std::map<std::string, RenderLayer> m_layers;
	Vec2u m_renderSize = {0, 0};
};
