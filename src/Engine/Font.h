#pragma once

#include <string>

#include <SFML/Graphics/Font.hpp>

class Font
{
public:
	Font(const std::string& in_filename, bool in_smooth = true);
	void SetSmooth(bool m_smooth);
	bool GetSmooth() const;
	const sf::Font& GetSFMLFont();

private:
	std::string m_debugFilename;
	sf::Font m_font;
};
