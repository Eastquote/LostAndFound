#include "TileMap.h"

#include "Engine/AssetCache.h"
#include "Engine/StringUtils.h"

#include "pugixml.hpp"

//--- TileSet ---//
TileSet::TileSet(const std::string& in_filename)
	: m_filename(in_filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(in_filename.c_str());
	if(result.status == pugi::status_ok)
	{
		// Parse tile data
		auto tileset = doc.child("tileset");
		m_name = tileset.attribute("name").value();
		m_tileDims = { tileset.attribute("tilewidth").as_int(), tileset.attribute("tileheight").as_int() };
		int32_t tilecount = tileset.attribute("tilecount").as_int();
		int32_t columns = tileset.attribute("columns").as_int();
		int32_t spacing = tileset.attribute("spacing").as_int();
		int32_t margin = tileset.attribute("margin").as_int();
		auto tw = m_tileDims.x + spacing;
		auto th = m_tileDims.y + spacing;
		for(auto i = 0; i < tilecount; ++i)
		{
			Vec2i gridPos = { i % columns, i / columns };
			auto x = margin + gridPos.x * tw;
			auto y = margin + gridPos.y * th;
			Box2i tile = { x, y, m_tileDims.x, m_tileDims.y };
			m_tiles.push_back(tile);
		}

		// Parse image data
		auto image = tileset.child("image");
		std::string imageFn = image.attribute("source").value();
		std::string basePath = in_filename.substr(0, in_filename.rfind("/"));
		m_texture = AssetCache<Texture>::Get()->LoadAsset(basePath + "/" + imageFn);
	}
}
const std::string& TileSet::GetName() const
{
	return m_name;
}
const std::string& TileSet::GetFilename() const
{
	return m_filename;
}
std::shared_ptr<Texture> TileSet::GetTexture() const
{
	return m_texture;
}
const std::vector<Box2i>& TileSet::GetTiles() const
{
	return m_tiles;
}
const Vec2i& TileSet::GetTileDims() const
{
	return m_tileDims;
}

