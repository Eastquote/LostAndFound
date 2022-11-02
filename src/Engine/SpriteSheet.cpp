#include "SpriteSheet.h"

#include <fstream>
#include <sstream>

#include "Engine/AssetCache.h"
#include "Engine/Texture.h"
#include "Engine/PaletteSet.h"

#include "TasksConfig.h"

#include "Engine/StringUtils.h"

//--- SpriteSheet ---//
SpriteSheet::SpriteSheet(const std::string& in_filename)
{
	std::ifstream ifs;
	auto imgFileName = in_filename + ".png";
	auto metaFileName = in_filename + ".txt";

	// Open metadata file handle
	ifs.open(metaFileName.c_str());
	if(ifs)
	{
		// Load texture
		std::shared_ptr<Texture> texture = AssetCache<Texture>::Get()->LoadAsset(imgFileName);
		std::shared_ptr<PaletteSet> paletteSet;

		// Parse metadata
		std::string line;
		std::getline(ifs, line); // Skip first line (hash line)
		std::string name;
		int32_t padding = 0;
		int32_t framerate = 0;
		int32_t palettize = 0;
		Vec2i dims_frame = Vec2i::Zero;
		Vec2i dims_grid = Vec2i::Zero;
		Vec2i origin = Vec2i::Zero;
		while(std::getline(ifs, line))
		{
			std::stringstream sstr;
			auto tokens = Split(line, " ");
			if(!tokens.size())
			{
				continue;
			}
			if(tokens[0][0] == '#')
			{
				if(tokens[1] == "Anims")
					break;
				continue;
			}

#define PARSE_META_INT(name) \
			if(tokens.size() >= 2 && tokens[0] == #name) \
			{ \
				sstr << tokens[1]; \
				sstr >> name; \
			}
#define PARSE_META_VEC2I(name) \
			if(tokens.size() >= 3 && tokens[0] == #name) \
			{ \
				sstr << tokens[1] << " " << tokens[2]; \
				sstr >> name.x >> name.y; \
			}
#define PARSE_META_STRING(name) \
			if(tokens.size() >= 2 && tokens[0] == #name) \
			{ \
				name = tokens[1]; \
			}
			PARSE_META_STRING(name);
			PARSE_META_INT(padding);
			PARSE_META_INT(framerate);
			PARSE_META_INT(palettize);
			PARSE_META_VEC2I(dims_frame);
			PARSE_META_VEC2I(dims_grid);
			PARSE_META_VEC2I(origin);
#undef PARSE_META_INT
#undef PARSE_META_VEC2I
		}

		// Load palette set
		if(palettize != 0)
		{
			std::string paletteBasePath = std::string("data/anims/") + name + "_Palette_";
			paletteSet = AssetCache<PaletteSet>::Get()->LoadAsset(paletteBasePath);
		}

		// Parse + create anims
		while(std::getline(ifs, line))
		{
			auto tokens = Split(line, " ");
			if(!tokens.size() || tokens[0][0] == '#')
			{
				continue;
			}

			std::string name = tokens[0];
			std::vector<Box2i> sprites;
			for(auto it = tokens.begin() + 1; it != tokens.end(); ++it)
			{
				std::stringstream sstr;
				sstr << *it;
				int32_t idx = 0;
				sstr >> idx;
				Vec2i pos = { idx % dims_grid.x, idx / dims_grid.x };
				pos.x *= dims_frame.x + padding * 2;
				pos.y *= dims_frame.y + padding * 2;
				pos.x += padding;
				pos.y += padding;

				sprites.push_back({ pos.x, pos.y, dims_frame.x, dims_frame.y });
			}
			auto anim = std::make_shared<Anim>(texture, paletteSet, sprites, (float)framerate, origin);
			m_anims[name] = anim;
		}

		// Close metadata file handle
		ifs.close();
	}
	else
	{
		SQUID_RUNTIME_ERROR("Could not load spritesheet");
	}
}
std::shared_ptr<Anim> SpriteSheet::GetAnim(const std::string& in_name) const
{
	auto found = m_anims.find(in_name);
	if(found != m_anims.end())
	{
		return found->second;
	}
	return nullptr;
}
