#include "Player.h"

#include "GameWorld.h"
#include "GameLoop.h"
#include "GameEnums.h"
#include "TaskFSM.h"
#include "AudioManager.h"
#include "Creature.h"
#include "Pickup.h"
#include "Bomb.h"
#include "Lava.h"
#include "AimReticle.h"
#include "AimReticleManager.h"
#include "Light.h"
#include "Hud.h"
#include "PlayerStatus.h"
#include "ParticleSpawner.h"
#include "ParticleSpawnerDefs.h"
#include "Projectile.h"
#include "Projectiles/Grenade.h"
#include "Projectiles/GrenadeDefs.h"
#include "Projectiles/HomingMissile.h"
#include "Projectiles/HomingMissileDefs.h"
#include "Projectiles/WaveBeam.h"
#include "Projectiles/WaveBeamDefs.h"
#include "ProjectileManager.h"
#include "SavePoint.h"
#include "Algorithms.h"
#include "Engine/InputSystem.h"
#include "Engine/Game.h"
#include "Engine/MathEasings.h"
#include "Engine/MathGeometry.h"
#include "Engine/CollisionWorld.h"
#include "Engine/Components/ShapeComponent.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/SensorComponent.h"
#include "Engine/DebugDrawSystem.h"
#include <iostream>
#include <math.h>
#include <mutex>

struct PlayerButtonState : public ButtonState
{
	using ButtonState::ButtonState;
	void SetActive(bool in_isActive) {
		m_bIsActive = in_isActive;
	}
	bool IsActive() const {
		return m_bIsActive;
	}

private:
	friend class Player;
	virtual void SetPressed(bool in_state) override {
		m_wasPressed = m_isPressed;
		m_isPressed = in_state && IsActive();
	}
	bool m_bIsActive = true;
};

//--- PLAYER CODE ---//

