#include "PaletteSet.h"

#include "MathCore.h"
#include "AssetCache.h"
#include "Texture.h"
#include "Shader.h"
#include "TasksConfig.h"

#include <fstream>

PaletteSet::PaletteSet(const std::string& in_filename)
{
	m_baseFilename = in_filename;
	GetPaletteIdx("Base"); // Add Base palette to the cache
	SQUID_RUNTIME_CHECK(m_paletteTextures.size(), "Could not find base palette texture!");
	m_paletteShader = AssetCache<Shader>::Get()->LoadAsset("data/shaders/Palette");
}
int32_t PaletteSet::GetPaletteIdx(const std::string& in_palette) const
{
	auto found = m_nameToIdxLut.find(in_palette);
	if(found != m_nameToIdxLut.end())
	{
		return found->second;
	}
	std::string paletteFilename = m_baseFilename + in_palette + ".png";
	FILE* file = 0;
	if(fopen_s(&file, paletteFilename.c_str(), "r") != EINVAL && file) // Check if file is accessible before trying to load it
	{
		fclose(file); // Close file handle
		auto paletteTexture = AssetCache<Texture>::Get()->LoadAsset(paletteFilename);
		if(paletteTexture)
		{
			int32_t paletteIdx = (int32_t)m_paletteTextures.size();
			m_paletteTextures.push_back(paletteTexture);
			m_nameToIdxLut[in_palette] = paletteIdx;
			return paletteIdx;
		}
	}
	return 0;
}
std::shared_ptr<Texture> PaletteSet::GetPaletteTexture(int32_t in_paletteIdx) const
{
	int32_t numTextures = (int32_t)m_paletteTextures.size();
	in_paletteIdx = Math::Clamp(in_paletteIdx, 0, numTextures - 1);
	return m_paletteTextures[in_paletteIdx];
}
std::shared_ptr<Shader> PaletteSet::GetPaletteShader() const
{
	return m_paletteShader;
}
