#pragma once

#include "Engine/Components/SceneComponent.h"
#include "Engine/Actor.h"
#include "Engine/Box.h"


struct Circle
{
	float radius = 8.0f;
};

enum class eSensorShapeType {
	None,
	Circle,
	Box,
};

struct SensorShape
{
public:
	eSensorShapeType GetShapeType() const { return m_shapeType; }
	bool IsCircle() const { return (m_shapeType == eSensorShapeType::Circle); }
	bool IsBox() const { return (m_shapeType == eSensorShapeType::Box); }
	const Circle& GetCircle() const { return m_circle; }
	const Vec2f& GetBox() const { return m_box; }
	void SetCircle(Circle in_circle) {
		m_shapeType = eSensorShapeType::Circle;
		m_circle = in_circle;
	}
	void SetBox(Vec2f in_dims) { 
		m_shapeType = eSensorShapeType::Box;
		m_box = Vec2f{ in_dims };
	}

private:
	eSensorShapeType m_shapeType = eSensorShapeType::None;
	Circle m_circle;
	Vec2f m_box = Vec2f{ 16.0f, 16.0f };
};

// Sensor Component
class SensorComponent : public SceneComponent
{
	using Callback = std::function<void(bool, std::shared_ptr<SensorComponent>)>;
public:
	void SetShape(const SensorShape& in_shape) { m_shape = in_shape; }
	const SensorShape& GetShape() const { return m_shape; }
	bool IsTouching(const std::shared_ptr<SensorComponent> in_otherComp) const;
	void SetTouchCallback(Callback in_func) { m_touchCallback = in_func; }
	void CallTouchCallback(bool in_bOnTouch, std::shared_ptr<SensorComponent> in_otherSensor);
	void SetFiltering(uint32_t in_category, uint32_t in_mask) {
		m_category = in_category;
		m_mask = in_mask;
	}
	const Vec2u GetFiltering() { return Vec2u{ m_category, m_mask }; }

private:
	uint32_t m_category = 0;
	uint32_t m_mask = 0;
	SensorShape m_shape;
	Callback m_touchCallback;
};