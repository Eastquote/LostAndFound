#include "Character.h"

#include "Creature.h"
#include "Effect.h"
#include "Player.h"
#include "Projectile.h"
#include "GameWorld.h"
#include "DamageInfo.h"
#include "Engine/GameWindow.h"
#include "Engine/MathGeometry.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/TileMap.h"
#include "Engine/CollisionWorld.h"

//--- CHARACTER ---//

void Character::Initialize() {
	GameActor::Initialize();
}
void Character::Destroy() {
	GameActor::Destroy();
}
void Character::CardinalUpdate(	Cardinals& in_blockCardinals, float sweepVal, Vec2f offset, std::optional<Box2f> in_collisionBox, 
								std::shared_ptr<Creature> in_creature) const {
	// Set all cardinals' result and threshold values to the full sweepVal to start
	in_blockCardinals.threshold = sweepVal;
	in_blockCardinals.cardinals = {sweepVal, sweepVal, sweepVal, sweepVal};

	// Populate list of sweeps to be performed
	std::vector<Vec2f> cardinalDisplacements = { 
		{ 0.0f,  sweepVal },	//< Up
		{ 0.0f, -sweepVal, },	//< Down
		{ -sweepVal, 0.0f },	//< Left
		{ sweepVal, 0.0f }		//< Right
	};
	auto cDispIdx = 0;
	Vec2f displacement;
	bool isLeavingRoom = false;
	auto world = GetWorld();
	Box2f collisionBox;

	// Determine what we're actually sweeping
	if(in_collisionBox == std::nullopt) {
		// Use character's world collision box
		collisionBox = GetCollisionBoxWorld(); 
	}
	else {
		// Use the box supplied by the in_collisionBox arg
		collisionBox = in_collisionBox.value().TransformedBy(GetWorldTransform());
	}
	collisionBox = collisionBox;
	collisionBox.x += offset.x;
	collisionBox.y += offset.y;
	for(auto& dir : in_blockCardinals.cardinals) {
		displacement = cardinalDisplacements[cDispIdx];
		// If a creature, check if its collision bounds are leaving the room bounds (if so, we'll bounce it back)
		if(in_creature) {
			Box2f displacedCollision = collisionBox;
			displacedCollision.x += displacement.x; //< Only need to test for this in the horizontal case! (no doors on floor/ceiling)
			auto rooms = world->GetRooms();
			std::shared_ptr<Room> spawnerRoom;

			// Determine which room the creature spawned in ("belongs to", essentially)
			for(auto room : rooms) {
				if(std::find(room->spawners.begin(), room->spawners.end(), in_creature->GetSpawner()) != room->spawners.end()) {
					spawnerRoom = room;
					break;
				}
			}
			if(!Math::BoxContainsBox(spawnerRoom->bounds, displacedCollision)) {
				isLeavingRoom = true;
				in_creature->FlipDir();
			}

			// Code to display room bounds for debugging purposes
			auto spawnerRoomCopy = std::make_shared<Room>(*spawnerRoom);
			spawnerRoomCopy->bounds.SetDims_CenterAnchor(spawnerRoom->bounds.GetDims() - Vec2f{ 1.0f, 1.0f });
			//DrawDebugBox(spawnerRoomCopy->bounds, sf::Color::Yellow);
		}

		// Do the sweep
		auto colliderSweepResults = GetWorld()->GetCollisionWorld()->SweepBox(collisionBox, displacement);
		if(isLeavingRoom) {
			dir = -1.0f;
		}

		// If the sweep hit something, set that cardinal's result value to the actual collision distance
		if(colliderSweepResults) {
			dir = colliderSweepResults.value().m_dist;
		}
		cDispIdx++;
	}
}
std::shared_ptr<Projectile> Character::SpawnProjectile(	const ProjectileDef& in_def, const Transform& in_transform, 
														const Vec2f& in_dir, int32_t in_facingDir, const float in_mag) {
	return in_def.Spawn(AsShared<Character>(), in_transform, in_dir, in_facingDir, in_mag);
}
Vec2f Character::Move(	const Vec2f& in_pos, bool in_teleport, float in_sweepVal, std::shared_ptr<Creature> in_creature, 
						bool in_breakStuff, std::optional<Box2f> in_box) {
	auto newPos = in_pos - GetWorldPos();
	Box2f collisionBox;

	// Determine what we're actually sweeping
	if(in_box == std::nullopt){
		collisionBox = m_worldCollisionBoxLocal; //< Use character's world collision box
	}
	else {
		collisionBox = in_box.value(); //< Use the box supplied by the in_box arg
	}
	int32_t tile = 0;
	//DrawDebugBox(GetCollisionBoxWorld(), sf::Color::Red);
	Vec2f currentDir = Vec2f::Zero;
	if(newPos.x != 0.0f) {
		currentDir.x = std::copysign(1.0f, newPos.x);
	}
	if(newPos.y != 0.0f) {
		currentDir.y = std::copysign(1.0f, newPos.y);
	}

	auto colliderSweepResults = GetWorld()->GetCollisionWorld()->SweepBox(collisionBox.TransformedBy(GetWorldTransform()), newPos);
	if(colliderSweepResults && !in_teleport) {
		// If can go through breakable terrain and succeeds at doing so, don't block
		if(in_breakStuff && TryDestroyTiles(in_pos, currentDir.Norm(), false)) {
			colliderSweepResults = GetWorld()->GetCollisionWorld()->SweepBox(collisionBox.TransformedBy(GetWorldTransform()), newPos);
		}
		// If blocked, revise target position accordingly and set Character to that new pos
		if(colliderSweepResults) {
			auto sweptBottomLeft = colliderSweepResults->m_sweptBox.GetBottomLeft();
			auto collisionBottomLeft = collisionBox.TransformedBy(GetWorldTransform()).GetBottomLeft();
			auto newVec = sweptBottomLeft - collisionBottomLeft;
			SetWorldPos(GetWorldPos() + newVec);
			newPos = newVec;
		}
	}
	else {
		SetWorldPos(in_pos);
	}
	CardinalUpdate(m_blockCardinals, in_sweepVal, Vec2f::Zero, in_box, in_creature);
	return newPos;
}
