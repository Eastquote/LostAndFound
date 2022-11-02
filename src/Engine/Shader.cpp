#include "Shader.h"

#include <iostream>
#include <fstream>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include "TasksConfig.h"
#include "RenderTexture.h"

Shader::Shader(const std::string& in_filename)
{
	m_filename = in_filename + ".frag";
	Reload();
}

sf::Shader& Shader::GetSFMLShader()
{
	return m_shader;
}

void Shader::Reload()
{
	std::ifstream ifs;
	ifs.open(m_filename.c_str());
	if(ifs)
	{
		ifs.close();
		bool success = m_shader.loadFromFile(m_filename.c_str(), sf::Shader::Fragment);
		if (!success)
		{
			std::cout << "failed to reload shader: " << m_filename << "\n";
			return;
		}
	}
	else
	{
		SQUID_RUNTIME_ERROR("Could not load shader");
	}
	std::cout << "reloaded shader: " << m_filename << "\n";
}

void Shader::SetUniform(const std::string& in_name, float in_val)
{
	m_shader.setUniform(in_name, in_val);
}

void Shader::SetUniform(const std::string& in_name, Vec2f in_val)
{
	m_shader.setUniform(in_name, sf::Vector2f(in_val.x, in_val.y));
}

void Shader::SetUniform(const std::string& in_name, sf::Color in_val)
{
	m_shader.setUniform(in_name, sf::Glsl::Vec4(in_val));
}

void Shader::SetUniform(const std::string& in_name, std::shared_ptr<RenderTexture> in_val)
{
	m_shader.setUniform(in_name, in_val->GetSFMLRenderTexture()->getTexture());
}

void Shader::SetUniform(const std::string& in_name, const RenderTexture* in_val)
{
	if (in_val)
	{
		m_shader.setUniform(in_name, in_val->GetSFMLRenderTexture()->getTexture());
	}
}