//--- TileMap ---//
TileMap::TileMap(const std::string& in_filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(in_filename.c_str());
	if(result.status == pugi::status_ok)
	{
		// Parse "header"
		std::string basePath = in_filename.substr(0, in_filename.rfind("/"));
		std::string name = in_filename.substr(basePath.size() + 1);
		auto suffixStart = name.rfind(".");
		if(suffixStart != std::string::npos)
		{
			name = name.substr(0, suffixStart);
		}
		auto map = doc.child("map");
		int32_t width = map.attribute("width").as_int();
		int32_t height = map.attribute("height").as_int();

		// Parse tilesets
		std::vector<std::shared_ptr<TileSet>> tileSets;
		std::vector<MinMaxi> tileIdRanges;
		int32_t lastStartId = 0;
		auto tileset = map.child("tileset");
		while(!tileset.empty())
		{
			int32_t firstgid = tileset.attribute("firstgid").as_int();
			std::string tilesetFn = tileset.attribute("source").value();
			auto tileSetObj = AssetCache<TileSet>::Get()->LoadAsset(basePath + "/" + tilesetFn);
			if(lastStartId > 0)
			{
				tileIdRanges.push_back({ lastStartId, firstgid });
			}
			lastStartId = firstgid;
			tileSets.push_back(tileSetObj);
			tileset = tileset.next_sibling("tileset");
		}
		tileIdRanges.push_back({ lastStartId, INT_MAX });

		// Parse tile layers
		std::map<int32_t, std::shared_ptr<TileLayer>> layers;
		auto layer = map.child("layer");
		while(!layer.empty())
		{
			std::string layerName = layer.attribute("name").value();
			int32_t id = layer.attribute("id").as_int();
			int32_t layerWidth = layer.attribute("width").as_int();
			int32_t layerHeight = layer.attribute("height").as_int();
			int32_t offsetx = layer.attribute("offsetx").as_int(0);
			int32_t offsety = layer.attribute("offsety").as_int(0);
			float parallaxx = layer.attribute("parallaxx").as_float(1.0f);
			float parallaxy = layer.attribute("parallaxy").as_float(1.0f);
			auto tileLayer = std::make_shared<TileLayer>(layerName, eTileLayerType::Tile, Vec2i{ layerWidth, layerHeight }, Vec2i{ offsetx, offsety }, Vec2f{ parallaxx, parallaxy });
			auto data = layer.child("data");
			std::string tilesStr = data.text().get();
			auto tileTokens = Split(tilesStr, ",");
			std::vector<int32_t> tiles;
			for(auto token : tileTokens)
			{
				try {
					int64_t rawTile = std::stoll(Trim(token));
					int32_t tile = (int32_t)(rawTile & 0xfffffff); // Mask out flag bits
					tiles.push_back(tile);
				}
				catch(...) {
					printf("Invalid Tile: %s\n", token.c_str());
					tiles.push_back(0);
				}
			}
			tileLayer->SetTiles(tileSets, tiles, tileIdRanges);
			layers[id] = tileLayer;
			layer = layer.next_sibling("layer");
		}

		const auto ParseCustomProperties = [](pugi::xml_node root) -> std::map<std::string, std::string> {
			// Parse custom properties
			std::map<std::string, std::string> props;
			auto properties = root.child("properties");
			auto prop = properties.child("property");
			while(!prop.empty())
			{
				std::string propName = prop.attribute("name").value();
				std::string propValue = prop.attribute("value").value();;
				props[propName] = propValue;
				prop = prop.next_sibling("property");
			}
			return props;
		};

		// Parse object group layers
		auto objectgroup = map.child("objectgroup");
		while(!objectgroup.empty())
		{
			std::map<std::string, std::string> groupProps = ParseCustomProperties(objectgroup);
			
			std::optional<std::string> defaultObjType;

			auto found = groupProps.find("type");
			if(found != groupProps.end())
			{
				defaultObjType = found->second;
			}

			// Parse all objects in group
			std::vector<std::shared_ptr<TileObject>> objects;
			auto ParsePoints = [](const std::string& in_pointsStr) {
				std::vector<Vec2f> points;
				auto pointTokens = Split(in_pointsStr, " ");
				for(const auto& token : pointTokens)
				{
					auto compTokens = Split(token, ",");
					if(compTokens.size() == 2)
					{
						auto x = std::stof(compTokens[0]);
						auto y = std::stof(compTokens[1]);
						points.push_back({x, -y});
					}
				}
				return points;
			};
			auto object = objectgroup.child("object");
			while(!object.empty())
			{
				std::map<std::string, std::string> props = ParseCustomProperties(object);

				uint32_t id = 0;
				auto idAttr = object.attribute("id");
				if(!idAttr.empty())
				{
					id = idAttr.as_uint();
				}

				uint32_t gid = 0;
				auto gidAttr = object.attribute("gid");
				if(!gidAttr.empty())
				{
					gid = gidAttr.as_uint();
				}
				const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
				const unsigned FLIPPED_VERTICALLY_FLAG = 0x40000000;
				bool flipHori = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
				bool flipVert = (gid & FLIPPED_VERTICALLY_FLAG) != 0;

				float rotation = 0.0f;
				auto rotationAttr = object.attribute("rotation");
				if(!rotationAttr.empty())
				{
					rotation = rotationAttr.as_float();
				}

				std::vector<Vec2f> points;
				eTileObjectShape shape = eTileObjectShape::Point;
				float x = object.attribute("x").as_float();
				float y = object.attribute("y").as_float();

				auto widthAttr = object.attribute("width");
				auto heightAttr = object.attribute("height");
				if(!widthAttr.empty() && !heightAttr.empty())
				{
					shape = eTileObjectShape::Polygon;
					float w = widthAttr.as_float();
					float h = heightAttr.as_float();
					float left = 0;
					float top = 0;
					float right = w;
					float bottom = h;
					points.push_back({ left, top });
					points.push_back({ left, bottom });
					points.push_back({ right, bottom });
					points.push_back({ right, top });
				}
				else if(auto polygon = object.child("polygon"))
				{
					shape = eTileObjectShape::Polygon;
					points = ParsePoints(polygon.attribute("points").value());
				}
				else if(auto polyline = object.child("polyline"))
				{
					shape = eTileObjectShape::Chain;
					points = ParsePoints(polyline.attribute("points").value());
				}
				else if(auto polyline = object.child("point"))
				{
					shape = eTileObjectShape::Point;
					points.push_back({ x, y });
				}
				else
				{
					continue; // Unknown object type
				}

				// Parse custom type attribute
				std::string objType = defaultObjType.value_or("");
				auto typeAttr = object.attribute("type");
				if(!typeAttr.empty())
				{
					objType = typeAttr.value();
				}

				// Create and add the object
				std::string objectName = object.attribute("name").as_string("");
				auto obj = std::make_shared<TileObject>(objectName, objType, id, shape, Vec2f{ x, -y }, rotation, points, flipHori, flipVert, props);
				objects.push_back(obj);
				object = object.next_sibling("object");
			}

			// Make sure we don't have any offset or parallax values set for this object layer
			int32_t offsetx = objectgroup.attribute("offsetx").as_int(0);
			int32_t offsety = objectgroup.attribute("offsety").as_int(0);
			float parallaxx = objectgroup.attribute("parallaxx").as_float(1.0f);
			float parallaxy = objectgroup.attribute("parallaxy").as_float(1.0f);
			SQUID_RUNTIME_CHECK(offsetx == 0 && offsety == 0, "Object layers do not support offset");
			SQUID_RUNTIME_CHECK(parallaxx == 1 && parallaxy == 1, "Object layers do not support parallax");

			// Create and add the object group layer
			std::string objectGroupName = objectgroup.attribute("name").value();
			int32_t id = objectgroup.attribute("id").as_int();
			auto objectLayer = std::make_shared<TileLayer>(objectGroupName, eTileLayerType::Object, Vec2i{ width, height });
			objectLayer->SetObjects(objects);
			layers[id] = objectLayer;
			objectgroup = objectgroup.next_sibling("objectgroup");
		}

		// Add layers to tile map
		for(const auto& layer : layers)
		{
			m_layers.push_back(layer.second);
		}
	}
}
const std::string& TileMap::GetName() const
{
	return m_name;
}
const std::vector<std::shared_ptr<TileLayer>>& TileMap::GetLayers() const
{
	return m_layers;
}
std::shared_ptr<TileLayer> TileMap::GetLayer(const std::string& in_layerName) const
{
	for(auto layer : m_layers)
	{
		if(layer->GetName() == in_layerName)
		{
			return layer;
		}
	}
	return nullptr;
}
std::vector<std::shared_ptr<TileObject>> TileMap::GetObjects() const
{
	std::vector<std::shared_ptr<TileObject>> objects;
	for(auto layer : m_layers)
	{
		const auto& layerObjects = layer->GetObjects();
		objects.insert(objects.end(), layerObjects.begin(), layerObjects.end());
	}
	return objects;
}
std::vector<std::shared_ptr<TileObject>> TileMap::GetObjectsByName(const std::string& in_objName) const
{
	std::vector<std::shared_ptr<TileObject>> objects;
	for(auto layer : m_layers)
	{
		auto layerObjects = layer->GetObjectsByName(in_objName);
		objects.insert(objects.end(), layerObjects.begin(), layerObjects.end());
	}
	return objects;
}
std::vector<std::shared_ptr<TileObject>> TileMap::GetObjectsByType(const std::string& in_objType) const
{
	std::vector<std::shared_ptr<TileObject>> objects;
	for(auto layer : m_layers)
	{
		auto layerObjects = layer->GetObjectsByType(in_objType);
		objects.insert(objects.end(), layerObjects.begin(), layerObjects.end());
	}
	return objects;
}

