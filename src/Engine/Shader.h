#pragma once

#include <memory>
#include <string>

#include <SFML/Graphics/Shader.hpp>

#include "Engine/Vec2.h"

class RenderTexture;

namespace sf
{
	class Color;
}

class Shader
{
public:
	Shader(const std::string& in_filename);
	sf::Shader& GetSFMLShader();
	void Reload();

	void SetUniform(const std::string& in_name, float in_val);
	void SetUniform(const std::string& in_name, Vec2f in_val);
	void SetUniform(const std::string& in_name, sf::Color in_val);
	void SetUniform(const std::string& in_name, std::shared_ptr<RenderTexture> in_val);
	void SetUniform(const std::string& in_name, const RenderTexture* in_val);

private:
	std::string m_filename;
	sf::Shader m_shader;
};
