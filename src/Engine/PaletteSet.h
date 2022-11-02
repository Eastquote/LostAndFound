#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class Texture;
class Shader;

class PaletteSet
{
public:
	PaletteSet(const std::string& in_filename);
	int32_t GetPaletteIdx(const std::string& in_palette) const;
	std::shared_ptr<Texture> GetPaletteTexture(int32_t in_paletteIdx) const;
	std::shared_ptr<Shader> GetPaletteShader() const;

private:
	std::string m_baseFilename;
	mutable std::unordered_map<std::string, int32_t> m_nameToIdxLut;
	mutable std::vector<std::shared_ptr<Texture>> m_paletteTextures;
	std::shared_ptr<Shader> m_paletteShader;
};
