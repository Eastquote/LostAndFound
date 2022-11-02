#pragma once

#include <string>

#include <SFML/Graphics/Texture.hpp>

class Texture
{
public:
	Texture(const std::string& in_filename);
	const sf::Texture& GetSFMLTexture();
	const std::string& GetDebugFilename() const { return m_debugFilename; }

private:
	std::string m_debugFilename;
	sf::Texture m_texture;
};