void Player::Initialize() {
	Character::Initialize();
	// Set up world collision
	m_worldCollisionBoxLocal = Box2f::FromBottomCenter(Vec2f{ 0.0f, -15.0f }, { 8.0f, 29.0f });

	// Set up touch sensors
	SensorShape playerSensorShape;
	playerSensorShape.SetBox({ 2.0f, 26.0f });
	m_creatureSensor = MakeSensor(Transform::Identity, playerSensorShape);
	m_creatureSensor->SetFiltering(CL_Player, CL_Enemy | CL_Armor);
	m_creatureSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			OnTouchCreature(std::dynamic_pointer_cast<Creature>(in_other->GetActor()), in_other);
		}
		else if(!in_beginning) {
			OnUnTouchCreature(std::dynamic_pointer_cast<Creature>(in_other->GetActor()), in_other);
		}
	});
	m_bulletSensor = MakeSensor(Transform::Identity, playerSensorShape); //< Uses the same box as creatureSensor (quite skinny!)
	m_bulletSensor->SetFiltering(CL_Player, CL_EnemyBullet | CL_PlayerBomb);
	m_bulletSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(std::dynamic_pointer_cast<Grenade>(in_other->GetActor())) {
				OnTouchGrenade(std::dynamic_pointer_cast<Grenade>(in_other->GetActor()), in_other);
			}
			if(std::dynamic_pointer_cast<Projectile>(in_other->GetActor())) {
				OnTouchBullet(std::dynamic_pointer_cast<Projectile>(in_other->GetActor()), in_other);
			}
		}
	});
	playerSensorShape.SetBox({ 8.0f, 27.0f });
	m_objectSensor = MakeSensor(Transform::Identity, playerSensorShape); //< Uses a wider box to make in-game objects easier to snag
	m_objectSensor->SetFiltering(CL_Player, CL_Pickup | CL_Door | CL_Lava | CL_SavePoint | CL_Trigger);
	m_objectSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(std::dynamic_pointer_cast<Lava>(in_other->GetActor())) {
				OnTouchLava(std::dynamic_pointer_cast<Lava>(in_other->GetActor()));
			}
			if(std::dynamic_pointer_cast<SavePoint>(in_other->GetActor())) {
				printf("Can save!\n");
				m_bCanSave = true;
			}
		}
		else if(!in_beginning){
			if(std::dynamic_pointer_cast<Lava>(in_other->GetActor())) {
				OnUnTouchLava();
			}
			if(std::dynamic_pointer_cast<SavePoint>(in_other->GetActor())) {
				printf("	Can't save!\n");
				m_bCanSave = false;
			}
		}
	});
	SetDamageInfo(1, DF_Player);

	// Setup sprites
	SetDrawLayer(1);
	m_playerSprite_effect = MakeSprite({ Vec2f{ 0.0f, 7.0f } });
	m_playerSprite_effect->SetComponentDrawOrder(-0);
	m_playerSprite_full = MakeSprite({ Vec2f{ 0.0f, 0.0f } });
	m_playerSprite_full->SetComponentDrawOrder(2);
	m_playerSprite_upper = MakeSprite({ Vec2f{ 0.0f, 0.0f } });
	m_playerSprite_upper->SetComponentDrawOrder(1);
	m_playerSprite_lower = MakeSprite({ Vec2f{ 0.0f, 0.0f } });
	m_playerSprite_lower->SetComponentDrawOrder(0);
	m_dashLineSprite = MakeSprite();
	m_dashReticleSprite = MakeSprite();
	m_dashReticleSprite->PlayAnim("AimReticle/Idle", true);
	m_dashReticleSprite->SetHidden(true);
	m_dashLineSprite->PlayAnim("DashReticle/DashReticle", true);
	m_dashLineSprite->SetHidden(true);

	// Setup lights
	m_light = Spawn<Light>({}, "LightLarge/Core", "LightLarge/Penumbra");
	m_light->SetScale(0.0f);
	m_light->InterpScale(1.0f, 0.5f);
	m_light->AttachToActor(AsShared<Actor>(), false);

	// Setup input
	m_inputComp = MakeChild<InputComponent>();
	m_startInput = m_inputComp->Button<PlayerButtonState>("Start", { eButton::Joy_Start, eButton::Escape });
	m_testInput = m_inputComp->Button<PlayerButtonState>("Test", { eButton::Joy_LeftBumper, eButton::A });
	m_ejectInput = m_inputComp->Button<PlayerButtonState>("Eject", { eButton::Joy_RightBumper, eButton::E });
	m_rightInput = m_inputComp->Button<PlayerButtonState>("Right", { eButton::Joy_Right, eButton::Right });
	m_leftInput = m_inputComp->Button<PlayerButtonState>("Left", { eButton::Joy_Left, eButton::Left });
	m_upInput = m_inputComp->Button<PlayerButtonState>("Up", { eButton::Joy_Up, eButton::Up });
	m_downInput = m_inputComp->Button<PlayerButtonState>("Down", { eButton::Joy_Down, eButton::Down });
	m_jumpInput = m_inputComp->Button<PlayerButtonState>("Jump", { eButton::Joy_DiamondBottom, eButton::Z });
	m_primaryFireInput = m_inputComp->Button<PlayerButtonState>("FirePrimary", { eButton::Joy_DiamondLeft, eButton::X });
	m_secondaryFireInput = m_inputComp->Button<PlayerButtonState>("FireSecondary", { eButton::Joy_DiamondRight, eButton::C });
	m_cyclePrimaryInput = m_inputComp->Button<PlayerButtonState>("CyclePrimary", { eButton::Joy_LeftBumper, eButton::Q });
	m_grappleInput = m_inputComp->Button<PlayerButtonState>("Grapple", { eButton::Joy_RightTrigger, eButton::E });
	SetControlsEnabled(false);

	// Ground character properly
	CardinalUpdate(m_blockCardinals);
	CardinalUpdate(m_blockCardinals, 255.0f, Vec2f::Zero);
	Move(GetWorldPos() + Vec2f{ 0.0f, -m_blockCardinals.DownDist() }, true);

	// Modify gravity properly if necessary
	GravityCheck();
	//printf("Player spawned!\n");
}
void Player::GravityCheck() {
	if(GetWorld()->GetPlayerStatus()->IsHighJumpUnlocked()) {
		m_gravityModifier = 0.66667f;
	}
	else {
		m_gravityModifier = 1.0f;
	}
}
void Player::SetControlsEnabled(bool in_enabled) {
	m_startInput->SetActive(in_enabled);
	m_testInput->SetActive(in_enabled);
	m_ejectInput->SetActive(in_enabled);
	m_rightInput->SetActive(in_enabled);
	m_leftInput->SetActive(in_enabled);
	m_upInput->SetActive(in_enabled);
	m_downInput->SetActive(in_enabled);
	m_jumpInput->SetActive(in_enabled);
	m_primaryFireInput->SetActive(in_enabled);
	m_secondaryFireInput->SetActive(in_enabled);
	m_cyclePrimaryInput->SetActive(in_enabled);
	m_grappleInput->SetActive(in_enabled);
}
void Player::Destroy() {
	Character::Destroy();
}
void Player::Update() {
	if(!m_bIsInDoorTransition) {
		Character::Update();
	}
}
void Player::OnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor) {
	m_currentTouches += 1;
	HandleDamage(in_creature->GetDamageInfo());
	if(m_bIsInvincible && m_bIsDashing) {
		DamageInfo dmgInfo;
		dmgInfo.m_damageFlags = DF_Dash | DF_Charged;
		dmgInfo.m_instigator = AsShared<GameActor>();
		dmgInfo.m_payload = 3;
		dmgInfo.m_damagePos = GetWorldPos() + ((in_creature->GetWorldPos() - GetWorldPos()) / 2);
		auto hitResponse = in_creature->HandleDamage(dmgInfo, in_sensor);
		if(hitResponse != eHitResponse::Ignore) {
			// TODO: Play hit effect here
		}
	}
}
void Player::OnUnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor) {
	m_currentTouches -= 1;
}
// Corrects the location on Player that enemies fire bullets at (in both standing and ball states)
Vec2f Player::GetTargetPos() {
	return m_bIsBall ? GetWorldPos() + Vec2f{ 0.0f, 8.0f } : GetWorldPos() + Vec2f{ 0.0f, 6.0f };
}
void Player::LaserTouch(std::shared_ptr<Creature> in_creature, const Vec2f& in_zapPos) {
	//printf("PLAYER: touched laser!\n");
	HandleDamage(in_creature->GetDamageInfo());
}
// Activates "TouchingLava" state and effects (see Player::WhileTouchingLava())
void Player::OnTouchLava(std::shared_ptr<Lava> in_lava) {
	m_bIsTouchingLava = true;
	m_bCanAccumulateJump = true;
	m_touchingLava = in_lava;
}
// Reduces player's movement power and causes damage while touching lava
Task<> Player::WhileTouchingLava() {
	auto damageTimer = 0.0f;
	auto lavaTimer = 0.0f;
	const auto jumpAccumulationTime = 10.0f / 60.0f;
	while(true) {
		if(m_touchingLava) {
			// Slow player movement
			m_lavaSpeedModifier = 0.4f;
			// Hurt player periodically
			if(damageTimer >= 0.0925) {
				HandleDamage(m_touchingLava->GetDamageInfo());
				damageTimer = 0.0f;
			}
			damageTimer += DT();
			// If floating, reduce jump power
			if(!m_blockCardinals.Down()) {
				// The longer player touches lava, the more of their jump they get back (up to 90%)
				if(lavaTimer < jumpAccumulationTime && m_bCanAccumulateJump){
					m_lavaJumpModifier = 0.4f + (0.5f * (lavaTimer / jumpAccumulationTime));
				}
				else if(lavaTimer >= jumpAccumulationTime && m_bCanAccumulateJump) {
					m_lavaJumpModifier = 0.9f;
				}
				m_bRunningJump = true;
			}
			// If they touch the ground before the timer expires, max out their lava jump power immediately
			else {
				m_bCanAccumulateJump = true;
				m_bRunningJump = false;
				m_lavaJumpModifier = 0.9f;
			}
			lavaTimer += DT();
		}
		else {
			// Once they stop touching lava, reset everything but jump power
			lavaTimer = 0.0f;
			damageTimer = 0.0f;
			m_lavaSpeedModifier = 1.0f;
			// Jump power only returns to 100% once they've touched the ground outside of the lava for the first time
			if(m_blockCardinals.Down() && m_lavaJumpModifier != 1.0f) {
				//printf("Resetting m_lavaJumpModifier to 1.0!\n");
				m_lavaJumpModifier = 1.0f;
			}
		}
		co_await Suspend();
	}
}
void Player::OnUnTouchLava() {
	m_touchingLava = {};
}
void Player::OnTouchBullet(std::shared_ptr<Projectile> in_bullet, std::shared_ptr<SensorComponent> in_sensor) {
	HandleDamage(in_bullet->GetDamageInfo());
}
eHitResponse Player::HandleDamage(const DamageInfo& in_dmgInfo) {
	std::optional<DamageInfo> damage = {};
	auto world = GetWorld();
	m_hitDamageInfo = in_dmgInfo;
	m_prevDamagePayload = in_dmgInfo.m_payload;
	m_prevInstigator = in_dmgInfo.m_instigator;
	m_prevHitDamageInfo = in_dmgInfo;
	damage = ModifyDamage(in_dmgInfo);
	if(damage.has_value()) {
		// Special-casing bullets from the first bossfight to force-eject player instead of death
		auto breakSuit = in_dmgInfo.m_damageFlags & DF_SuitBreaking;
		if(breakSuit && !world->GetPlayerStatus()->IsCinematicUnlocked(3)) {
			world->GetPlayerStatus()->UnlockCinematic(3);
			m_bForceEject = true;
		}
		auto damagePos = damage.value().m_damagePos;
		if(damagePos != Vec2f::Zero) {
			auto touchDist = damagePos - GetWorldPos();
			if(touchDist.x == 0.0f) {
				m_hitDir = 0;
			}
			else {
				m_hitDir = touchDist.x > 0.0f ? 1 : -1;
			}
		}
		else {
			m_hitDir = damage.value().m_instigator->DistanceFromPlayer().x >= 0 ?	 1 /*< Hit came from player's right*/ : 
																					-1 /*< Hit came from player's left*/;
		}

		// Subtract payload from player health
		world->ChangePlayerHealth(-damage.value().m_payload);
		m_bHitReactRequest = true;
		AudioManager::Get()->PlaySound("Hurt", 0.0f, 0.6f);
		if(world->GetPlayerHealth() <= 0) {
			DeferredDestroy();
			AudioManager::Get()->PlaySound("Die");
		}
		return eHitResponse::Hit;
	}
	return eHitResponse::Ignore;
}
std::optional<DamageInfo> Player::ModifyDamage(const DamageInfo& in_dmgInfo) {
	auto out_dmgInfo = in_dmgInfo;
	if(m_bIsInvincible) { //< No damage is player is invincible
		return std::nullopt;
	}
	if(std::dynamic_pointer_cast<Creature>(in_dmgInfo.m_instigator)) { //< No damage if instigator is a frozen creature
		auto creatureInstigator = std::dynamic_pointer_cast<Creature>(in_dmgInfo.m_instigator);
		auto frozenInstigator = creatureInstigator->CheckFrozen();
		if(frozenInstigator) {
			return std::nullopt;
		}
	}
	else if(in_dmgInfo.m_damageFlags & DF_SuitBreaking) { //< Damage is one less than total current energy if damage is suit-breaking
		auto currentPlayerEnergy = GetWorld()->GetPlayerStatus()->GetEnergy();
		out_dmgInfo.m_payload = currentPlayerEnergy > 1 ? currentPlayerEnergy - 1 : 0;
	}
	return out_dmgInfo;
}
void Player::OnTouchGrenade(std::shared_ptr<Grenade> in_grenade, std::shared_ptr<SensorComponent> in_sensor) {
	HandleDamage(in_grenade->GetDamageInfo());
}
Task<> Player::ManagePlayerFSM() {
	// Set up the finite state machine for player
	TaskFSM fsm;

	// Suit states
	auto standState = fsm.State<void>("Stand", [this] { return StandState(); });
	auto idleState = fsm.State<void>("Idle", [this] { return IdleState(); });
	auto jumpState = fsm.State<void>("Jump", [this] { return JumpState(); });
	auto fallState = fsm.State<void>("Fall", [this] { return FallState(false); });
	auto fallTransitioningState = fsm.State<void>("FallTransitioning", [this] { return FallState(true); });
	auto moveState = fsm.State<void>("Move", [this] { return MoveState(); });
	auto ballState = fsm.State<void>("Ball", [this] { return BallState(); });
	auto ballFallState = fsm.State<void>("BallFall", [this] { return BallFallState(false); });
	auto ballFallTransitioningState = fsm.State<void>("BallFallTransitioning", [this] { return BallFallState(true); });
	auto bounceState = fsm.State<void>("Bounce", [this] { return BounceState(); });
	auto dashState = fsm.State<void>("Dash", [this] { return DashState(); });
	auto grappleState = fsm.State<void>("Grapple", [this] { return GrappleState(); });

	// Fish states
	auto fishIdleState = fsm.State<void>("FishIdle", [this] { return FishIdleState(); });
	auto fishJumpState = fsm.State<void>("FishJump", [this] { return FishJumpState(false); });
	auto fishTransitioningState = fsm.State<void>("FishJumpTransitioning", [this] { return FishJumpState(true); });
	auto fishFallState = fsm.State<void>("FishFall", [this] { return FishFallState(); });
	auto fishSuitTransitioningState = fsm.State<void>("FishFall", [this] { return FishSuitTransitioningState(); });

	fsm.EntryLinks({
		standState.Link(),
	});

	// Stand Exits
	fsm.StateLinks(standState, {
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		moveState.Link([this] { return ShouldEnterMoveState(false); }),
		ballFallTransitioningState.Link([this] { return ShouldEnterBallFallState(); }),
	});

	// Idle Exits
	fsm.StateLinks(idleState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		fallState.Link([this] { return ShouldEnterFallState(); }),
		moveState.Link([this] { return ShouldEnterMoveState(false); }),
		ballFallTransitioningState.Link([this] { return ShouldEnterBallFallState(); }),
		bounceState.Link([this] { return ShouldBounce(); }),
	});

	// Jump Exits
	fsm.StateLinks(jumpState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		grappleState.Link([this] { return ShouldEnterGrappleState(); }),
		dashState.Link([this] { return ShouldEnterDashState(); }),
		ballFallState.OnCompleteLink([this] { return m_bIsBall; }),
		fallState.OnCompleteLink(),
	});

	// Fall Exits
	fsm.StateLinks(fallState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		grappleState.Link([this] { return ShouldEnterGrappleState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		moveState.Link([this] { return ShouldEnterMoveState(true); }),
		idleState.Link([this] { return ShouldEnterIdleState(true); }),
	});

	// FallTransitioning Exits
	fsm.StateLinks(fallTransitioningState, {
		moveState.Link([this] { return ShouldEnterMoveState(true); }),
		idleState.Link([this] { return ShouldEnterIdleState(true); }),
		fallState.OnCompleteLink(),
	});

	// Move Exits
	fsm.StateLinks(moveState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		fallState.Link([this] { return ShouldEnterFallState(); }),
		idleState.OnCompleteLink(),
	});

	// Ball Exits
	fsm.StateLinks(ballState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		dashState.Link([this] { return ShouldEnterDashState(); }),
		fallTransitioningState.Link([this] { return ShouldLeaveBallState(); }),
		ballFallState.Link([this] { return ShouldEnterFallState(); }),
		bounceState.Link([this] { return ShouldBounce(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		idleState.OnCompleteLink(),
	});

	// BallFall Exits
	fsm.StateLinks(ballFallState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		fallTransitioningState.Link([this] { return ShouldLeaveBallState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		dashState.Link([this] { return ShouldEnterDashState(); }),
		bounceState.Link([this] { return ShouldBounce(); }),
		ballState.Link([this] { return IsGrounded(); }),
	});

	// BallFallTransition Exits
	fsm.StateLinks(ballFallTransitioningState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		fallTransitioningState.Link([this] { return ShouldLeaveBallState(); }),
		bounceState.Link([this] { return ShouldBounce(); }),
		ballState.Link([this] { return IsGrounded(); }),
		ballState.OnCompleteLink(),
	});

	// Bounce Exits
	fsm.StateLinks(bounceState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }),
		fallTransitioningState.Link([this] { return ShouldLeaveBallState(); }),
		fallState.OnCompleteLink([this] { return m_bBouncedStanding; }),
		ballState.Link([this] { return IsGrounded(); }),
		ballFallState.OnCompleteLink(),
	});

	// Dash Exits
	fsm.StateLinks(dashState, {
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		ballState.OnCompleteLink([this] { return IsGrounded(); }),
		ballFallState.OnCompleteLink([this] { return !IsGrounded(); }),
	});

	// Grapple Exits
	fsm.StateLinks(grappleState, {
		fallState.OnCompleteLink(),
	});

	// FishIdle Exits
	fsm.StateLinks(fishIdleState, {
		fishJumpState.Link([this] { return ShouldEnterFishJumpState(); }),
	});

	// FishJump Exits
	fsm.StateLinks(fishTransitioningState, {
		fishFallState.OnCompleteLink(),
	});

	// FishTransitioning Exits
	fsm.StateLinks(fishJumpState, {
		fishFallState.OnCompleteLink(),
	});

	// FishFall Exits
	fsm.StateLinks(fishFallState, {
		fishSuitTransitioningState.Link([this] { return ShouldEnterFishSuitTransitioningState(); }),
		fishIdleState.Link([this] { return IsGrounded(); }),
	});

	// FishSuitTransitioning Exits
	fsm.StateLinks(fishSuitTransitioningState, {
		idleState.OnCompleteLink(),
	});

	co_await fsm.Run([](TransitionDebugData in_data) {
		// Uncomment for state machine debugging purposes
		//printf("Player State Change: %d -> %d (%s -> %s)\n", in_data.oldStateId.idx, in_data.newStateId.idx, in_data.oldStateName, in_data.newStateName);
	});
}
Task<> Player::StandState() {
	m_upperAnimName = "Util/Blank";
	m_lowerAnimName = "Util/Blank";
	m_fullAnimName = "GariAppear/GariAppear";
	while(true) {
		co_await Suspend();
	}
}
Task<> Player::IdleState() {
	// Flag-setting
	m_bIsBall = false;
	m_runningJumpInputTimer = 0.0f;
	m_bHitReactRequest = false;
	m_bIsMovingLaterally = false;
	m_bIsSomersaulting = false;
	m_bRunningJump = false;
	m_bFiredInAir = false;
	m_lastFrameDisp.x = 0.0f;

	// Play Idle anims by default
	m_playerSprite_upper->SetFlipHori(!m_bIsFacingRight);
	m_playerSprite_lower->SetFlipHori(!m_bIsFacingRight);
	m_fullAnimName = "Util/Blank";
	m_upperAnimName = "Gari_U/GariIdle_U";
	m_lowerAnimName = "Gari_L/GariIdle_L";

	// Main Idle Loop (the contents of this loop will execute once per tick for as long as we are in this state)
	while(true) { //< 
		if(m_bIsTouchingLava) { //< Suck the player down into lava (n.b this is NOT FallState())
			Move({ GetWorldPos().x, GetWorldPos().y - k_lavaGravity * DT() });
		}
		co_await Suspend();
	}
}
Task<> Player::JumpState() {
	AudioManager::Get()->PlaySound("Jump");
	m_bHitReactRequest = false;
	m_bCanAccumulateJump = false;
	m_bIsSomersaulting = false;
	std::shared_ptr<Token> isJumpingToken = m_bIsJumping.TakeToken(__FUNCTION__); //< Similar to setting a flag, but much less prone to spaghetti outcomes

	// Set up animations
	if(!m_bIsBall) { //< Supply "standing" posture body anims
		m_fullAnimName = "Util/Blank";
		m_upperAnimName = "Gari_U/GariJump_U";
		m_lowerAnimName = "Gari_L/GariJump_L";
		// Scale hitbox up relative to its center/top, to smooth out getting "feet" over ledge lips when jumping up
		ResizePlayer("fullShorter");
	}
	else { //< Supply "ball" posture body anims
		m_upperAnimName = "Util/Blank";
		m_lowerAnimName = "Util/Blank";
		m_fullAnimName = "GariBall/GariBall";
	}

	// Track jump "Quality Of Life" feature usage
	if(m_bJumpBuffered) {
		m_qolBenefit += 1;
	}
	if(m_bCoyoteTime) {
		m_qolBenefit += 1;
	}
	
	//Check if it's a wall jump
	if(m_bCanWallJump) {
		AudioManager::Get()->PlaySound("Footstep01"); //< Play a little wall-jump sound
	}

	// Check if it's a charge jump
	if(!IsGrounded() && m_bIsCharged) {
		m_bChargeJumped = true;
	}

	// Re-set flags
	m_bJumpBuffered = false;
	m_bCoyoteTime = false;
	m_bRunningJump = false;
	m_bRunningJumpInputTimerElapsed = true;
	m_bRunningJumpLateralInputReleased = true;

	// Check if it's a running jump
	bool isMoving = m_rightInput->IsPressed() || m_leftInput->IsPressed();
	if(isMoving && !m_bIsBall) {
		m_bRunningJump = true;
		m_bRunningJumpInputTimerElapsed = false;
		m_bRunningJumpLateralInputReleased = false;
		m_runningJumpInputTimer = 0.0f;
	}

	// Check if it's a mid-air jump
	if(!IsGrounded() && !m_bCanWallJump) {
		m_bAirJumping = true;
	}

	// Play jump dust effects if grounded
	if(IsGrounded()) {
		Vec2f offset = m_bIsBall ? Vec2f{ 0.0f, 0.0f } : Vec2f{ 0.0f, -15.0f };
		Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + offset }, "JumpDust/JumpDust");
	}

	// Start Run parallel task (this handles lateral movement & contextual animation behavior)
	TaskHandle<> conRunJump;
	if(m_bRunningJump) {
		conRunJump = m_taskMgr.Run(ConditionalRunTask(false, k_runningJumpRunSpeed));
	}
	else {
		conRunJump = m_taskMgr.Run(ConditionalRunTask(false, k_standingJumpRunSpeed));
	}

	// Setup movement data
	float startY = GetWorldPos().y;
	const float minJumpTime = 0.14f;
	const float knockbackJumpTime = 0.1f;
	bool knockbackJumpTimeElapsed = false;
	bool minJumpElapsed = false;
	float yImpulse = k_jumpImpulse * m_lavaJumpModifier;
	if(m_bIsBall) {
		yImpulse = k_jumpImpulse * 0.8f * m_lavaJumpModifier; //< Can't jump as high in Ball form
	}
	bool flip = !m_bIsFacingRight;
	float elapsedTime = 0.0f;

	// Main Jump Loop (the contents of this loop will execute once per tick for as long as we are in this state)
	while((!minJumpElapsed || m_jumpInput->IsPressed()) || (!knockbackJumpTimeElapsed && m_bIsInvincible)) {
		elapsedTime += DT();
		if(elapsedTime > minJumpTime) { //< Ensures that the player's jumps have a minimum height
			minJumpElapsed = true; 
		}
		if(elapsedTime > knockbackJumpTime) { //< Same goes for knockback "jumps" that resulted from being hurt
			knockbackJumpTimeElapsed = true;
		}

		// If still rising
		if(yImpulse >= 0.0f){
			Move({ GetWorldPos().x, GetWorldPos().y + yImpulse * DT() });

			// If running jump, start somersaulting eventually
			if(m_bRunningJump && elapsedTime > 0.15f && !m_bIsSomersaulting && !m_bIsBall) {
				m_upperAnimName = "Util/Blank";
				m_lowerAnimName = "Util/Blank";
				m_fullAnimName = "GariFlip/GariFlip";
				m_playerSprite_full->SetFlipHori(flip);
				m_bIsSomersaulting = true;
			}

			// If collision box hits ceiling, jump is over
			if(m_blockCardinals.Up()) {
				break; //< FSM will now use JumpState()'s OnCompleteLink() -- FallState() in this case
			}
			if(m_bIsSomersaulting) {
				// Handle walljump attempts, if enabled
				if(!TryWallJump(5.0f)) {
					m_upperAnimName = "Util/Blank";
					m_lowerAnimName = "Util/Blank";
					m_fullAnimName = "GariFlip/GariFlip";
					m_playerSprite_full->SetFlipHori(flip);
					m_bCanWallJump = false;
				}
			}
		}

		// If max height reached, jump is over
		else {
			break; //< FSM will now use JumpState()'s OnCompleteLink() -- FallState() in this case
		}
		if(m_bFiredInAir && !m_bIsSomersaulting && !m_bIsBall) {
			m_upperAnimName = "Gari_U/GariFireAir_U";
		}
		yImpulse -= k_jumpGravity * m_gravityModifier; //< Apply gravity once per tick
		co_await Suspend();
	}
	m_bAirJumping = false;
	m_jumpPeak = GetWorldPos().y - startY;
}
Task<> Player::FallState(bool in_bIsTransitioningFromBall) {
	m_bBouncedStanding = false;
	m_bHitReactRequest = false;
	auto isFallingToken = m_bIsFalling.TakeToken(__FUNCTION__);
	CardinalUpdate(m_blockCardinals, 18.0f);

	// Resize player collision as needed (see various cases below)
	if(in_bIsTransitioningFromBall && m_bIsBall) {
		auto transitionDisp = 14.0f;
		if(m_blockCardinals.DownDist() == 0.0f) {
			// CASE: Transitioning from ball while grounded (pop up to a margin of 18px before full resize)
			Move({ GetWorldPos().x, GetWorldPos().y + transitionDisp + 4.0f });
		}
		else if(m_blockCardinals.DownDist() < transitionDisp) {
			// CASE: Transitioning from ball while falling, not enough room (pop up to a margin of 14px before full resize)
			Move({ GetWorldPos().x, GetWorldPos().y + (transitionDisp - m_blockCardinals.DownDist()) });
		}
		if(m_blockCardinals.DownDist() > transitionDisp) {
			// CASE: Transitioning from ball while falling, plenty of room so no height offset necessary (no-op)
		}
	}
	if(!m_bIsBall && m_blockCardinals.DownDist() < 4.0f) { //< Checks if clear to "fix" shortened lower collsion extent at jump apex
		// CASE: Transitioning to falling from a jump, not enough room (pop up to a margin of 4px before resize to full)
		Move({ GetWorldPos().x, GetWorldPos().y + (4.0f - m_blockCardinals.DownDist()) });
	}
	else {
		// CASE: Transitioning to falling from a jump, plenty of room (no-op)
	}
	ResizePlayer(); //< Returns player collision to "full size" by default
	m_bIsBall = false;
	m_bFallingFromLedge = true;

	// Start Run parallel task
	TaskHandle<> conRunFall;
	if(m_bRunningJump) {
		conRunFall = m_taskMgr.Run(ConditionalRunTask(false, k_runningJumpRunSpeed));
	}
	else if(m_bIsBall || m_bCoyoteTime) {
		conRunFall = m_taskMgr.Run(ConditionalRunTask(false, k_runSpeed));
	}
	else {
		conRunFall = m_taskMgr.Run(ConditionalRunTask(false, k_standingJumpRunSpeed));
	}

	// Setup anims
	m_upperAnimName = "Gari_U/GariFall_U";
	m_lowerAnimName = "Gari_L/GariFall_L";
	m_fullAnimName = "Util/Blank";

	// Setup jump forgiveness
	auto jumpBufferWindow = 0.1333f;
	auto coyoteTimeWindow = 0.1f;
	auto jumpForgiveness = m_taskMgr.Run(JumpForgivenessTask(jumpBufferWindow, coyoteTimeWindow));

	auto startY = GetWorldPos().y;
	auto yImpulse = 0.0f;
	while(m_bIsFalling) {
		if(yImpulse > -k_jumpImpulse) {
			Move({ GetWorldPos().x, GetWorldPos().y + yImpulse * DT() });
		}
		else {
			Move({ GetWorldPos().x, GetWorldPos().y + -k_jumpImpulse * DT() });
		}
		if(m_bIsSomersaulting) {
			// Handle walljump, if enabled
			if(!TryWallJump(5.0f)) {
				m_upperAnimName = "Util/Blank";
				m_lowerAnimName = "Util/Blank";
				m_fullAnimName = "GariFlip/GariFlip";
				m_playerSprite_full->SetFlipHori(!m_bIsFacingRight);
				m_bCanWallJump = false;
			}
		}

		// Stop somersaulting once close to starting height of jump...
		if(m_bRunningJump && (m_jumpPeak - (startY - GetWorldPos().y) < 11.0f) && m_bIsSomersaulting) {
			m_upperAnimName = "Gari_U/GariFall_U";
			m_lowerAnimName = "Gari_L/GariFall_L";
			m_fullAnimName = "Util/Blank";
			m_bIsSomersaulting = false;
			m_runningJumpInputTimer = 0.0;
		}

		// ...or after firing
		if(m_bFiredInAir) {
			m_upperAnimName = "Gari_U/GariFireAir_U";
			m_lowerAnimName = "Gari_L/GariFall_L";
			m_fullAnimName = "Util/Blank";
		}
		yImpulse -= k_jumpGravity * m_gravityModifier;
		co_await Suspend();
	}
	m_bHitReactRequest = false;
}
Task<> Player::MoveState() {
	// Start Run parallel task
	TaskHandle<> conRunGround;
	conRunGround = m_taskMgr.Run(ConditionalRunTask(true, k_runSpeed));

	while((m_rightInput->IsPressed() && !m_blockCardinals.Right()) || (m_leftInput->IsPressed() && !m_blockCardinals.Left())) {
		if(IsGrounded()) {
			m_runningJumpInputTimer = 0.0f;
			if(m_bIsTouchingLava) {
				Move({ GetWorldPos().x, GetWorldPos().y - k_lavaGravity * DT() });
			}
		}
		co_await Suspend();
	}
}
Task<> Player::BallState() {
	m_bCoyoteTime = false;
	m_bIsBall = true;
	auto conRunBall = m_taskMgr.Run(ConditionalRunTask(false, k_runSpeed));
	while(true) {
		m_playerSprite_full->SetFlipHori(!m_bIsFacingRight);
		m_bHitReactRequest = false;
		if(m_bIsTouchingLava) {
			Move({ GetWorldPos().x, GetWorldPos().y - k_lavaGravity * DT() });
		}
		co_await Suspend();
	}
}
Task<> Player::BallFallState(bool in_bIsTransitioningToBall) {
	m_bIsBall = true;
	auto isFallingToken = m_bIsFalling.TakeToken(__FUNCTION__);
	if(in_bIsTransitioningToBall) {
		AudioManager::Get()->PlaySound("BallEnter");
	}

	// Setup anims, resize collision if needed
	TaskHandle<> transitionAnimTask;
	std::shared_ptr<Token> manualAnimToken;
	if(in_bIsTransitioningToBall) {
		ResizePlayer("ball");
		m_playerSprite_upper->PlayAnim("Util/Blank", true);
		m_playerSprite_lower->PlayAnim("Util/Blank", true);
		transitionAnimTask = m_playerSprite_full->PlayAnim("GariBallTrans/GariBallTrans", false);
		manualAnimToken = m_manualAnimUOverride.TakeToken(__FUNCTION__);
	}
	else {
		m_upperAnimName = "Util/Blank";
		m_lowerAnimName = "Util/Blank";
		m_fullAnimName = "GariBall/GariBall";
	}
	// Start Run parallel task
	TaskHandle<> conRunBallFall;
	conRunBallFall = m_taskMgr.Run(ConditionalRunTask(false, k_runSpeed));

	// Setup jump forgiveness
	auto jumpBufferWindow = 0.1333f;
	auto coyoteTimeWindow = 0.1f;
	auto jumpForgiveness = m_taskMgr.Run(JumpForgivenessTask(jumpBufferWindow, coyoteTimeWindow));

	auto yImpulse = 0.0f;
	while(m_bIsFalling) {
		m_playerSprite_full->SetFlipHori(!m_bIsFacingRight);
		m_bHitReactRequest = false;
		if(transitionAnimTask && transitionAnimTask.IsDone()) {
			m_upperAnimName = "Util/Blank";
			m_lowerAnimName = "Util/Blank";
			m_fullAnimName = "GariBall/GariBall";
			transitionAnimTask = {};
			manualAnimToken = {};
		}
		if(yImpulse > -k_jumpImpulse) {
			Move({ GetWorldPos().x, GetWorldPos().y + yImpulse * DT() });
		}
		else {
			Move({ GetWorldPos().x, GetWorldPos().y + -k_jumpImpulse * DT() });
		}
		if(!IsGrounded()) {
			m_lastFrameDisp = GetWorldPos() - m_lastFramePos;;
		}
		m_lastFramePos = GetWorldPos();
		yImpulse -= k_jumpGravity * m_gravityModifier;
		co_await Suspend();
	}
}
Task<> Player::BounceState() {
	m_bHitReactRequest = false;
	m_bCoyoteTime = true;
	if(!m_bBouncedStanding) {
		m_bIsBall = true;
	}
	m_bIsBouncing = true;
	auto bounceScalar = 0.6f;
	auto startY = GetWorldPos().y;
	auto startVel = std::abs(m_lastFrameDisp.y) * 60.0f * bounceScalar;

	// Start parallel run task
	TaskHandle<> conRunBounce;
	conRunBounce = m_taskMgr.Run(ConditionalRunTask(false, k_runSpeed));

	// Setup jump forgiveness
	auto coyoteTimeWindow = 0.075f;
	auto jumpForgiveness = m_taskMgr.Run(JumpForgivenessTask(0.0, coyoteTimeWindow));

	auto currentVel = startVel;
	while(m_bIsBouncing) {
		if(currentVel >= 0) {
			Move({ GetWorldPos().x, GetWorldPos().y + currentVel * DT() });
			if(m_blockCardinals.Up()) {
				break;
			}
		}
		else {
			break;
		}
		currentVel -= k_jumpGravity * m_gravityModifier;
		co_await Suspend();
	}
	m_bIsBouncing = false;
	m_jumpPeak = GetWorldPos().y - startY;
}
Task<> Player::DashState() {
	m_bJustDashed = false;
	const auto dashDuration = 0.225f;
	const auto dashChargeMultiplier = m_fireHeur.primaryChargedFiring ? 6.0f : 0.0f;
	const auto dashMag = m_dashVec.Norm() * dashChargeMultiplier * 60.0f;
	auto elapsedTime = 0.0f;
	std::shared_ptr<Token> isDashingToken;
	isDashingToken = m_bIsDashing.TakeToken(__FUNCTION__);
	std::shared_ptr<Token> isInvincibleToken;
	if(m_fireHeur.primaryChargedFiring) {
		isInvincibleToken = m_bIsInvincible.TakeToken(__FUNCTION__);
	}
	auto frameCount = 0;
	auto prevPalette = m_playerSprite_full->GetPalette();
	m_lastFramePos = GetWorldPos();
	auto offset = Vec2f{ 0.0f, 7.0f };
	std::vector<Vec2f> trailPoints;
	trailPoints.push_back(GetWorldPos());
	m_dashReticleSprite->SetHidden(true);
	m_dashLineSprite->SetHidden(true);
	AudioManager::Get()->PlaySound("Jump");
	bool testedCorner = false;
	auto cornerForgiven = false;
	while(true) {
		if(m_bIsCharged) {
			if(frameCount % 2 == 0) {
				m_playerSprite_full->SetPalette("Charged");
			}
			else {
				m_playerSprite_full->SetPalette(prevPalette);
			}
		}
		if(elapsedTime < k_dashDuration) {
			if(!testedCorner && (m_blockCardinals.Right() || m_blockCardinals.Left())) {
				auto yPosRounded = std::round(GetWorldPos().y);
				auto xImpulse = dashMag.x * DT();

				// Test new Y positions one pixel at a time up to the forgiveness threshold
				for(auto i = 0; i <= m_cornerForgiveness && !cornerForgiven; ++i) {
					auto flipVal = 1.0f;
					auto testBox = m_worldCollisionBoxLocal;
					for(auto j = 0; j < 2; j++) { //< Tests pos AND neg versions of the offset (i.e. offsets up and down)
						auto boxOffsetPos = Vec2f{ GetWorldPos().x, yPosRounded } + Vec2f{ 0.0f, float(i) * flipVal };
						auto boxOffsetTransform = Transform{ boxOffsetPos, 0.0f, Vec2f::One };
						auto testSweepVec = Vec2f{ xImpulse, 0.0f }.Norm();
						auto colliderSweepResults = GetWorld()->GetCollisionWorld()->SweepBox(m_worldCollisionBoxLocal.TransformedBy(boxOffsetTransform), testSweepVec);
						auto tryDestroyResults = TryDestroyTiles(boxOffsetPos, Vec2f{ xImpulse, 0.0f }.Norm(), false);
						if(!colliderSweepResults) {
							Move(boxOffsetPos, false, 0.01f, nullptr, true);
							Move(boxOffsetPos + Vec2f{ xImpulse, 0.0f }, false, 0.01f, nullptr, true);
							cornerForgiven = true;
							break;
						}
						flipVal *= -1.0f;
					}
				}
				testedCorner = true;
				cornerForgiven = false;
			}
			else {
				Move({ GetWorldPos().x + dashMag.x * DT(), GetWorldPos().y }, false, 0.01f, nullptr, true);
			}
			cornerForgiven = false;
			if(!testedCorner && dashMag.y != 0.0f && (m_blockCardinals.Up() || m_blockCardinals.Down())) {
				std::vector<Vec2f> testPositions;
				auto xPosRounded = std::round(GetWorldPos().x);
				auto yImpulse = dashMag.y * DT();
				auto collisionWorld = GetWorld()->GetCollisionWorld();

				// Test new Y positions one pixel at a time up to the forgiveness threshold
				for(auto i = 0; i <= m_cornerForgiveness && !cornerForgiven; ++i) { 
					auto flipVal = 1.0f;
					auto testBox = m_worldCollisionBoxLocal;
					for(auto j = 0; j < 2; j++) { //< Tests pos AND neg versions of the offset (i.e. offsets up and down)
						auto boxOffsetPos = Vec2f{ xPosRounded, GetWorldPos().y } + Vec2f{ float(i) * flipVal, 0.0f};
						auto boxOffsetTransform = Transform{ boxOffsetPos, 0.0f, Vec2f::One };
						auto testSweepVec = Vec2f{ 0.0f, yImpulse }.Norm();
						auto colliderSweepResults = collisionWorld->SweepBox(testBox.TransformedBy(boxOffsetTransform), testSweepVec);
						auto tryDestroyResults = TryDestroyTiles(boxOffsetPos, Vec2f{  0.0f, yImpulse }.Norm(), false);
						if(!colliderSweepResults) {
							Move(boxOffsetPos, false, 0.01f, nullptr, true);
							Move(boxOffsetPos + Vec2f{ 0.0f, yImpulse }, false, 0.01f, nullptr, true);
							cornerForgiven = true;
							break;
						}
						flipVal *= -1.0f;
					}
				}
				testedCorner = true;
				cornerForgiven = false;
			}
			else {
				Move({ GetWorldPos().x, GetWorldPos().y + dashMag.y * DT() }, false, 0.01f, nullptr, true);
			}
			m_lastFrameDisp = GetWorldPos() - m_lastFramePos;
			testedCorner = false;
		}
		else {
			break;
		}
		m_lastFramePos = GetWorldPos();
		if(m_lastFrameDisp != Vec2f::Zero) {
			Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, m_lastFrameDisp.SignedAngleDeg() }, "DashTrail/DashTrail", std::nullopt);
		}
		co_await Suspend();
		elapsedTime += DT();
		frameCount++;
	}
	m_playerSprite_full->SetPalette(prevPalette);
}
Task<> Player::GrappleState() { //< NOTE: deprecated feature
	m_bAirJumping = false;
	auto swingRadius = GetWorldPos().Dist(m_grappleAnchor);
	auto delta = Vec2f::Zero;
	auto swingForceYDir = -1;
	auto ropeVector = m_grappleAnchor - GetWorldPos();
	auto ropeAngle = ropeVector.SignedAngleDeg();
	auto halfSwingDegrees = (ropeAngle < 90) ? (90 - ropeAngle / 2.0f) : (ropeAngle - 90 / 2);
	auto prevRopeAngle = ropeAngle;
	while(true) {
		ropeVector = m_grappleAnchor - GetWorldPos();
		ropeAngle = ropeVector.SignedAngleDeg();
		auto swingForceXDir = (ropeAngle > 270.0f || ropeAngle < 90) ? 1 : -1;
		auto ropeAngleRadians = float(Math::DegreesToRadians(ropeAngle));
		delta += Vec2f{ (k_jumpGravity * sin(ropeAngleRadians) * swingForceXDir), 0.0f };
		auto newPos = GetWorldPos() + delta;
		auto newRopeVec = m_grappleAnchor - newPos;
		auto newRopeAngle = 90 - newRopeVec.SignedAngleDeg();
		auto xSideDir = newRopeAngle > 0 ? -1 : 1;
		ropeAngleRadians = float(Math::DegreesToRadians(newRopeAngle));
		auto xSide = swingRadius * sin(ropeAngleRadians) * -1;
		auto ySide = swingRadius * cos(ropeAngleRadians) * -1;
		Move(m_grappleAnchor + Vec2f{ xSide, ySide });
		if(m_grappleInput->JustReleased()) {
			break;
		}
		prevRopeAngle = ropeAngle;
		co_await Suspend();
	}
}
Task<> Player::FishIdleState() {
	SetControlsEnabled(true);
	m_bIsFish = true;
	// Play fish idle anim
	m_upperAnimName = "Util/Blank";
	m_lowerAnimName = "Util/Blank";
	m_fullAnimName = "Fish/Idle";
	m_playerSprite_full->SetPlayRate(0.0f);
	while(true) {
		// TODO: flip anim to look left on left input, right on right input
		co_await Suspend();
	}
}
Task<> Player::FishJumpState(bool in_isEjecting) {
	if(in_isEjecting) {
		// Transform into fish
		ResizePlayer("fish");
		m_light->InterpScale(0.5f, 1.0f);
		//co_await WaitSeconds(1.0f);

		// TODO: swap to fishy oxygen meter hud here
		
		// Make invincible
		auto isInvincibleToken = m_bIsInvincible.TakeToken(__FUNCTION__);
	}
	m_upperAnimName = "Util/Blank";
	m_lowerAnimName = "Util/Blank";
	m_fullAnimName = "Fish/Flip";
	m_playerSprite_full->SetPlayRate(1.0f);
	
	// Flip dir based on L/R input
	if((m_bIsFacingRight && m_leftInput->IsPressed()) || (!m_bIsFacingRight && m_rightInput->IsPressed())) {
		m_bIsFacingRight = !m_bIsFacingRight;
	}
	auto dir = m_bIsFacingRight ? 1.0f : -1.0f;
	auto xImpulse = 1.0f * 60.0f * dir;
	auto yImpulse = 1.0f * 60.0f;
	

	// Start Run parallel task
	auto conRunJump = m_taskMgr.Run(ConditionalRunTask(false, abs(xImpulse)));
	SetControlsEnabled(false);
	while(true) {
		// Kill horizonal momentum when a wall is hit (so fish will fall straight down)
		if((xImpulse > 0.0f && m_blockCardinals.Right()) || (xImpulse < 0.0f && m_blockCardinals.Left())) {
			xImpulse = 0.0f;
		}
		if(yImpulse >= 0.0f) {
			Move({ GetWorldPos().x + xImpulse * DT(), GetWorldPos().y });
			Move({ GetWorldPos().x, GetWorldPos().y + yImpulse * DT() });
			if(m_blockCardinals.Up()) {
				break;
			}
		}
		else {
			break;
		}
		yImpulse -= k_jumpGravity * m_gravityModifier;
		co_await Suspend();
	}
}
Task<> Player::FishFallState() {
	m_bJumpEnded = false;
	auto dir = m_bIsFacingRight ? 1.0f : -1.0f;
	auto xImpulse = 1.0f * 60.0f * dir;
	auto yImpulse = 0.0f;

	// Start Run parallel task
	auto conRunJump = m_taskMgr.Run(ConditionalRunTask(false, abs(xImpulse)));
	m_upperAnimName = "Util/Blank";
	m_lowerAnimName = "Util/Blank";
	m_fullAnimName = "Fish/Flip";
	auto jumpBufferWindow = 0.1333f;
	auto coyoteTimeWindow = 0.1f;
	while(true) {
		if((xImpulse > 0.0f && m_blockCardinals.Right()) || (xImpulse < 0.0f && m_blockCardinals.Left())) {
			xImpulse = 0.0f;
		}

		// Coyote time
		if(m_bCoyoteTime) {
			if(coyoteTimeWindow <= 0.0f) {
				m_bCoyoteTime = false;
			}
			coyoteTimeWindow -= DT();
		}

		// Jump buffer
		if(!m_bCoyoteTime && m_jumpInput->JustPressed()) {
			m_bJumpBuffered = true;
		}
		if(m_jumpInput->IsPressed() && m_bJumpBuffered) {
			if(jumpBufferWindow <= 0.0f) {
				m_bJumpBuffered = false;
			}
			jumpBufferWindow -= DT();
		}
		if(m_jumpInput->JustReleased()) {
			m_bJumpBuffered = false;
		}

		if(yImpulse > -k_jumpImpulse) {
			Move({ GetWorldPos().x + xImpulse * DT(), GetWorldPos().y });
			Move({ GetWorldPos().x, GetWorldPos().y + yImpulse * DT()});
		}
		else {
			Move({ GetWorldPos().x + xImpulse * DT(), GetWorldPos().y });
			Move({ GetWorldPos().x, GetWorldPos().y + -k_jumpImpulse * DT() });
		}
		yImpulse -= k_jumpGravity * m_gravityModifier;
		co_await Suspend();
	}
}
Task<> Player::FishSuitTransitioningState() {
	SetControlsEnabled(false);

	// Setup anims
	m_playerSprite_upper->SetFlipHori(!m_bIsFacingRight);
	m_playerSprite_lower->SetFlipHori(!m_bIsFacingRight);
	m_playerSprite_full->SetFlipHori(!m_bIsFacingRight);
	m_upperAnimName = "Util/Blank";
	m_lowerAnimName = "Util/Blank";

	// Resize collision and place on floor cleanly
	ResizePlayer("full", 0.0f, 4.0f);
	CardinalUpdate(m_blockCardinals, 10.0f);
	Move(GetWorldPos() + Vec2f{ 0.0f, -m_blockCardinals.DownDist() });
	CardinalUpdate(m_blockCardinals, 1.0f);

	// Play suit-activation animation
	auto manualAnimToken = m_manualAnimUOverride.TakeToken(__FUNCTION__);
	m_light->InterpScale(1.0f, 3.0f);
	co_await m_playerSprite_full->PlayAnim("GariSuitEntry/Activate", false);
	m_fullAnimName = "Util/Blank";
	m_upperAnimName = "Gari_U/GariIdle_U";
	m_lowerAnimName = "Gari_L/GariIdle_L";
	m_bIsInSuitTransition = false;

	// Save game
	auto status = GetWorld()->GetPlayerStatus();
	status->SetSpawnPos(GetWorldPos());
	status->SaveRecharge();

	// Show hud here
	GetWorld()->GetHud()->InterpHudVisiblity(true, 1.0f, false, true);
	m_bIsFish = false;
	GravityCheck();
	SetControlsEnabled(true);

	// HACK: suit-entry only happens in this one spot, so this is a hardcoded background music cue
	AudioManager::Get()->PlayMusic("TorchOfKnowledge", 0.0f, 0.035f);
}
bool Player::ShouldEnterJumpState() {
	auto canJump = IsGrounded() || m_bCoyoteTime || m_bCanWallJump || (!IsGrounded() && m_bIsCharged);
	auto justJumped = m_jumpInput->JustPressed() || m_bJumpBuffered;
	if(canJump && justJumped) {
		return true;
	}
	return false;
}
bool Player::ShouldEnterMoveState(bool in_transitioningFromFall) {
	CardinalUpdate(m_blockCardinals);
	if(	m_blockCardinals.Down() && 
		((m_rightInput->IsPressed() && !m_blockCardinals.Right()) || (m_leftInput->IsPressed() && !m_blockCardinals.Left()))) {
		m_bRunningJump = false;
		if(in_transitioningFromFall){
			AudioManager::Get()->PlaySound("GroundHit", 0.0f, 0.5f);
			if(!m_bFallingFromLedge) {
				printf("       SHOULD ENTER MOVE STATE\n");
				m_qolBenefit += 1;
				//std::cout << "QOL Benefit: short jump collision!  |  Total Benefits: " << m_qolBenefit << std::endl;
				ResizePlayer("full", 0.0f, 4.0f);
			}
		}
		m_bFallingFromLedge = false;
		return true;
	}
	return false;
}
bool Player::ShouldEnterIdleState(bool in_transitioningFromFall) {
	if(IsGrounded() && (
		(!m_rightInput->IsPressed() && !m_leftInput->IsPressed()) || 
		(m_rightInput->IsPressed() && m_blockCardinals.Right()) || 
		(m_leftInput->IsPressed() && m_blockCardinals.Left())
		)) {

		// If player "snuck" up over landable surface due to helper collision shrinkage, resize and place cleanly on ground
		if(in_transitioningFromFall){
			AudioManager::Get()->PlaySound("GroundHit", 0.0f, 0.6f);
			if(!m_bFallingFromLedge) {
				//printf("       SHOULD ENTER IDLE STATE\n");
				Cardinals transitionCardinals;
				CardinalUpdate(transitionCardinals, 10.0f);
				if(transitionCardinals.Down()) {
					Move({ GetWorldPos().x, GetWorldPos().y + (10.0f - (float)ceil(transitionCardinals.DownDist())) });
					CardinalUpdate(transitionCardinals, 10.0f);
					if(transitionCardinals.Down()) {
						return false;
					}
				}
				ResizePlayer("full", 0.0f, 4.0f);
				m_qolBenefit += 1;
				//std::cout << "QOL Benefit: short jump collision!  |  Total Benefits: " << m_qolBenefit << std::endl;
			}
		}
		m_bFallingFromLedge = false;
		return true;
	}
	return false;
}
bool Player::ShouldEnterFallState() {
	if(!IsGrounded()) {
		// These flags need to be set here as the JumpState->FallState transition is a fallthrough that never hits this function
		m_bCoyoteTime = true;
		m_bFallingFromLedge = true;
		return true;
	}
	return false;
}
bool Player::ShouldEnterGrappleState() {
	if(m_bGrappled) {
		return true;
	}
	return false;
}
bool Player::ShouldEnterBallFallState() {
	if(GetWorld()->GetPlayerStatus()->IsBallUnlocked() && IsGrounded() && m_downInput->JustPressed() && !m_bIsChargingSecondary) {
		return true;
	}
	return false;
}
bool Player::IsGrounded() const {
	return (m_blockCardinals.Down() || m_bIsTouchingLava);
}
bool Player::ShouldBounce() {
	// Check if in a "tunnel" (i.e. a one-tile-height open space)
	CardinalUpdate(m_blockCardinals, 0.29f);
	auto inTunnel = m_blockCardinals.Up();

	// "Normal" bounce (from hitting the ground as a ball)
	if(m_bIsBall && IsGrounded() && std::abs(m_lastFrameDisp.y) * k_bounceScalar > 1.0f && !m_bIsTouchingLava && !inTunnel) {
		AudioManager::Get()->PlaySound("GroundHit", 0.0f, 0.7f);
		return true;
	}

	// "Forced" bounce (usually from touching something that hurts)
	else if(m_bBounceRequest || (m_bIsBall && m_bHitReactRequest && !m_bIsTouchingLava)) {
		m_lastFrameDisp.y = 3.0f; // Force a vertical impulse
		m_bBounceRequest = false;
		if(!m_bIsBall) {
			m_bBouncedStanding = true;
		}
		return true;
	}
	else {
		return false;
	}
}             
bool Player::ShouldLeaveBallState() {
	// Make sure there's enough room to accomodate the full standing-height collision box
	auto standingRoom = 29.0f - 15.0f;
	Cardinals ballTransitionOutCardinals;
	CardinalUpdate(ballTransitionOutCardinals, standingRoom);
	auto actualRoom = ballTransitionOutCardinals.UpDist() + ballTransitionOutCardinals.DownDist();
	auto hasStandingRoom = actualRoom >= standingRoom;
	if(m_upInput->JustPressed() && hasStandingRoom && !m_primaryFireInput->IsPressed()) {
		AudioManager::Get()->PlaySound("BallExit");
		return true;
	}
	return false;
}
bool Player::ShouldEnterDashState() {
	return m_bJustDashed && m_dashVec != Vec2f::Zero ? true : false;
}
bool Player::ShouldEnterFishJumpState() {
	if(m_bForceEject) { //< Check if this "jump" coincides with an ejection event (a very rare case!)
		std::shared_ptr<PlayerStatus> status = GetWorld()->GetPlayerStatus();
		status->SetupNewSuit(); //< SIDE EFFECT: Re-set player's health, weapons, abilities etc. to defaults
		m_bForceEject = false;
		return true;
	}
	else if(m_bIsFish && m_jumpInput->IsPressed()) { //< Nominal case
		return true;
	}
	return false;
}
bool Player::ShouldEnterFishSuitTransitioningState() {
	return m_bIsInSuitTransition ? true : false;
}
void Player::ResizePlayer(std::string in_profile, float in_cardinalDistCheck, float in_verticalMove) {
	auto baseHeight = 29.0f;
	auto baseWidth = 8.0f;
	auto heightOffset = 0.0f;
	auto fishHeightOffset = 0.0f;
	if(in_cardinalDistCheck) {
		Cardinals distCheckCardinals;
		CardinalUpdate(distCheckCardinals, in_cardinalDistCheck);
	}
	if(in_verticalMove) {
		Move({ GetWorldPos().x, GetWorldPos().y + in_verticalMove });
	}
	if(in_profile == "full") {
	}
	else if(in_profile == "fullShorter") { //< Used during during jumps to make clearing obstacles a bit more forgiving
		heightOffset = 4.0f;
	}
	else if(in_profile == "ball") {
		heightOffset = 14.0f;
	}
	else if(in_profile == "fish") {
		heightOffset = 14.0f;
		fishHeightOffset = 10.0f;
	}
	else {
		printf("Player::ResizePlayer() error: invalid in_profile name!\n");
		return;
	}
	m_worldCollisionBoxLocal.SetDims_TopCenterAnchor({ baseWidth, baseHeight - heightOffset });
	if(in_profile == "fish") {
		// Shrink again from the bottom to reduce height
		m_worldCollisionBoxLocal.SetDims_BottomCenterAnchor({ baseWidth, baseHeight - heightOffset - fishHeightOffset });
	}
	else if(in_profile != "fish" && m_bIsFish) {
		m_worldCollisionBoxLocal = Box2f::FromBottomCenter(Vec2f{ 0.0f, -15.0f }, { 8.0f, 29.0f });
		m_worldCollisionBoxLocal.SetDims_TopCenterAnchor({ baseWidth, baseHeight - heightOffset });
	}
	SensorShape playerSensorShape;
	playerSensorShape.SetBox({ 2.0f, baseHeight - heightOffset - 3.0f });
	m_bulletSensor->SetShape(playerSensorShape);
	m_bulletSensor->SetRelativePos({ 0.0f, 0.0f + (heightOffset / 2) });
	m_creatureSensor->SetShape(playerSensorShape);
	m_creatureSensor->SetRelativePos({ 0.0f, 0.0f + (heightOffset / 2) });
	playerSensorShape.SetBox({ baseWidth, baseHeight - heightOffset - 2.0f });
	m_objectSensor->SetShape(playerSensorShape);
	m_objectSensor->SetRelativePos({ 0.0f, 0.0f + (heightOffset / 2) });
}
bool Player::TryWallJump(float in_threshold) {
	// Check cardinals for walljump purposes (sweepVal arg is walljump threshold in px)
	CardinalUpdate(m_blockCardinals, in_threshold);
	auto wallJumpUnlocked = GetWorld()->GetPlayerStatus()->IsWallJumpUnlocked();
	if(wallJumpUnlocked && m_bIsSomersaulting &&
		(m_blockCardinals.Left() && m_rightInput->IsPressed() ||
			m_blockCardinals.Right() && m_leftInput->IsPressed())) {
		m_upperAnimName = "Util/Blank";
		m_lowerAnimName = "Util/Blank";
		m_fullAnimName = "GariBall/GariBall";
		m_playerSprite_full->SetFlipHori(!m_bIsFacingRight);
		m_bCanWallJump = true;
		return true;
	}
	return false;
}
Task<> Player::RunTask(bool in_playAnims, float in_moveSpeed) {
	auto disp = Vec2f::Zero;
	auto dir = m_bIsFacingRight ? 1 : -1;
	if(in_playAnims) {
		m_fullAnimName = "Util/Blank";
		m_upperAnimName = "Gari_U/GariRun_U";
		m_lowerAnimName = "Gari_L/GariRun_L";
	}

	auto audioMgr = AudioManager::Get();
	auto elapsedTime = 0.0f;
	auto startX = GetWorldPos().x;
	auto prevFrameDisp = 0.0f;
	auto accelDisp = 0.0f;
	auto stillAccelerating = true;

	// Plays footstep sounds from time to time
	auto footstepSoundFunc = [this, prevStep = (int32_t)3, currentStep = (int32_t)0, timer = (float)0.0f, audioMgr]() mutable {
		if(timer >= ((15.0f * (1 / m_chargedSpeedModifier)) / 60.0f)) {
			std::string footstepFilename = "Footstep0" + std::to_string(currentStep);
			audioMgr->PlaySound(footstepFilename);
			prevStep = currentStep;
			currentStep += 1;
			if(currentStep > 4) {
				currentStep = 0;
			}
			timer = 0.0f;
		}
		timer += DT();
		return;
	};

	if(std::abs(m_lastFrameDisp.x) > 0.0f) {
		stillAccelerating = false;
	}
	while((m_leftInput->IsPressed() || m_rightInput->IsPressed() || (m_bRunningJump && !m_bRunningJumpInputTimerElapsed))) {
		elapsedTime += DT();
		auto speedModifiers = m_lavaSpeedModifier * m_chargedSpeedModifier;

		// Determine facing direction
		if(m_rightInput->IsPressed()) {
			m_bIsFacingRight = true;
			dir = 1;
			if(!m_bRunningJumpLateralInputReleased) {
				m_runningJumpInputTimer += DT();
			}
		}
		else if(m_leftInput->IsPressed()) {
			m_bIsFacingRight = false;
			dir = -1;
			if(!m_bRunningJumpLateralInputReleased) {
				m_runningJumpInputTimer += DT();
			}
		}
		else {
			m_bRunningJumpLateralInputReleased = true;
			m_runningJumpInputTimer -= DT();
		}
		if(m_runningJumpInputTimer <= 0.0) {
			m_bRunningJumpInputTimerElapsed = true;
		}

		// Setup anims
		m_playerSprite_full->SetPlayRate(1.0f * m_chargedSpeedModifier);
		m_playerSprite_upper->SetPlayRate(1.0f * m_chargedSpeedModifier);
		m_playerSprite_lower->SetPlayRate(1.0f * m_chargedSpeedModifier);
		m_playerSprite_upper->SetFlipHori(!m_bIsFacingRight);
		m_playerSprite_lower->SetFlipHori(!m_bIsFacingRight);
		m_playerSprite_full->SetFlipHori(!m_bIsFacingRight);

		// Setup movement data
		auto minSpeed = 0.15f * 60.0f;
		disp = Vec2f::Zero;

		// Grounded movement
		if(IsGrounded()) {
			m_bIsSomersaulting = false;
			if(!m_bIsBall) {
				footstepSoundFunc();
			}
			
			// Accelerate to full runSpeed (and clamp to minSpeed)
			auto currRunSpeed = std::clamp(GetWorld()->EaseAlpha(elapsedTime, k_accelDuration, Math::EaseOutSin) * in_moveSpeed * speedModifiers, minSpeed, in_moveSpeed);
			
			// Set player's y-pos to resulting value
			if(currRunSpeed < in_moveSpeed * speedModifiers && stillAccelerating) {
				disp = Move(Vec2f{ GetWorldPos().x + (currRunSpeed * DT() * dir), GetWorldPos().y, });
				accelDisp += disp.x;
			}
			// Or just go full speed
			else {
				stillAccelerating = false;
				disp = Move(Vec2f{ GetWorldPos().x + (in_moveSpeed * DT() * speedModifiers * dir), GetWorldPos().y });
			}
		}

		// Mid-air movement
		else {
			disp = Move(Vec2f{ GetWorldPos().x + (in_moveSpeed * DT() * dir), GetWorldPos().y });
		}

		m_bIsMovingLaterally = disp.x != 0 ? true : false;
		if(disp.x == 0 && !stillAccelerating) {
			break;
		}
		m_lastFrameDisp.x = disp.x;
		co_await Suspend();
	}
};
Task<> Player::ConditionalRunTask(bool in_playAnims, float in_moveSpeed) {
	while(true) {
		auto rightReleased = m_rightInput->JustReleased();
		auto leftReleased = m_leftInput->JustReleased();
		if((m_rightInput->IsPressed() && !m_blockCardinals.Right()) || (m_leftInput->IsPressed() && !m_blockCardinals.Left()) || (m_bRunningJump && !m_bRunningJumpInputTimerElapsed)) {
			co_await RunTask(in_playAnims, in_moveSpeed).CancelIf([this] {
				return (m_rightInput->JustReleased() || m_leftInput->JustReleased()) && m_bRunningJumpInputTimerElapsed;
				});
		}
		co_await Suspend();
	}
};
Task<> Player::ConditionalAimUpTask() {
	// Special thanks to Tim for help in organizing/refactoring this -- it was beyond my ken at the time we wrote it.
	std::string lastUpperAnimName;
	std::string lastLowerAnimName;
	std::string lastFullAnimName;
	while(true) {
		bool bIsMovingOnGround = IsGrounded() && std::abs(m_lastFrameDisp.x) != 0.0f;
		bool bCanLookUp = !bIsMovingOnGround && !m_bIsBall && !m_bCanSave;
		bool bWantsToLookUp = m_upInput->IsPressed();
		bool bCanLookDown = !IsGrounded() && !m_bIsBall && !m_bCanSave;
		bool bWantsToLookDown = m_downInput->IsPressed();

		bool bShouldLookUp = bCanLookUp && bWantsToLookUp;
		bool bShouldLookDown = bCanLookDown && bWantsToLookDown;
		bool bIsMidAir = IsMidAir();
		bool bIsSomersaulting = m_bIsSomersaulting & !m_bFiredInAir;
		enum class eAimState {
			Up = 0,
			Down = 1,
			Neutral = 2,
		};
		eAimState aimState = bShouldLookUp   ?	eAimState::Up :
							 bShouldLookDown ?	eAimState::Down :
												eAimState::Neutral;
		struct AimAnimsEntry {
			std::string anim;
			std::string midAirAnim;
			std::string somersaultingAnim;
		};
		struct AimAnims {
			AimAnimsEntry upper;
			AimAnimsEntry lower;
			AimAnimsEntry full;
		};
		auto UpdateAnim = [this, bIsMidAir, bIsSomersaulting](	const AimAnimsEntry& in_aimAnims, std::string in_animName, 
																std::string& out_lastAnimName, SpriteComponent* in_sprite, 
																bool in_bManualOverride) {
			if(!in_bManualOverride){
				auto animName = in_animName;
				if(in_aimAnims.anim != "") {
					animName = in_aimAnims.anim;
				}
				else if(in_aimAnims.midAirAnim != "" && bIsMidAir) {
					animName = in_aimAnims.midAirAnim;
				}
				else if(in_aimAnims.somersaultingAnim != "" && bIsSomersaulting) {
					animName = in_aimAnims.somersaultingAnim;
				}

				if(animName != out_lastAnimName) {
					in_sprite->PlayAnim(animName, true);
					out_lastAnimName = animName;
				}
			}
			else {
				out_lastAnimName = "";
			}
		};
		std::array<AimAnims, 3> aimAnimLut = {
			AimAnims{ "Gari_U/GariAimUp_U",		"Gari_U/GariAimUp_U",	"Util/Blank",	"", "Gari_L/GariFall_L",	"Util/Blank",	"", "Util/Blank",				""}, //< Aiming up
			AimAnims{ "",						"Util/Blank",			"Util/Blank",	"", "Util/Blank",			"Util/Blank",	"", "GariAimDown/GariAimDown",	""}, //< Aiming down
			AimAnims{ "",						"",						"Util/Blank",	"", "",						"Util/Blank",	"", "",							""}, //< Aiming neutral
		};
		const auto& aimAnims = aimAnimLut[int(aimState)];
		
		UpdateAnim(aimAnims.upper, m_upperAnimName, lastUpperAnimName, m_playerSprite_upper.get(), m_manualAnimUOverride);
		UpdateAnim(aimAnims.lower, m_lowerAnimName, lastLowerAnimName, m_playerSprite_lower.get(), m_manualAnimLOverride);
		UpdateAnim(aimAnims.full, m_fullAnimName, lastFullAnimName, m_playerSprite_full.get(), m_manualAnimFOverride);
		m_bIsAimingUp = aimState == eAimState::Up;
		m_bIsAimingDown = aimState == eAimState::Down;

		co_await Suspend();
	}
};
Task<> Player::TakeDamageTask() {
	auto elapsedTime = 0.0f;
	auto visibility = false;
	auto xImpulse = 2.0f * 60.0f;
	auto friction = 0.04f * 60.0f;
	while(true) {
		if(m_bHitReactRequest) {
			std::shared_ptr<Token> isInvincibleToken;
			auto damageFlags = m_hitDamageInfo.m_damageFlags;
			if(IsGrounded() && !(damageFlags & DF_NoKnockback)) {
				m_bBounceRequest = true;
			}
			if(damageFlags & DF_Enemy) {
				isInvincibleToken = m_bIsInvincible.TakeToken(__FUNCTION__);
			}

			auto isHitReactingToken = m_bIsHitReacting.TakeToken(__FUNCTION__);
			while(elapsedTime <= 0.83333f) {

				// Knockback
				if(m_hitDamageInfo.m_damageFlags & DF_Enemy | DF_Explodes) {
					Move({ GetWorldPos().x + (xImpulse * DT() * -m_hitDir), GetWorldPos().y });
					xImpulse = std::abs(xImpulse) - friction;
				}

				// Invincibility flicker
				if(m_hitDamageInfo.m_damageFlags & DF_Enemy) {
					SetHidden(visibility);
					visibility = !visibility;
				}
				co_await Suspend();
				elapsedTime += DT();
			}
			m_prevHitDamageInfo = m_hitDamageInfo;
			elapsedTime = 0.0f;
			xImpulse = 2.0f * 60.0f;
			SetHidden(false);
		}
		m_bHitReactRequest = false;
		m_bBounceRequest = false;
		if(!m_bIsInvincible && m_currentTouches) {
			//printf("follow-up hit!\n");
			if(auto instigator = m_prevInstigator.lock()) {
				m_prevHitDamageInfo.m_instigator = instigator;
				HandleDamage(m_prevHitDamageInfo);
				continue;
			}
			else {
				//printf(" -- attempted follow-up hit but couldn't find instigator!\n");
			}
		}
		co_await Suspend();
	}
}
Task<> Player::JumpForgivenessTask(float in_bufferWindow, float in_coyoteWindow) {
	auto coyoteWindow = in_coyoteWindow;
	auto bufferWindow = in_bufferWindow;
	while(true) {

		// Coyote time (grace time to jump AFTER walking off ledge)
		if(m_bCoyoteTime) {
			if(coyoteWindow <= 0.0f) {
				m_bCoyoteTime = false;
			}
			coyoteWindow -= DT();
		}

		// Jump buffer (grace time to buffer a jump BEFORE hitting the ground)
		if(!m_bCoyoteTime && m_jumpInput->JustPressed()) {
			m_bJumpBuffered = true;
		}
		if(m_jumpInput->IsPressed() && m_bJumpBuffered) {
			if(bufferWindow <= 0.0f) {
				m_bJumpBuffered = false;
			}
			bufferWindow -= DT();
		}
		if(m_jumpInput->JustReleased()) {
			m_bJumpBuffered = false;
			bufferWindow = in_bufferWindow;
		}
		co_await Suspend();
	}
}