//--- TileLayer ---//
TileLayer::TileLayer(const std::string& in_name, eTileLayerType in_layerType, const Vec2i& in_gridDims, const Vec2i& in_layerOffset, const Vec2f& in_parallax)
	: m_name(in_name)
	, m_layerType(in_layerType)
	, m_gridDims(in_gridDims)
	, m_layerOffset(in_layerOffset)
	, m_parallax(in_parallax)
{
}
void TileLayer::SetTiles(const std::vector<std::shared_ptr<TileSet>>& in_tileSets, const std::vector<int32_t>& in_tiles, const std::vector<MinMaxi>& in_tileIdRanges)
{
	m_tileSets = in_tileSets;
	m_tiles = in_tiles;
	m_tileIdRanges = in_tileIdRanges;
}
void TileLayer::SetObjects(const std::vector<std::shared_ptr<TileObject>>& in_objects)
{
	m_objects = in_objects;
}
const std::string& TileLayer::GetName() const
{
	return m_name;
}
eTileLayerType TileLayer::GetLayerType() const
{
	return m_layerType;
}
const Vec2i& TileLayer::GetLayerOffset() const
{
	return m_layerOffset;
}
const Vec2i& TileLayer::GetGridDims() const
{
	return m_gridDims;
}

Box2i TileLayer::GetGridBoundingBox() const
{
	return Box2i::FromTopLeft(Vec2i::Zero, m_gridDims);
}

const Vec2i& TileLayer::GetTileDims() const
{
	if(m_tileSets.size())
	{
		return m_tileSets.front()->GetTileDims();
	}
	return Vec2i::Zero;
}
const Vec2f& TileLayer::GetParallax() const
{
	return m_parallax;
}
const std::vector<std::shared_ptr<TileSet>>& TileLayer::GetTileSets() const
{
	return m_tileSets;
}
const std::vector<int32_t>& TileLayer::GetTiles() const
{
	return m_tiles;
}
const std::vector<std::shared_ptr<TileObject>>& TileLayer::GetObjects() const
{
	return m_objects;
}
std::vector<std::shared_ptr<TileObject>> TileLayer::GetObjectsByName(const std::string& in_objName) const
{
	std::vector<std::shared_ptr<TileObject>> objects;
	for(auto& obj : m_objects)
	{
		if(obj->GetName() == in_objName)
		{
			objects.push_back(obj);
		}
	}
	return objects;
}
std::vector<std::shared_ptr<TileObject>> TileLayer::GetObjectsByType(const std::string& in_objType) const
{
	std::vector<std::shared_ptr<TileObject>> objects;
	for(auto& obj : m_objects)
	{
		if(obj->GetType() == in_objType)
		{
			objects.push_back(obj);
		}
	}
	return objects;
}
const std::vector<MinMaxi>& TileLayer::GetTileIdRanges() const
{
	return m_tileIdRanges;
}

