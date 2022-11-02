#pragma once

#include "LayerManager.h"

#include "Engine/GameWindow.h"
#include "Engine/RenderTexture.h"
#include "Engine/Shader.h"

RenderLayer::RenderLayer(Vec2u in_renderSize)
{
	Resize(in_renderSize);
	SetView(GetWindow()->GetWorldView());
}

void RenderLayer::SetView(const Box2f& in_view, float in_angle /*= 0.0f*/)
{
	m_renderTexture->SetView(in_view, in_angle);
}

void RenderLayer::Resize(Vec2u in_size)
{
	m_renderTexture = std::make_shared<RenderTexture>(in_size);
}


/////////////////////////////////////////////////////////////////////
void LayerManager::AddLayer(const std::string& in_name)
{
	if (GetLayer(in_name))
	{
		SQUID_RUNTIME_ERROR("Layer already exists");
	}

	m_layers.emplace(in_name, m_renderSize);
}

RenderLayer* LayerManager::GetLayer(const std::string& in_name)
{
	auto found = m_layers.find(in_name);
	if(found != m_layers.end())
	{
		return &found->second;
	}

	return nullptr;
}

void LayerManager::BindShaderUniforms(std::shared_ptr<Shader> in_shader) const
{
	for (auto it = m_layers.begin(); it != m_layers.end(); it++)
	{
		in_shader->SetUniform(it->first + "Texture", it->second.m_renderTexture);
	}
}

void LayerManager::SetWorldView(const Box2f& in_view, float in_angle /*= 0.0f*/)
{
	for (auto it = m_layers.begin(); it != m_layers.end(); it++)
	{
		if (it->second.m_viewMode == eRenderLayerViewMode::World)
		{
			it->second.SetView(in_view, in_angle);
		}
	}
}

void LayerManager::Clear()
{
	for (auto it = m_layers.begin(); it != m_layers.end(); it++)
	{
		it->second.m_renderTexture->Clear(sf::Color::Transparent);
	}
}

void LayerManager::SetRenderSize(Vec2u in_size)
{
	m_renderSize = in_size;

	for (auto it = m_layers.begin(); it != m_layers.end(); it++)
	{
		it->second.Resize(in_size);
	}
}
