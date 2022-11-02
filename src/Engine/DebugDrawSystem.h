#pragma once

#include <algorithm>
#include <array>

#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>

#include "Vec2.h"
#include "Time.h"
#include "Polygon.h"
#include "Box.h"
#include "GameWindow.h"
#include "RenderTexture.h"
#include "Transform.h"

#include "Engine/LayerManager.h"

class Font;

// Debug Draw System
// provides simple immediate-mode drawing for various primitives, with optional duration

void DrawDebugLine(Vec2f in_p0, Vec2f in_p1, sf::Color in_color = sf::Color::White, Transform in_TM = {}, float in_lifetime = -1.f);
void DrawDebugCircle(Vec2f in_p0, float in_r, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f);
void DrawDebugPoint(Vec2f in_p0, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f);
void DrawDebugBox(Box2f in_box, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f);
void DrawDebugPolygon(const Polygon& in_poly, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f);
void DrawDebugString(Vec2f in_p0, const std::string& in_str, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f);

class DebugDrawSystem
{
public:
	static DebugDrawSystem* Get();

	DebugDrawSystem();
	~DebugDrawSystem();


	void UpdateAndDraw() {
		DrawList(m_lines);
		DrawList(m_circles);
		DrawList(m_boxes);
		DrawList(m_polys);
		DrawList(m_texts);
	}

	void DrawLine(Vec2f in_p0, Vec2f in_p1, sf::Color in_color = sf::Color::White, Transform in_TM = {}, float in_lifetime = -1.f) {
		m_lines.emplace_back(in_p0, in_p1, in_color, in_TM, in_lifetime);
	}
	void DrawCircle(Vec2f in_p0, float in_r, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f) {
		const int numVerts = Math::Clamp<int>((int)(in_r / 5.0f), 12, 24);
		m_circles.emplace_back(in_p0, in_r, numVerts, in_color, in_TM, in_lifetime);
	}
	void DrawPoint(Vec2f in_p0, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f) {
		m_boxes.emplace_back(Box2f::FromCenter(in_p0, { 4.0f, 4.0f }), in_color, in_TM, in_lifetime);
	}
	void DrawBox(Box2f in_box, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f) {
		m_boxes.emplace_back(in_box, in_color, in_TM, in_lifetime);
	}
	void DrawPolygon(const Polygon& in_poly, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f) {
		m_polys.emplace_back(in_poly, in_color, in_TM, in_lifetime);		
	}
	void DrawString(Vec2f in_p0, const std::string& in_str, sf::Color in_color = sf::Color::White, Transform in_TM={}, float in_lifetime = -1.f) {
		m_texts.emplace_back(in_p0, in_str, in_color, in_TM, in_lifetime);
	}

private:
	struct Drawable
	{
		float m_lifetime;

	protected:
		template<typename VertsT>
		void Setup(VertsT& in_verts, sf::Color in_color, Transform in_TM, float in_lifetime) { 
			m_lifetime = in_lifetime;
			
			for (sf::Vertex& vert : in_verts)
			{
				Vec2f pos = { vert.position.x, vert.position.y };
				pos = in_TM.TransformPoint(pos);
				vert.position = ToPt(pos);

				vert.color = in_color;
			}
		}

		static sf::Vector2f ToPt(Vec2f in_pt) { 
			return { in_pt.x, in_pt.y }; 
		}
	};

	struct DDrawLine : public Drawable
	{
		DDrawLine(Vec2f in_p0, Vec2f in_p1, sf::Color in_color, Transform in_TM, float in_lifetime) {
			m_verts[0].position = ToPt(in_p0);
			m_verts[1].position = ToPt(in_p1);
			Setup(m_verts, in_color, in_TM, in_lifetime);
		}

		void Draw(sf::RenderTarget* in_window) const {
			in_window->draw(&m_verts[0], m_verts.size(), sf::PrimitiveType::Lines);
		}

	private:
		std::array<sf::Vertex, 2> m_verts;
	};

