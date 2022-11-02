#pragma once

#include <memory>
#include <string>
#include <map>

template <typename T>
class AssetCache
{
public:
	static AssetCache<T>* Get()
	{
		static AssetCache<T> s_cache;
		return &s_cache;
	}
	std::shared_ptr<T> LoadAsset(const std::string& in_filename)
	{
		auto found = m_cache.find(in_filename);
		if(found != m_cache.end())
		{
			return found->second;
		}
		auto asset = std::make_shared<T>(in_filename);
		m_cache[in_filename] = asset;
		return asset;
	}

	void ReloadAll()
	{
		for (auto it = m_cache.begin(); it != m_cache.end(); it++)
		{
			it->second->Reload();
		}
	}

private:
	AssetCache() {}
	using tAssetCache = std::map<std::string, std::shared_ptr<T>>;
	tAssetCache m_cache;
};