bool TileLayer::IsValidGridCoord(Vec2i in_gridPos) const
{
	return GetGridBoundingBox().Contains_InclExcl(in_gridPos);
}

bool TileLayer::HasTile(Vec2i in_gridPos) const
{
	return GetTile(in_gridPos) > 0;
}

int32_t TileLayer::GetTile(Vec2i in_gridPos) const
{
	if (!IsValidGridCoord(in_gridPos)) return -1;

	return m_tiles[GridPosToTileIdx(in_gridPos)];
}

void TileLayer::SetTile(Vec2i in_gridPos, int32_t in_tile)
{
	if(!IsValidGridCoord(in_gridPos)) return;

	m_tiles[GridPosToTileIdx(in_gridPos)] = in_tile;
}

int32_t TileLayer::GridPosToTileIdx(Vec2i in_gridPos) const
{
	// the math here has a y-flip and offset because Tiled editor uses y-down, with tile origins in the top-left, but this engine uses y-up, with tile origins in the bottom-left
	// since tile {0, 0} lies above the axis with its bottom-left corner at the origin, flipping y gives the tile {0, -1}

	// offsetting tiles this way means that we can draw tile maps directly via TilesComponent with top-left anchor, and that we can convert object positions by simply negating y
	return (-in_gridPos.y - 1) * m_gridDims.x + in_gridPos.x;
}

Vec2i TileLayer::TileIdxToGridPos(int32_t in_tileIdx) const
{
	// inverse of GridPosToTileIdx (see comment there)
	Vec2i tileGridPos = { in_tileIdx % m_gridDims.x, in_tileIdx / m_gridDims.x };
	tileGridPos.y = -tileGridPos.y - 1;

	return tileGridPos;
}

void TileLayer::ReplaceTileSet(std::shared_ptr<TileSet> in_target, std::shared_ptr<TileSet> in_replacement)
{
	auto found = std::find(m_tileSets.begin(), m_tileSets.end(), in_target);
	if(found != m_tileSets.end())
	{
		*found = in_replacement;
	}
}

//--- TileObject ---//
TileObject::TileObject(const std::string& in_name, const std::string& in_type, uint32_t in_objId, eTileObjectShape in_shape, const Vec2f& in_pos, float in_rotation,
	const std::vector<Vec2f>& in_points, bool in_flipHori, bool in_flipVert, const std::map<std::string, std::string>& in_props)
	: m_name(in_name)
	, m_type(in_type)
	, m_objId(in_objId)
	, m_shape(in_shape)
	, m_pos(in_pos)
	, m_rotation(in_rotation)
	, m_points(in_points)
	, m_flipHori(in_flipHori)
	, m_flipVert(in_flipVert)
	, m_props(in_props)
{
	float left = FLT_MAX;
	float top = FLT_MAX;
	float bottom = FLT_MIN;
	float right = FLT_MIN;
	Vec2d sum = Vec2d::Zero;
	for(auto p : m_points)
	{
		sum += p;
		left = p.x < left ? p.x : left;
		top = p.y < top ? p.y : top;
		right = p.x > right ? p.x : right;
		bottom = p.y > bottom ? p.y : bottom;
	}
	m_box = { m_pos.x + left, m_pos.y + top, m_pos.x + right - left, m_pos.y + bottom - top };
	m_centroid = m_pos + (Vec2f)(sum / (double)m_points.size());
}
const std::string& TileObject::GetName() const
{
	return m_name;
}
const std::string& TileObject::GetType() const
{
	return m_type;
}
eTileObjectShape TileObject::GetShape() const
{
	return m_shape;
}
const Vec2f& TileObject::GetPos() const
{
	return m_pos;
}
float TileObject::GetRotation() const
{
	return m_rotation;
}
bool TileObject::GetFlipHori() const
{
	return m_flipHori;
}
bool TileObject::GetFlipVert() const
{
	return m_flipVert;
}
uint32_t TileObject::GetObjectId() const
{
	return m_objId;
}
const Box2f& TileObject::GetBox() const
{
	return m_box;
}
const Vec2f& TileObject::GetCentroid() const
{
	return m_centroid;
}
const std::vector<Vec2f>& TileObject::GetPoints() const
{
	return m_points;
}