// FIRING WEAPON CODE
void Player::UpdateFireHeuristics(FireHeuristics& in_heur) {
	in_heur.chargeUnlocked = GameWorld::Get()->GetPlayerStatus()->IsChargeUnlocked();
	in_heur.cooldownExpired = true; //m_fireCooldownTimer >= k_fireCooldown;
	in_heur.canCharge = in_heur.chargeUnlocked && !m_bIsCharged && !m_bAirJumping && !m_bChargeJumped;
	in_heur.canFireBullet = !m_bIsBall && !m_bIsFish && m_bullets.size() < 3 && !m_bIsInDoorTransition;
	in_heur.canDropBomb = m_bIsBall && m_bombs.size() < 3 && GameWorld::Get()->GetPlayerStatus()->IsBombUnlocked();
	in_heur.justChangedToBall = m_bIsBall && !m_prevBallState;
	in_heur.primaryFiring = m_primaryFireInput->JustPressed() && in_heur.canFireBullet && in_heur.cooldownExpired;
	in_heur.primaryReleaseFiring = m_primaryFireInput->JustReleased() && in_heur.canFireBullet;
	in_heur.primaryChargedFiring = m_bIsCharged && in_heur.primaryReleaseFiring;
	in_heur.dashReleaseFiring = m_bIsCharged && m_primaryFireInput->JustReleased() && m_bIsBall;
	in_heur.secondaryReleaseFiring = m_secondaryFireInput->JustReleased() && in_heur.canFireBullet; //&& in_heur.cooldownExpired;
	in_heur.secondaryChargedFiring = m_bIsCharged && in_heur.secondaryReleaseFiring;
	in_heur.regularFiring = in_heur.primaryFiring || in_heur.secondaryReleaseFiring;
	in_heur.chargedJumping = m_bIsCharged && m_bAirJumping && !m_bCanWallJump;
	in_heur.chargedFiring = in_heur.primaryChargedFiring || in_heur.secondaryChargedFiring;
	in_heur.facingDir = !m_bIsFacingRight ? -1 : 1;
	in_heur.justChangedDir = (m_bIsFacingRight && m_leftInput->JustPressed()) || (!m_bIsFacingRight && m_rightInput->JustPressed());
}
void Player::SetUpCharge(std::string in_palette) {
	m_playerSprite_effect->PlayAnim("ChargeEffect/Charged", true);
	Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 90.0f }, "ChargeActivate/Discharge", AsShared<Player>());
	m_playerSprite_effect->SetPalette(in_palette);
	m_bIsCharged = true;
	m_chargeElementsLive = 0;
	m_chargedSpeedModifier = 0.75f;
};
void Player::CleanUpCharge() {
	m_playerSprite_effect->PlayAnim("Util/Blank", false);
	m_dashReticleSprite->SetHidden(true);
	m_dashLineSprite->SetHidden(true);
	m_primaryFireInputTimer = 0.0f;
	m_secondaryFireInputTimer = 0.0f;
	m_bIsChargingPrimary = false;
	m_bIsChargingSecondary = false;
	m_chargedSpeedModifier = 1.0f;
	m_chargeElementsLive = 0;
	m_bIsCharged = false;
	GameWorld::Get()->GetAimReticleManager()->ClearOldReticles();
};
Task<> Player::ChargeWeaponTask() {
	while(true) {
		auto playerStatus = GetWorld()->GetPlayerStatus();
		// Check for charging-ness
		if(m_primaryFireInput->IsPressed() && m_fireHeur.canCharge) {
			m_primaryFireInputTimer += DT();
			m_secondaryFireInputTimer = 0.0f;
			auto chargeSegment = 1.0f / 6.0f;
			auto chargeNormalized = m_primaryFireInputTimer / k_chargeDelay;
			if(m_primaryFireInputTimer > k_minCharge && m_primaryFireInputTimer < k_chargeDelay) {
				if(chargeNormalized > chargeSegment && m_chargeElementsLive == 0) {
					Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f } }, "BeamCharger/RadialElement", AsShared<Player>());
					++m_chargeElementsLive;
				}
				else if(chargeNormalized > (chargeSegment * 2) && m_chargeElementsLive == 1) {
					Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 180.0f }, "BeamCharger/RadialElement", AsShared<Player>());
					++m_chargeElementsLive;
				}
				else if(chargeNormalized > (chargeSegment * 3) && m_chargeElementsLive == 2) {
					Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 90.0f }, "BeamCharger/RadialElement", AsShared<Player>());
					Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 0.0f }, "BeamCharger/RadialElement", AsShared<Player>());
					++m_chargeElementsLive;
				}
				else if(chargeNormalized > (chargeSegment * 4) && m_chargeElementsLive == 3) {
					Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 270.0f }, "BeamCharger/RadialElement", AsShared<Player>());
					Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 90.0f }, "BeamCharger/RadialElement", AsShared<Player>());
					++m_chargeElementsLive;
				}
				m_bIsChargingPrimary = true;
			}
		}
		else if(m_secondaryFireInput->IsPressed() && m_fireHeur.canCharge && !playerStatus->IsGrappleSelected()) {
			m_secondaryFireInputTimer += DT();
			m_primaryFireInputTimer = 0.0f;
			if(m_secondaryFireInputTimer < k_chargeDelay * 0.85f && !m_bIsChargingSecondary) {
				m_bIsChargingSecondary = true;
			}
			// Manually detonate the oldest live grenade
			if(m_grenades.size()) {
				m_grenades.front()->Detonate();
			}
		}

		// Show fully-charged effect, if applicable
		if(m_primaryFireInputTimer >= k_chargeDelay && !m_bIsCharged) {
			SetUpCharge("Base");
		}
		if(m_secondaryFireInputTimer >= k_chargeDelay && !m_bIsCharged) {
			//SetUpCharge("Secondary"); //< Disabled for now
		}

		// Turn off and clean up charged state in various cases
		if(m_primaryFireInput->JustReleased()) {
			if(m_bIsCharged) {
				m_fireHeur.primaryChargedFiring = true;
				Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 0.0f }, "ChargeActivate/ChargeActivate");
			}
			CleanUpCharge();
		}
		if(m_fireHeur.justChangedToBall) {
			CleanUpCharge();
		}
		if(!m_primaryFireInput->IsPressed() && !m_secondaryFireInput->IsPressed() && !m_bIsDashing) {
			CleanUpCharge();
		}
		if(m_fireHeur.chargedJumping) {
			CleanUpCharge();
			Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{ 0.0f, 7.0f }, 90.0f }, "ChargeActivate/Discharge");
		}
		co_await Suspend();
	}
}
Task<> Player::MissileTargetingTask() {
	auto retargetTimer = 0.0f;
	auto retargetTime = 1.0f / 8.0f;
	std::vector<std::shared_ptr<Creature>> rawTargets;
	std::vector<std::shared_ptr<Creature>> newTargets;
	std::vector<std::shared_ptr<Creature>> prevTargets;
	std::shared_ptr<Creature> mainTarget;
	std::shared_ptr<Creature> prevMainTarget;
	auto playerStatus = GetWorld()->GetPlayerStatus();
	auto hadMissileLastFrame = playerStatus->IsMissileSelected();
	while(true) {
		if(m_cyclePrimaryInput->JustPressed() && playerStatus->IsMissileSelected()){ // && !hadMissileLastFrame) {
			prevTargets.clear();
		}
		//Handle homing - missile - targeting behavior
		retargetTimer = std::min(retargetTimer - DT(), 0.0f);
		if(m_bIsCharged && playerStatus->IsMissileSelected() && !m_bIsBall) {
			newTargets = GetWorld()->GetOnCameraCreatures();
			if(newTargets.size()) {
				// Cull invincible creatures
				newTargets.erase(std::remove_if(newTargets.begin(), newTargets.end(), [](auto creature) {
					return creature->GetHealth() == -1;
					}), newTargets.end());
			}
			auto creatureCountChanged = newTargets.size() != prevTargets.size();
			if(newTargets.size() && (retargetTimer <= 0.0f || creatureCountChanged)) {
				// Reset retargetTimer
				retargetTimer = retargetTime;
				// Sort remaining creatures by distance from player (closest -> furthest)
				std::sort(newTargets.begin(), newTargets.end(), [this](std::shared_ptr<Creature> targetA,
					std::shared_ptr<Creature> targetB) {
						auto aDist = targetA->DistanceFromPlayer().Len();
						auto bDist = targetB->DistanceFromPlayer().Len();
						return aDist < bDist ? true : false;
					});
				prevTargets = newTargets;

				// Top up targets list with copies of other targets if it's smaller than missileCount:
				auto copyIndex = 0;
				for(auto i = newTargets.size(); i < k_missileCount; i++) {
					newTargets.push_back(newTargets[copyIndex]);
					copyIndex++;
				}
				m_liveTargets = newTargets;
			}
			auto mainTarget = m_liveTargets.size() ? m_liveTargets.front() : nullptr;
			if(mainTarget && mainTarget != prevMainTarget) {
				// If this is a new mainTarget, spawn a visible reticle on it
				if(!mainTarget->GetTargeted()) {
					auto center = m_liveTargets.front()->GetWorldPos();
					std::shared_ptr<GameActor> target = m_liveTargets.front();
					auto aimReticle = Spawn<AimReticle>(GetWorld(), { center }, target);
				}
				else {
					// Top up targets list with nullptrs if it's smaller than missileCount
					for(auto i = 0; i < k_missileCount; i++) {
						newTargets.push_back(nullptr);
					}
					m_liveTargets = newTargets;
				}
				prevMainTarget = mainTarget;
			}
		}
		hadMissileLastFrame = playerStatus->IsMissileSelected();
		co_await Suspend();
	}
}
void Player::SetUpGrenade() {
	// Set/Update Grenade charge level/magnitude, direction, and optional visualization:
	auto playerStatus = GetWorld()->GetPlayerStatus();
	if(m_bIsChargingSecondary && playerStatus->IsGrenadeSelected() && playerStatus->GetGrenadeCount()) {
		m_chargedSpeedModifier = 0.5f;
		auto grenadeAngle = 10.0f;
		if(m_upInput->IsPressed()) {
			grenadeAngle = 45.0f;
		}
		else if(m_downInput->IsPressed()) {
			if(IsGrounded()) {
				grenadeAngle = -10.0f;
			}
			else {
				grenadeAngle = -45.0f;
			}
		}
		m_grenadeDir = Vec2f{ 1.0f, 0.0f }.RotateDeg(grenadeAngle);

		// On secondaryFireInput release, magnitude and grenadeDir will be passed into the actual projectile spawn func
		m_grenadeDir.x = abs(m_grenadeDir.x) * m_fireHeur.facingDir;
		m_grenadeMagnitude = std::clamp(m_secondaryFireInputTimer / (k_chargeDelay / 3.0f), 0.1f, 1.0f);

		// Visualization code
		auto visualScalar = 27.0f;
		auto targetPoint = GetWorldPos() + m_grenadeDir * m_grenadeMagnitude * visualScalar;
		DrawDebugPoint(targetPoint, sf::Color::Magenta);
		DrawDebugLine(GetWorldPos(), targetPoint, sf::Color::Blue);
	}
}
Vec2f Player::SetUpDash() {
	// Set/Update Dash charge level/magnitude, direction, and optional visualization:
	auto dashVec = Vec2f::Zero;
	if((m_bIsChargingPrimary || m_bIsCharged) && m_bIsBall) {
		m_chargedSpeedModifier = 0.75f;
		Vec2f targetPoint = GetWorldPos();
		// Determine dashVec
		// - Up case
		if(m_upInput->IsPressed() && !(m_rightInput->IsPressed() || m_leftInput->IsPressed())) {
			dashVec = Vec2f::Up;
		}
		// - Down case
		else if(m_downInput->IsPressed() && !(m_rightInput->IsPressed() || m_leftInput->IsPressed())) {
			dashVec = Vec2f::Down;
		}
		// - Diag Up cases
		else if(m_upInput->IsPressed() && (m_rightInput->IsPressed() || m_leftInput->IsPressed())) {
			dashVec = Vec2f{ (float)m_fireHeur.facingDir, 1.0f };
		}
		// - Diag Down cases
		else if(m_downInput->IsPressed() && (m_rightInput->IsPressed() || m_leftInput->IsPressed())) {
			dashVec = Vec2f{ (float)m_fireHeur.facingDir, -1.0f };
		}
		// - Right/Left case
		else if((m_rightInput->IsPressed() || m_leftInput->IsPressed()) && !m_upInput->IsPressed() && !m_downInput->IsPressed()) {
			dashVec = Vec2f{ (float)m_fireHeur.facingDir, 0.0f };
		}

		auto magnitude = std::clamp(m_primaryFireInputTimer / k_chargeDelay, 0.0f, 1.0f);

		// Visualization code
		const auto dashChargeMultiplier = m_bIsCharged ? 6.0f : 0.0f;
		const auto dashMag = dashVec.Norm() * dashChargeMultiplier * 60.0f * k_dashDuration * magnitude;
		targetPoint = GetWorldPos() + dashMag + Vec2f{ 0.0f, 7.0f }; //< Offset to center on ball
		m_dashLineSprite->SetWorldPos(GetWorldPos() + Vec2f{ 0.0f, 7.0f });
		m_dashLineSprite->SetWorldRot(dashMag.SignedAngleDeg());
		m_dashReticleSprite->SetWorldPos(targetPoint);
		if(m_bIsCharged && dashVec != Vec2f::Zero && !m_bIsDashing) {
			m_dashReticleSprite->SetHidden(false);
			m_dashLineSprite->SetHidden(false);
		}
		else {
			m_dashReticleSprite->SetHidden(true);
			m_dashLineSprite->SetHidden(true);
		}
		// Debug visualization statements
		//DrawDebugPoint(targetPoint, sf::Color::Magenta);
		//DrawDebugLine(GetWorldPos() + Vec2f{0.0f, 7.0f}, targetPoint, sf::Color::Blue);
	}
	return dashVec;
}
Vec2f Player::PrepToFire(Vec2f& in_spawnPos, AnimOverrideData& in_overrideData) {
	// Set projectile spawn pos and show correct anims when firing projectile, return direction
	Vec2f bulletDirection;
	m_playerSprite_upper->SetPlayRate(1.0);
	// Aiming UP cases
	if(m_bIsAimingUp) {
		bulletDirection = { 0.0f, 1.0f };
		in_spawnPos = { GetWorldPos().x + 1.0f * m_fireHeur.facingDir, GetWorldPos().y + 20.0f };
		//manualAnimToken = m_manualAnimOverride.TakeToken(__FUNCTION__);
	}

	// Aiming DOWN cases
	else if(m_bIsAimingDown) {
		bulletDirection = { 0.0f, -1.0f };
		in_spawnPos = { GetWorldPos().x + 2.0f * m_fireHeur.facingDir, GetWorldPos().y - 7.0f };
		in_overrideData.fireAnimTaskU = m_playerSprite_upper->PlayAnim("Util/Blank", false);
		in_overrideData.manualAnimUToken = m_manualAnimUOverride.TakeToken(__FUNCTION__);
		in_overrideData.fireAnimTaskL = m_playerSprite_lower->PlayAnim("Util/Blank", false);
		in_overrideData.manualAnimLToken = m_manualAnimLOverride.TakeToken(__FUNCTION__);
		in_overrideData.fireAnimTaskF = m_playerSprite_full->PlayAnim("GariAimDown/GariFireDown", false);
		in_overrideData.manualAnimFToken = m_manualAnimFOverride.TakeToken(__FUNCTION__);
		m_bFiredInAir = true;
	}

	// Aiming LATERALLY cases
	else {
		bulletDirection = { 1.0f * m_fireHeur.facingDir, 0.0f };
		in_spawnPos = { GetWorldPos().x + 10.0f * m_fireHeur.facingDir, GetWorldPos().y + 6.0f };
		if(IsGrounded()) {
			in_overrideData.fireAnimTaskU = m_playerSprite_upper->PlayAnim("Gari_U/GariFire_U", false);
			in_overrideData.manualAnimUToken = m_manualAnimUOverride.TakeToken(__FUNCTION__);
			//fireAnimTaskF = m_playerSprite_full->PlayAnim("Util/Blank", false);
			//manualAnimFToken = m_manualAnimFOverride.TakeToken(__FUNCTION__);
		}
		else {
			in_overrideData.fireAnimTaskU = m_playerSprite_upper->PlayAnim("Gari_U/GariFireAir_U", false);
			in_overrideData.manualAnimUToken = m_manualAnimUOverride.TakeToken(__FUNCTION__);
			in_overrideData.fireAnimTaskL = m_playerSprite_lower->PlayAnim("Gari_L/GariFall_L", false);
			in_overrideData.manualAnimLToken = m_manualAnimLOverride.TakeToken(__FUNCTION__);
			in_overrideData.fireAnimTaskF = m_playerSprite_full->PlayAnim("Util/Blank", false);
			in_overrideData.manualAnimFToken = m_manualAnimFOverride.TakeToken(__FUNCTION__);
		}
		if(!IsGrounded()) {
			m_bFiredInAir = true;
		}
	}
	return bulletDirection;
}
Task<> Player::FireWeaponTask() {
	// This is the main weapon-firing function for Player
	auto elapsedTime = 0.0f;
	auto world = GetWorld();
	auto projectileMgr = world->GetProjectileManager();
	auto spawnPos = Vec2f::Zero;
	auto playerStatus = world->GetPlayerStatus();
	AnimOverrideData animOverrides;

	std::vector<ProjectileDef> projectileDefLookupTable = {
		// Short versions:
		g_playerBulletDef, //< 0
		g_playerBulletWaveDef, //< 1
		g_playerBulletIceDef, //< 2

		// Charged versions:
		g_playerBulletChargedDef, //< 3
		g_playerBulletWaveDef, //< 4 -- g_playerBulletWaveChargedDef doesn't yet exist
		g_playerBulletIceChargedDef, //< 5

		// Long versions:
		g_playerBulletLongDef, //< 6
		g_playerBulletLongWaveDef, //< 7
		g_playerBulletLongIceDef //< 8
	};

	while(true) {
		projectileMgr->Update();
		elapsedTime += DT();
		if(IsGrounded()) {
			m_bChargeJumped = false;
		}

		// Update heuristics
		UpdateFireHeuristics(m_fireHeur);

		// Set/Update Dash charge level/magnitude, direction, and optional visualization:
		if((m_bIsChargingPrimary || m_bIsCharged) && m_bIsBall) {
			m_dashVec = SetUpDash();
		}
		// Set/Update Grenade charge level/magnitude, direction, and optional visualization:
		m_grenadeMagnitude = 1.0f;
		if(m_bIsChargingSecondary && playerStatus->IsGrenadeSelected() && playerStatus->GetGrenadeCount()) {
			SetUpGrenade();
		}

		// Update palette if missile is selected (currently non-functional, pending art support)
		if(!m_bIsDashing) {
			if(playerStatus->IsMissileSelected() && playerStatus->GetMissileCount()) {
				m_playerSprite_upper->SetPalette("Missile");
				m_playerSprite_lower->SetPalette("Missile");
				m_playerSprite_full->SetPalette("Missile");
			}
			else {
				m_playerSprite_upper->SetPalette("Base");
				m_playerSprite_lower->SetPalette("Base");
				m_playerSprite_full->SetPalette("Base");
			}
		}

		// Handle weapon cycling
		if(m_cyclePrimaryInput->JustPressed()) {
			playerStatus->CyclePrimary();
			if(!playerStatus->IsMissileSelected()) {
				GameWorld::Get()->GetAimReticleManager()->ClearOldReticles();
			}
		}

		/* -----------------ACTUALLY FIRING ANY WEAPON BEGINS HERE----------------- */
		if((m_fireHeur.regularFiring || m_fireHeur.chargedFiring) && !m_bIsBall) { //m_fireHeur.primaryReleaseFiring) && !m_bIsBall) {
			auto bulletDirection = PrepToFire(spawnPos, animOverrides);

			Transform bulletTransform = { spawnPos, 0.0f, Vec2f::One };
			std::shared_ptr<Projectile> bullet;
			std::shared_ptr<Projectile> grenade;
			std::vector<std::shared_ptr<HomingMissile>> homingMissiles;

			// Fire a PRIMARY weapon
			if((m_fireHeur.primaryFiring || m_fireHeur.primaryChargedFiring)// || m_fireHeur.primaryReleaseFiring) 
				&& m_bullets.size() < 4 && !m_bIsBall) {
				// Missiles
				if(playerStatus->IsMissileSelected() && playerStatus->GetMissileCount()) {
					// Charged (i.e. homing)
					if(m_fireHeur.primaryChargedFiring) {
						playerStatus->TryRemoveMissile();
						auto arcLength = 90.0f;
						auto fireDir = Math::VecToDegrees(bulletDirection * -1.0f) - (arcLength / 2);
						AudioManager::Get()->PlaySound("MissileFireLg", 0.0f, 0.5f);
						for(int i = 0; i < k_missileCount; i++) {
							auto projectile = SpawnProjectile(g_playerHomingMissileDef, bulletTransform, bulletDirection, m_fireHeur.facingDir);
							if(auto homingMissile = std::dynamic_pointer_cast<HomingMissile>(projectile)) {
								SQUID_RUNTIME_CHECK(homingMissile != nullptr, "MissileDef must return a child class of HomingMissile");
								if(m_liveTargets.size() && m_liveTargets[i] != nullptr) {
									m_liveTargets[i]->SetTargeted(true);
									homingMissile->SetTarget(m_liveTargets[i]);
								}
								homingMissiles.push_back(homingMissile);
								fireDir += arcLength / (k_missileCount - 1 != 0 ? k_missileCount - 1 : 1);
							}
						}
						m_liveTargets.clear();
					}

					// Regular missiles
					else if(m_fireHeur.primaryFiring) {
						playerStatus->TryRemoveMissile();
						auto missile = SpawnProjectile(g_playerMissileDef, bulletTransform, bulletDirection, m_fireHeur.facingDir);
						m_bullets.push_back(missile);
						AudioManager::Get()->PlaySound("MissileFire");
						if(!playerStatus->GetMissileCount()) {
							playerStatus->SetPrimary(0);
						}
					}
				}
				else {
					// Beam weapons
					
					// Set to correct weapon type base index
					auto projectileDefIndex = 0;
					if(playerStatus->IsWaveBeamSelected()) {
						projectileDefIndex = 1;
					}
					else if(playerStatus->IsIceBeamSelected()) {
						projectileDefIndex = 2;
					}
					// Increment to correct def block
					if(m_fireHeur.primaryChargedFiring) {
						projectileDefIndex += 3;
					}
					else if(playerStatus->IsLongBeamUnlocked()) {
						projectileDefIndex += 6;
					}
					bullet = Actor::Spawn<Projectile>(world, bulletTransform, projectileDefLookupTable[projectileDefIndex], bulletDirection, m_fireHeur.facingDir);
					AudioManager::Get()->PlaySound("BeamFire", 0.0f, 0.8f);
				}
				m_fireHeur.primaryChargedFiring = false;
			}

			// ...or fire a secondary weapon
			else if((m_fireHeur.secondaryReleaseFiring || m_fireHeur.secondaryChargedFiring) && m_grenades.size() < 1) {
				// Grenade:
				if(playerStatus->IsGrenadeSelected() && playerStatus->GetGrenadeCount()) {
					if(m_fireHeur.secondaryChargedFiring) { //< Unused, currently
						//grenade = Actor::Spawn<Grenade>(world, bulletTransform, g_playerGrenadeDef, m_grenadeDir, m_fireHeur.facingDir, m_grenadeMagnitude);
					}
					else {
						playerStatus->TryRemoveGrenade();
						grenade = Actor::Spawn<Grenade>(world, bulletTransform, g_playerGrenadeDef, m_grenadeDir, m_fireHeur.facingDir, m_grenadeMagnitude);
						// If that was your last grenade, try to cycle away from the weapon automatically
						if(!playerStatus->GetGrenadeCount()) {
							playerStatus->CycleSecondary();
						}
					}
				}
				m_fireHeur.secondaryChargedFiring = false;
			}

			// Store new projectile in the correct list(s)
			if(bullet) {
				bullet->SetInstigator(AsShared<Player>());
				m_bullets.push_back(bullet);
			}
			if(grenade) {
				grenade->SetInstigator(AsShared<Player>());
				m_grenades.push_back(grenade);
			}
			else if(homingMissiles.size()) {
				for(auto missile : homingMissiles) {
					m_bullets.push_back(std::static_pointer_cast<Projectile>(missile));
				}
			}
			elapsedTime = 0.0f;
		}

		// ...or, istead of firing, execute the dash instead
		else if(m_bIsBall && m_fireHeur.dashReleaseFiring){
			m_bJustDashed = true;
			CleanUpCharge();
			m_fireHeur.primaryChargedFiring = true;
		}
		// Bomb-laying code (deprecated)
		//else if(canDropBomb && cooldownExpired && m_primaryFireInput->JustPressed()) {
		//	spawnPos = { GetWorldPos().x + 0.0f, GetWorldPos().y + 4.0f };
		//	Transform bombTransform = { spawnPos, 0.0f, Vec2f::One };
		//	auto bomb = Actor::Spawn<Bomb>(world, bombTransform);
		//	AudioManager::Get()->PlaySound("BallExit");
		//	m_bombs.push_back(bomb);
		//	auto elapsedTime = 0.0f;
		//}
		//std::cout << "UFOverride = " << m_manualAnimUOverride;
		//std::cout << "  | LOverride = " << m_manualAnimLOverride << std::endl;

		if(IsGrounded()) {
			animOverrides.fireAnimTaskL = {};
			animOverrides.manualAnimLToken = {};
			animOverrides.fireAnimTaskF = {};
			animOverrides.manualAnimFToken = {};
		}
		if((animOverrides.fireAnimTaskU && animOverrides.fireAnimTaskU.IsDone()) || (m_bFiredInAir && IsGrounded())) {
			m_playerSprite_upper->SetPlayRate(1.0);
			animOverrides.fireAnimTaskU = {};
			animOverrides.manualAnimUToken = {};
			animOverrides.fireAnimTaskL = {};
			animOverrides.manualAnimLToken = {};
			animOverrides.fireAnimTaskF = {};
			animOverrides.manualAnimFToken = {};
		}
		if(IsGrounded() && m_bFiredInAir) {
			m_bFiredInAir = false;
		}
		m_prevBallState = m_bIsBall;
		EraseInvalid(m_bullets);
		EraseInvalid(m_grenades);
		EraseInvalid(m_bombs);
		co_await Suspend();
	}
}
Task<> Player::GrappleTask() {
	auto probeTarget = Vec2f{ 100.0f, 100.0f };
	auto probeOffset = Vec2f{ 0.0f, 0.0f };
	auto probeSweepBox = Box2f::FromCenter(Vec2f::Zero, Vec2f::Zero);
	while(true) {
		auto grappleUnlocked = GetWorld()->GetPlayerStatus()->IsGrappleUnlocked();
		auto canGrapple = !IsGrounded() && grappleUnlocked;
		if(m_grappleInput->JustPressed() && canGrapple) {
			probeTarget.x = m_bIsFacingRight ? 100.0f : -100.0f;
			auto grappleProbe = Box2f::FromCenter(GetWorldPos() + probeOffset, { 2.0f, 2.0f });
			DrawDebugLine(GetWorldPos() + probeOffset, GetWorldPos() + probeTarget, sf::Color::Red);
			auto probeSweepResults = GetWorld()->GetCollisionWorld()->SweepBox(grappleProbe, probeTarget);
			if(probeSweepResults) {
				probeSweepBox = probeSweepResults.value().m_sweptBox;
				m_grappleAnchor = probeSweepResults.value().m_sweptBox.GetCenter();
				m_bGrappled = true;
			}
		}
		if(m_bGrappled && m_grappleInput->IsPressed()) {
			DrawDebugLine(GetWorldPos() + probeOffset, probeSweepBox.GetCenter(), sf::Color::White);
			DrawDebugBox(probeSweepBox, sf::Color::Blue);
		}
		if(m_grappleInput->JustReleased()) {
			m_bGrappled = false;
		}
		co_await Suspend();
	}
}
Task<> Player::UseSavePointTask() {
	while(true) {
		if(m_bCanSave && IsGrounded() && m_upInput->JustPressed()) {
			auto world = GetWorld();
			auto status = world->GetPlayerStatus();
			auto manualUAnimToken = m_manualAnimUOverride.TakeToken(__FUNCTION__);
			auto manualLAnimToken = m_manualAnimLOverride.TakeToken(__FUNCTION__);
			auto manualFAnimToken = m_manualAnimFOverride.TakeToken(__FUNCTION__);
			m_playerSprite_full->PlayAnim("GariAppear/GariAppear", false);
			m_playerSprite_lower->PlayAnim("Util/Blank", false);
			m_playerSprite_upper->PlayAnim("Util/Blank", false);
			status->SaveRecharge();
			co_await world->OnItemPickup(std::pair<std::wstring, std::wstring> {L"GAME SAVED", L"Life and Supplies Replenished"}); //< Game pause & delay/musical interlude happens in here
			status->SetSpawnPos(world->GetPlayerWorldPos());
			m_bCanSave = false;
		}
		co_await Suspend();
	}
}
Task<> Player::PauseGameTask() {
	while(true) {
		if(m_startInput->JustPressed()) {
			printf("PLAYER: Pausing!\n");
			GetWorld()->OnPause();
		}
		co_await Suspend();
	}
}
Task<> Player::TestInputListenerTask() {
	auto projectileMgr = GetWorld()->GetProjectileManager();
	while(true) {
		if(m_testInput->JustPressed()) {
			std::cout << "playerpos = " << GetWorldPos().x << ", " << GetWorldPos().y << std::endl;
		}
		co_await Suspend();
	}
};
Task<> ToggleDebug(std::shared_ptr<Player> in_player) {
	while(true) {
		if(GameLoop::ShouldDisplayDebug()) {
			//DrawDebugBox(in_player->GetCollisionBoxLocal(), sf::Color::White, in_player->GetWorldTransform(), (-1.0f));
			//DrawDebugCircle(shape.origin, shape.radius, sf::Color::White, in_player->GetWorldTransform(), (-1.0f));
		}
		co_await Suspend();
	}
}
Task<> Player::ManageActor() {
	auto testInputListener = m_taskMgr.Run(TestInputListenerTask());
	auto pauseGameTask = m_taskMgr.Run(PauseGameTask());
	auto takeDamageTask = m_taskMgr.Run(TakeDamageTask());
	auto fsmTask = m_taskMgr.Run(ManagePlayerFSM()); //< The main player-movement FSM task
	auto lavaTask = m_taskMgr.Run(WhileTouchingLava());
	auto aimUpTask = m_taskMgr.Run(ConditionalAimUpTask()); //< Handles vertical aiming
	auto fireTask = m_taskMgr.Run(FireWeaponTask()); //< Handles weapon firing
	auto chargeTask = m_taskMgr.Run(ChargeWeaponTask()); //< Handles weapon charging
	auto missileTargetTask = m_taskMgr.Run(MissileTargetingTask());
	auto grappleTask = m_taskMgr.Run(GrappleTask());
	auto saveTask = m_taskMgr.Run(UseSavePointTask());
	auto toggleDebug = m_taskMgr.Run(ToggleDebug(AsShared<Player>()));
	co_await WaitForAny({
		fsmTask,
	});
}
