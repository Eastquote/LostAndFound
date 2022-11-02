#include "Font.h"

#include <fstream>
#include "TasksConfig.h"

Font::Font(const std::string& in_filename, bool in_smooth)
{
	m_debugFilename = in_filename;
	std::ifstream ifs;
	ifs.open(in_filename.c_str());
	if(ifs)
	{
		ifs.close();
		m_font.loadFromFile(in_filename.c_str());
		m_font.setSmooth(in_smooth);
	}
	else
	{
		SQUID_RUNTIME_ERROR("Could not load font");
	}
}
void Font::SetSmooth(bool in_smooth)
{
	m_font.setSmooth(in_smooth);
}
bool Font::GetSmooth() const
{
	return m_font.isSmooth();
}
const sf::Font& Font::GetSFMLFont()
{
	return m_font;
}
