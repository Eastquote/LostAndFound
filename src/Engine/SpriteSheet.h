#pragma once

#include <map>
#include <string>
#include <memory>

#include "Vec2.h"
#include "Anim.h"

class SpriteSheet
{
public:
	SpriteSheet(const std::string& in_filename);
	std::shared_ptr<Anim> GetAnim(const std::string& in_name) const;

private:
	std::map<std::string, std::shared_ptr<Anim>> m_anims;
};