	struct DDrawCircle : public Drawable
	{
		DDrawCircle(Vec2f in_p0, float in_r, int32_t in_numVerts, sf::Color in_color, Transform in_TM, float in_lifetime) {
			m_verts.reserve(in_numVerts + 1);
			for (int i = 0; i <= in_numVerts; i++)
			{
				const float theta = i / (float)in_numVerts * (float)M_PI * 2.0f;
				m_verts.emplace_back(sf::Vector2f{ (float)std::cos(theta) * in_r + in_p0.x, (float)std::sin(theta) * in_r + in_p0.y });
			}
			Setup(m_verts, in_color, in_TM, in_lifetime);
		}

		void Draw(sf::RenderTarget* in_window) const {
			in_window->draw(&m_verts[0], m_verts.size(), sf::PrimitiveType::LineStrip);
		}

	private:
		std::vector<sf::Vertex> m_verts;
	};

	struct DDrawBox : public Drawable
	{
		DDrawBox(Box2f in_box, sf::Color in_color, Transform in_TM, float in_lifetime) {
			m_verts[0].position = ToPt(in_box.GetBottomLeft());
			m_verts[1].position = ToPt(in_box.GetBottomRight());
			m_verts[2].position = ToPt(in_box.GetTopRight());
			m_verts[3].position = ToPt(in_box.GetTopLeft());
			m_verts[4].position = ToPt(in_box.GetBottomLeft());
			Setup(m_verts, in_color, in_TM, in_lifetime);
		}

		void Draw(sf::RenderTarget* in_window) const {
			in_window->draw(&m_verts[0], m_verts.size(), sf::PrimitiveType::LineStrip);
		}

	private:
		std::array<sf::Vertex, 5> m_verts;
	};

	struct DDrawPolygon : public Drawable
	{
		DDrawPolygon(const Polygon& in_poly, sf::Color in_color, Transform in_TM, float in_lifetime) {
			const auto& verts = in_poly.GetVerts();

			if (verts.size() > 0)
			{
				m_verts.reserve(verts.size() + 1);
				for (auto v : verts)
				{
					m_verts.push_back(sf::Vertex({ v.x, v.y }, sf::Color::White));
				}
				m_verts.push_back(sf::Vertex({ verts[0].x, verts[0].y }, sf::Color::White));
			}

			Setup(m_verts, in_color, in_TM, in_lifetime);
		}

		void Draw(sf::RenderTarget* in_window) const {
			in_window->draw(&m_verts[0], m_verts.size(), sf::PrimitiveType::LineStrip);
		}

	private:
		std::vector<sf::Vertex> m_verts;
	};

	struct DDrawText : public Drawable
	{
		DDrawText(Vec2f in_p0, const std::string& in_str, sf::Color in_color, Transform in_TM, float in_lifetime);

		void Draw(sf::RenderTarget* in_window) const {
			in_window->draw(m_text);
		}


	private:
		static void InitializeFont();
		static std::shared_ptr<Font> m_font;
		sf::Text m_text;
	};

	std::vector<DDrawLine> m_lines;
	std::vector<DDrawCircle> m_circles;
	std::vector<DDrawBox> m_boxes;
	std::vector<DDrawPolygon> m_polys;
	std::vector<DDrawText> m_texts;

	template<typename DrawableT>
	void DrawList(std::vector<DrawableT>& inout_drawables)
	{
		// get the render target to draw to (we want to draw on top of all post processing)
		std::shared_ptr<RenderTexture> renderTarget = GetWindow()->GetPostProcessRenderTarget();
		sf::RenderTexture* sfmlRenderTarget = renderTarget->GetSFMLRenderTexture();

		// this texture's view tm will be in view space - set it to world space for debug drawing
		const Box2f prevView = renderTarget->GetView();
		renderTarget->SetView(GetWindow()->GetWorldView());

		// draw all and tick lifetimes
		for (DrawableT& drawable : inout_drawables)
		{
			drawable.Draw(sfmlRenderTarget);
			drawable.m_lifetime -= Time::DT();
		}

		// restore view tm
		renderTarget->SetView(prevView);

		// remove expired drawables (after drawing so 0-lifetime ones still draw once)
		inout_drawables.erase(
			std::remove_if(inout_drawables.begin(), inout_drawables.end(), [](DrawableT& d){ return d.m_lifetime <= 0.0f; }),
			inout_drawables.end()
			);
	}
};
