#pragma once

#include "Character.h"
#include "TokenList.h"
#include "GameEnums.h"

class Projectile;
class InputComponent;
class Pickup;
class Bomb;
class Grenade;
class Lava;
struct ButtonState;
struct PlayerButtonState;

class Creature;
struct PlayerStatus;
class Light;

// Main player character class (full movement & abilities controller, including weapon charging/firing)
class Player : public Character {
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual void Update() override;
	virtual Task<> ManageActor() override;

	Vec2f GetTargetPos();
	bool GetDashingCharged() const { 
		return (m_bIsCharged && m_bIsDashing); 
	}
	bool IsFacingRight() { return m_bIsFacingRight; }
	void SetLavaTouch(bool in_state) { m_bIsTouchingLava = in_state; }
	void LaserTouch(std::shared_ptr<Creature> in_creature, const Vec2f& in_zapPos);
	void SetDoorTransition(bool in_state) { m_bIsInDoorTransition = in_state; }
	void SetSuitTransition(bool in_state) { m_bIsInSuitTransition = in_state; }
	void SetControlsEnabled(bool in_enabled);
	void SetGravityModifier(float in_gravityMod) { m_gravityModifier = in_gravityMod; }
	bool IsGrounded() const;
	bool IsFish() const { return m_bIsFish; }
	void SetDirection(int32_t in_xDir = 1) { m_bIsFacingRight = (in_xDir > 0) ? true : false; }

private:
	struct FireHeuristics;
	struct AnimOverrideData;
	// Sensor event handlers
	void OnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor);
	void OnUnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor);
	void OnTouchLava(std::shared_ptr<Lava> in_lava);
	Task<> WhileTouchingLava();
	void OnUnTouchLava();
	void OnTouchBullet(std::shared_ptr<Projectile> in_bullet, std::shared_ptr<SensorComponent> in_sensor);
	std::optional<DamageInfo> ModifyDamage(const DamageInfo& in_dmgInfo);
	eHitResponse HandleDamage(const DamageInfo& in_dmgInfo);
	void OnTouchGrenade(std::shared_ptr<Grenade> in_grenade, std::shared_ptr<SensorComponent> in_sensor);

	void GravityCheck();
	void UpdateFireHeuristics(FireHeuristics& in_heur);
	void CleanUpCharge();
	void SetUpCharge(std::string in_palette);
	void SetUpGrenade();
	Vec2f SetUpDash();
	Vec2f PrepToFire(Vec2f& in_spawnPos, AnimOverrideData& in_overrideData);
	Task<> ManagePlayerFSM();
	
	// States
	Task<> StandState();
	Task<> IdleState();
	Task<> JumpState();
	Task<> FallState(bool in_bIsTransitioningFromBall);
	Task<> MoveState();
	Task<> BallState();
	Task<> BallFallState(bool in_bIsTransitioningToBall);
	Task<> BounceState();
	Task<> DashState();
	Task<> GrappleState();
	Task<> FishIdleState();
	Task<> FishJumpState(bool in_isEjecting = false);
	Task<> FishFallState();
	Task<> FishSuitTransitioningState();
	
	// State transition funcs
	bool ShouldEnterJumpState();
	bool ShouldEnterMoveState(bool in_transitioningFromFall = false);
	bool ShouldEnterIdleState(bool in_transitioningFromFall = false);
	bool ShouldEnterFallState();
	bool ShouldEnterGrappleState();
	bool ShouldEnterBallFallState();
	bool IsMidAir() const { return m_bIsJumping || m_bIsFalling; }
	bool ShouldBounce();
	bool ShouldLeaveBallState();
	bool ShouldEnterDashState();
	bool ShouldEnterFishJumpState();
	bool ShouldEnterFishSuitTransitioningState();
	
	// Resizes (and relocates) player's sensors and collision boxes during certain state changes
	void ResizePlayer(std::string in_profile = "full", float in_cardinalDistCheck = 0.0f, float in_verticalMove = 0.0f);

	// Handles walljump attempts, returns success or failure
	bool TryWallJump(float in_threshold);
	
	// Parallel Tasks
	Task<> ConditionalAimUpTask();
	Task<> TakeDamageTask();
	Task<> ChargeWeaponTask();
	Task<> MissileTargetingTask();

	// Handles run movement/sounds (and animations if applicable)
	Task<> RunTask(bool in_playAnims = true, float in_moveSpeed = 1.5f);

	// Acts as a conditional filter to the run state (evaluates input state, various flags)
	Task<> ConditionalRunTask(bool in_playAnims = true, float in_moveSpeed = 1.5f);

	// Evaluates all firing state (weapon selection, inputs, state heuristics) and fires something when relevant
	Task<> FireWeaponTask();

	// Enables coyote time and/or jump buffering
	Task<> JumpForgivenessTask(float in_bufferWindow = 0.1333f, float in_coyoteWindow = 0.1f);

	// Listens for grapple inputs, and handles "grappled" state setup
	Task<> GrappleTask();

	Task<> UseSavePointTask();
	Task<> PauseGameTask();
	Task<> TestInputListenerTask();

	// Sensor data
	std::shared_ptr<SensorComponent> m_creatureSensor;
	std::shared_ptr<SensorComponent> m_bulletSensor;
	std::shared_ptr<SensorComponent> m_objectSensor;
	int32_t m_currentTouches = 0;
	std::shared_ptr<Lava> m_touchingLava;

	// Live projectile lists
	std::vector<std::shared_ptr<Projectile>> m_bullets;
	std::vector<std::shared_ptr<Projectile>> m_grenades;
	std::vector<std::shared_ptr<Bomb>> m_bombs;

	// Targeted creature list
	std::vector<std::shared_ptr<Creature>> m_liveTargets;

	// Tokens
	TokenList<> m_bIsInvincible;
	TokenList<> m_bIsHitReacting;
	TokenList<> m_bIsDashing;
	TokenList<> m_bIsJumping;
	TokenList<> m_bIsFalling;
	TokenList<> m_manualAnimUOverride;
	TokenList<> m_manualAnimFOverride;
	TokenList<> m_manualAnimLOverride;

	// Damage Data
	int32_t m_hitDir = 1;
	DamageInfo m_hitDamageInfo;
	DamageInfo m_prevHitDamageInfo;
	int32_t m_prevDamagePayload = 0;
	std::weak_ptr<GameActor> m_prevInstigator;
	int32_t m_defaultDamagePayload = 2;
	bool m_bHitReactRequest = false;

	// Flags
	bool m_bIsBall = false;
	bool m_prevBallState = false;
	bool m_bBounceRequest = false;
	bool m_bBouncedStanding = false;
	bool m_bIsFacingRight = true;
	bool m_bIsAimingUp = false;
	bool m_bIsAimingDown = false;
	bool m_bIsMovingLaterally = false;
	bool m_bIsBouncing = false;
	bool m_bRunningJump = false;
	bool m_bIsSomersaulting = false;
	bool m_bRunningJumpInputTimerElapsed = true;
	bool m_bRunningJumpLateralInputReleased = true;
	bool m_bJumpBuffered = false;
	bool m_bCoyoteTime = true;
	bool m_bFallingFromLedge = false;
	bool m_bAirJumping = false;
	bool m_bGrappled = false;
	bool m_bJustDashed = false;
	bool m_bCanWallJump = false;
	bool m_bChargeJumped = false;
	bool m_bJumpEnded = false;
	bool m_bFiredInAir = true;
	bool m_bIsCharged = false;
	bool m_bIsChargingPrimary = false;
	bool m_bIsChargingSecondary = false;
	bool m_bIsTouchingLava = false;
	bool m_bIsInDoorTransition = false;
	bool m_bIsInSuitTransition = false;
	bool m_bCanSave = false;

	struct FireHeuristics {
		bool chargeUnlocked = false;
		bool cooldownExpired = false;
		bool canCharge = false;
		bool canFireBullet = false;
		bool canDropBomb = false;
		bool justChangedToBall = false;
		bool primaryFiring = false;
		bool primaryReleaseFiring = false;
		bool primaryChargedFiring = false;
		bool dashReleaseFiring = false;
		bool secondaryReleaseFiring = false;
		bool secondaryChargedFiring = false;
		bool regularFiring = false;
		bool chargedJumping = false;
		bool chargedFiring = false;
		int32_t facingDir = 1;
		bool justChangedDir = false;
	};
	FireHeuristics m_fireHeur;

	// Fish flags
	bool m_bIsFish = false;
	bool m_bForceEject = false;
	bool m_bEjectEnabled = true;

	// Anim names
	std::string m_fullAnimName;
	std::string m_upperAnimName;
	std::string m_lowerAnimName;

	struct AnimOverrideData {
		std::shared_ptr<Token> manualAnimUToken;
		std::shared_ptr<Token> manualAnimFToken;
		std::shared_ptr<Token> manualAnimLToken;
		TaskHandle<> fireAnimTaskU;
		TaskHandle<> fireAnimTaskL;
		TaskHandle<> fireAnimTaskF;
	};

	// Constants
	const float k_runSpeed = 1.5f * 60.0f;
	const float k_standingJumpRunSpeed = 1.0f * 60.0f;
	const float k_runningJumpRunSpeed = 1.1f * 60.0f;
	const float k_accelDuration = 0.15f;
	const float k_jumpDuration = 0.7f;
	const float k_jumpHeight = 82.0f;
	const float k_jumpGravity = 0.092f * 60.0f;
	const float k_lavaGravity = 1.6f * 60.0f;
	float m_gravityModifier = 1.0f;
	const float k_jumpImpulse = 3.84f * 60.0f;
	const float k_bounceScalar = 0.6f;
	const float k_dashDuration = 0.225f;

	// Weapon constants
	const float k_fireCooldown = 0.05f;
	const float k_chargeDelay = 1.35f;
	const float k_minCharge = 0.2f;
	int32_t k_missileCount = 1; //< How many are fired when a full charge is released
	
	// Weapon data
	float m_primaryFireInputTimer = 0.0f;
	float m_secondaryFireInputTimer = 0.0f;
	float m_fireCooldownTimer = 0.0f;
	int32_t m_chargeElementsLive = 0;
	float m_grenadeMagnitude = 1.0f;
	Vec2f m_grenadeDir = Vec2f{ 1.0f, 0.0f }.RotateDeg(10.0f);

	// Movement data
	Vec2f m_dashVec = Vec2f::Zero;
	float m_chargedSpeedModifier = 1.0f;
	float m_lavaSpeedModifier = 1.0f;
	float m_lavaJumpModifier = 1.0f;
	bool m_bCanAccumulateJump = false;
	Vec2f m_lastFramePos = GetWorldPos();
	Vec2f m_lastFrameDisp = Vec2f::Zero;
	float m_jumpPeak = 0.0f;
	float m_bouncePeak = 0.0f;
	float m_runningJumpInputTimer = 0.0f;
	Vec2f m_grappleAnchor;
	float m_cornerForgiveness = 7.0f;

	// Sprites
	std::shared_ptr<SpriteComponent> m_playerSprite_full;
	std::shared_ptr<SpriteComponent> m_playerSprite_upper;
	std::shared_ptr<SpriteComponent> m_playerSprite_lower;
	std::shared_ptr<SpriteComponent> m_playerSprite_effect;
	std::shared_ptr<SpriteComponent> m_dashReticleSprite;
	std::shared_ptr<SpriteComponent> m_dashLineSprite;

	// Lights
	std::shared_ptr<Light> m_light;

	// Input components & button states
	std::shared_ptr<InputComponent> m_inputComp;
	std::shared_ptr<PlayerButtonState> m_rightInput;
	std::shared_ptr<PlayerButtonState> m_leftInput;
	std::shared_ptr<PlayerButtonState> m_upInput;
	std::shared_ptr<PlayerButtonState> m_downInput;
	std::shared_ptr<PlayerButtonState> m_jumpInput;
	std::shared_ptr<PlayerButtonState> m_primaryFireInput;
	std::shared_ptr<PlayerButtonState> m_secondaryFireInput;
	std::shared_ptr<PlayerButtonState> m_grappleInput;
	std::shared_ptr<PlayerButtonState> m_ejectInput;
	std::shared_ptr<PlayerButtonState> m_testInput;
	std::shared_ptr<PlayerButtonState> m_startInput;
	std::shared_ptr<PlayerButtonState> m_cyclePrimaryInput;
	std::shared_ptr<PlayerButtonState> m_cycleSecondaryInput;

	// Metrics
	int32_t m_qolBenefit = 0;
};
