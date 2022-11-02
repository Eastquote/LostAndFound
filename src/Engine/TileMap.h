#pragma once

#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <map>

#include "Vec2.h"
#include "Box.h"
#include "Texture.h"
#include "TasksConfig.h"

class TileSet;
class TileMap;
class TileLayer;
class TileObject;

//--- eTileLayerType ---//
enum class eTileLayerType
{
	Unknown,
	Tile,
	Object,
};

//--- eTileObjectShape ---//
enum class eTileObjectShape
{
	Unknown,
	Point,
	Polygon,
	Chain,
};

//--- TileSet ---//
class TileSet
{
public:
	TileSet(const std::string& in_filename);
	const std::string& GetName() const;
	const std::string& GetFilename() const;
	std::shared_ptr<Texture> GetTexture() const;
	const std::vector<Box2i>& GetTiles() const;
	const Vec2i& GetTileDims() const;

private:
	std::string m_name;
	std::string m_filename;
	std::shared_ptr<Texture> m_texture;
	Vec2i m_tileDims = Vec2i::Zero;
	std::vector<Box2i> m_tiles;
};

//--- TileMap ---//
class TileMap
{
public:
	TileMap(const std::string& in_filename);
	const std::string& GetName() const;
	const std::vector<std::shared_ptr<TileLayer>>& GetLayers() const;
	std::shared_ptr<TileLayer> GetLayer(const std::string& in_layerName) const;
	std::vector<std::shared_ptr<TileObject>> GetObjects() const;
	std::vector<std::shared_ptr<TileObject>> GetObjectsByName(const std::string& in_objName) const;
	std::vector<std::shared_ptr<TileObject>> GetObjectsByType(const std::string& in_objType) const;

private:
	std::string m_name;
	std::vector<std::shared_ptr<TileLayer>> m_layers;
};

//--- TileLayer ---//
class TileLayer
{
public:
	TileLayer(const std::string& in_name, eTileLayerType in_layerType, const Vec2i& in_gridDims, const Vec2i& in_layerOffset = Vec2f::Zero, const Vec2f& in_parallax = Vec2f::One);
	TileLayer(const TileLayer& in_tileLayer) = default;
	void SetTiles(const std::vector<std::shared_ptr<TileSet>>& in_tileSets, const std::vector<int32_t>& in_tiles, const std::vector<MinMaxi>& in_tileIdRanges);
	void SetObjects(const std::vector<std::shared_ptr<TileObject>>& in_objects);

	const std::string& GetName() const;
	eTileLayerType GetLayerType() const;
	const Vec2i& GetLayerOffset() const;
	const Vec2i& GetGridDims() const;
	Box2i GetGridBoundingBox() const;
	const Vec2i& GetTileDims() const;
	const Vec2f& GetParallax() const;
	const std::vector<std::shared_ptr<TileSet>>& GetTileSets() const;
	const std::vector<int32_t>& GetTiles() const;
	const std::vector<std::shared_ptr<TileObject>>& GetObjects() const;
	std::vector<std::shared_ptr<TileObject>> GetObjectsByName(const std::string& in_objName) const;
	std::vector<std::shared_ptr<TileObject>> GetObjectsByType(const std::string& in_objType) const;
	const std::vector<MinMaxi>& GetTileIdRanges() const;
	
	bool IsValidGridCoord(Vec2i in_gridPos) const;
	bool HasTile(Vec2i in_gridPos) const;
	int32_t GetTile(Vec2i in_gridPos) const;
	void SetTile(Vec2i in_gridPos, int32_t in_tile);
	int32_t GridPosToTileIdx(Vec2i in_gridPos) const;
	Vec2i TileIdxToGridPos(int32_t in_tileIdx) const;
	void ReplaceTileSet(std::shared_ptr<TileSet> in_target, std::shared_ptr<TileSet> in_replacement);

private:
	std::string m_name;
	eTileLayerType m_layerType = eTileLayerType::Unknown;
	Vec2i m_gridDims = Vec2f::Zero;
	Vec2i m_layerOffset = Vec2f::Zero;
	Vec2f m_parallax = Vec2f::One;
	std::vector<std::shared_ptr<TileSet>> m_tileSets;
	std::vector<int32_t> m_tiles;
	std::vector<std::shared_ptr<TileObject>> m_objects;
	std::vector<MinMaxi> m_tileIdRanges;
};

//--- TileObject ---//
class TileObject
{
public:
	TileObject(const std::string& in_name, const std::string& in_type, uint32_t in_objId, eTileObjectShape in_shape, const Vec2f& in_pos, float in_rotation,
				const std::vector<Vec2f>& in_points, bool in_flipHori, bool in_flipVert, const std::map<std::string, std::string>& in_props);
	const std::string& GetName() const;
	const std::string& GetType() const; // Can be set per-object (defaults to TileLayer's custom 'type' property, if provided)
	eTileObjectShape GetShape() const; // Point, Rect, Polygon, Chain
	const Vec2f& GetPos() const;
	float GetRotation() const;
	const Box2f& GetBox() const;
	const Vec2f& GetCentroid() const;
	const std::vector<Vec2f>& GetPoints() const;
	bool GetFlipHori() const;
	bool GetFlipVert() const;
	uint32_t GetObjectId() const;

	template <typename tProp>
	std::optional<tProp> GetPropertyAs(const std::string& in_propName) const
	{
		auto found = m_props.find(in_propName);
		if(found != m_props.end())
		{
			std::stringstream sstr;
			tProp propVal;
			sstr << found->second;
			sstr >> propVal;
			return propVal;
		}
		return {};
	}

private:
	std::string m_name;
	std::string m_type;
	uint32_t m_objId = 0;
	eTileObjectShape m_shape = eTileObjectShape::Unknown;
	Vec2f m_pos;
	float m_rotation = 0.0f;
	std::vector<Vec2f> m_points;
	bool m_flipHori = false;
	bool m_flipVert = false;
	std::map<std::string, std::string> m_props; // Custom object properties
	Vec2f m_centroid; // Computed automatically from points
	Box2f m_box = { 0, 0, 0, 0 }; // Computed automatically from points
};
