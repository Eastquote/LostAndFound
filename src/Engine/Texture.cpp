#include "Texture.h"

#include <fstream>
#include "TasksConfig.h"

Texture::Texture(const std::string& in_filename)
{
	m_debugFilename = in_filename;
	std::ifstream ifs;
	ifs.open(in_filename.c_str());
	if(ifs)
	{
		ifs.close();
		m_texture.loadFromFile(in_filename.c_str());
	}
	else
	{
		SQUID_RUNTIME_ERROR("Could not load texture");
	}
}
const sf::Texture& Texture::GetSFMLTexture()
{
	return m_texture;
}
