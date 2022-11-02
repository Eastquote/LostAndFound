#include "Engine/Components/SensorComponent.h"
#include "Engine/MathGeometry.h"

bool SensorComponent::IsTouching(const std::shared_ptr<SensorComponent> in_otherComp) const {
	if((m_mask & in_otherComp->m_category) && (in_otherComp->m_mask & m_category)) {
		auto diff = GetWorldPos() - in_otherComp->GetWorldPos();
		auto thisCompShapeType = GetShape().GetShapeType();
		auto otherCompShapeType = in_otherComp->GetShape().GetShapeType();
		if(thisCompShapeType == eSensorShapeType::Circle && otherCompShapeType == eSensorShapeType::Circle) {
			auto dist = GetWorldPos().Dist(in_otherComp->GetWorldPos());
			auto minDist = GetShape().GetCircle().radius + in_otherComp->GetShape().GetCircle().radius;
			if(dist <= minDist) {
				return true;
			}
		}
		if(thisCompShapeType == eSensorShapeType::Box && otherCompShapeType == eSensorShapeType::Box) {
			Box2f thisBox = Box2f::FromCenter(GetWorldPos(), GetShape().GetBox());
			Box2f otherBox = Box2f::FromCenter(in_otherComp->GetWorldPos(), in_otherComp->GetShape().GetBox());
			std::optional<MinMaxf> xIntersect = thisBox.GetExtentsX().Intersect(otherBox.GetExtentsX());
			if(!xIntersect) return false;

			std::optional<MinMaxf> yIntersect = thisBox.GetExtentsY().Intersect(otherBox.GetExtentsY());
			if(!yIntersect) return false;
			return true;
		}
		if(thisCompShapeType == eSensorShapeType::Box && otherCompShapeType == eSensorShapeType::Circle) {
			Box2f thisBox = Box2f::FromCenter(GetWorldPos(), GetShape().GetBox());
			auto otherCircle = in_otherComp->GetShape().GetCircle();
			auto dist = in_otherComp->GetWorldPos().Dist(Math::ClosestPointOnBox(thisBox, in_otherComp->GetWorldPos()));
			return (dist <= otherCircle.radius); // TODO: Verify integrity of this algorithm -- it is probably close, but wrong
		}
		if(thisCompShapeType == eSensorShapeType::Circle && otherCompShapeType == eSensorShapeType::Box) {
			Box2f otherBox = Box2f::FromCenter(in_otherComp->GetWorldPos(), in_otherComp->GetShape().GetBox());
			auto thisCircle = GetShape().GetCircle();
			auto dist = GetWorldPos().Dist(Math::ClosestPointOnBox(otherBox, GetWorldPos()));
			return (dist <= thisCircle.radius); // TODO: Verify integrity of this algorithm -- it is probably close, but wrong
		}
	}
	return false;
}
void SensorComponent::CallTouchCallback(bool in_bOnTouch, std::shared_ptr<SensorComponent> in_otherSensor) {
	if(m_touchCallback) {
			m_touchCallback(in_bOnTouch, in_otherSensor);
	}
}