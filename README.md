# LOST & FOUND
### A 2D action platformer prototype written from scratch with C++14 and C++ coroutines.

![](https://github.com/Eastquote/LostAndFound/blob/main/data/docs/Gameplay01.gif)

### [--> You can download and play a compiled binary HERE! <--](https://mikeprimo.itch.io/lost-and-found)

This game makes extensive use of Giant Squid's open-source [C++ coroutine implementation](https://github.com/westquote/SquidTasks), which can be found in [middleware/SquidTasks](https://github.com/Eastquote/LostAndFound/tree/main/middleware/SquidTasks). If you intend to read my code and aren't familiar with coroutines already, you should probably watch this video explaining one of my other coroutine projects before you dig in:

[![Intro To C++ Coroutines for Game Development](https://img.youtube.com/vi/HrKnT5MHEoY/0.jpg)](https://www.youtube.com/watch?v=HrKnT5MHEoY?t=24)

## FRAMEWORK NOTE
This project uses a lightweight "learning" framework (located in the *[src/Engine](https://github.com/Eastquote/LostAndFound/tree/main/src/Engine)* folder) that was written on top of [SFML](https://www.sfml-dev.org/) by [Tim Ambrogi](https://github.com/westquote) *(Giant Squid)* with help from [Drew Wallace](http://www.drewwallacegames.com/) *(Naughty Dog)*. It was designed to provide only what was necessary to learn to build a proper game, and mostly comprises:

- an [Object](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/Object.h) base class with child object storage and Initialize() and Destroy() functions -- these get called after the constructor runs and before the destructor runs, respectively. (This should be a familiar paradigm for Unreal devs.)
- Various useful Components for [Colliders](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/Components/ColliderComponent.h), [Text](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/Components/TextComponent.h), [Sprites](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/Components/SpriteComponent.h), [Tiles](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/Components/TilesComponent.h), etc.
- an [Actor](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/Actor.h) base class that includes Transforms and Component factories.
- a Box2D integration (unused).
- [MathGeometry](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/MathGeometry.h), a collection of low-level geometry functions that I used (along with the [SweepBox()](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/CollisionWorld.h#L29) function) to implement the game's physics.
- Various other useful Math functions ([MathCore](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/MathCore.h), [MathRandom](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/MathRandom.h), [MathEasings](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/MathEasings.h), etc.)
- [TileMap](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/TileMap.h), a class that can load/parse a [Tiled](https://www.mapeditor.org/) map file and make it available for game logic and rendering purposes.
- [GameWindow](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/GameWindow.h), a wrapper for SFML functionality that configures the view into the game that gets rendered.
- [InputSystem](https://github.com/Eastquote/LostAndFound/blob/main/src/Engine/InputSystem.h), another SFML wrapper which captures and interprets low-level input from mouse/keyboard/controllers.

---
<br>
Because I was actively learning C++ as I wrote this game, the quality and sophistication of the code varies depending on when in the development cycle it was written. That said, I feel that the following samples accurately represent my current skill level:

## CODE SAMPLE 1 : [PLAYER FSM](https://github.com/Eastquote/LostAndFound/blob/main/src/Player.cpp#L345)
![](https://github.com/Eastquote/LostAndFound/blob/main/data/docs/FSMHeader.jpg)<br>
The Player controller class employs a Finite State Machine (FSM) implementation that comes with the SquidTasks library mentioned above. It allows me to specify:
- all of my player character's possible movement **States** (e.g. JumpState(), FallState(), etc.)
- a prioritized network of **StateLinks** between them (aka **"Exits"**)
- the **predicate functions** for determining when transitions can occur (e.g ShouldEnterJumpState(), IsGrounded(), etc.)

*Lost & Found* has a fairly complex character controller, so this is what the FSM setup looks like:
```cpp
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
```

Let's go over a simple transition from the IdleState() to the JumpState(). Here is the IdleState() coroutine:

```cpp
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
	while(true) {
		if(m_bIsTouchingLava) { //< Suck the player down into lava (n.b this is NOT FallState())
			Move({ GetWorldPos().x, GetWorldPos().y - k_lavaGravity * DT() });
		}
		co_await Suspend();
	}
}
```

Every tick, the FSM will determine whether to leave the current state by sequentially evaluating all of that state's "Exit" Link predicates. In the case of IdleState(), these are:

```cpp
// Idle Exits
	fsm.StateLinks(idleState, { //< These link predicates are sequentially evaluated until one of them returns True
		fishTransitioningState.Link([this] { return ShouldEnterFishJumpState(); }),
		jumpState.Link([this] { return ShouldEnterJumpState(); }), //< This is the link this example is focused on
		fallState.Link([this] { return ShouldEnterFallState(); }),
		moveState.Link([this] { return ShouldEnterMoveState(false); }),
		ballFallTransitioningState.Link([this] { return ShouldEnterBallFallState(); }),
		bounceState.Link([this] { return ShouldBounce(); }),
	});
```
First on the list is the fishTransitioningState Link, which calls ShouldEnterFishJumpState() and will return false in this case.<br>
The FSM will then proceed to the next Link on the list (the jumpState link) and call its predicate function: ShouldEnterJumpState().

```cpp
bool Player::ShouldEnterJumpState() {
	auto canJump = IsGrounded() || m_bCoyoteTime || m_bCanWallJump || (!IsGrounded() && m_bIsCharged);
	auto justJumped = m_jumpInput->JustPressed() || m_bJumpBuffered;
	if(canJump && justJumped) {
		return true;
	}
	return false;
}
```

That ***canJump*** variable reports if any of the (many) situations that allow the player to jump is currently true. The ***justJumped*** variable reports whether a jump is actually desired at this time.<br>
If both come back true, then ShouldEnterJumpState() returns true and the FSM will dutifully create and resume the JumpState() coroutine:

```cpp
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

	// Main Jump Loop
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
```

Certain affordances of the player controller, like using weapons or aiming vertically, can be thought of as orthogonal states that run in parallel to the main player-movement FSM. You can see these parallel tasks being added to the Player's TaskManager here in the ManageActor() task:

```cpp
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
```

All told, this formalized FSM approach made it possible to largely avoid the spaghetti code that wants to erupt out of complex systems like a player controller, and helped me to write nice plump ravioli code instead!  :)

## CODE SAMPLE 2 : [DIALOGUE SYSTEM](https://github.com/Eastquote/LostAndFound/blob/main/src/Dialogue.h)
![](https://github.com/Eastquote/LostAndFound/blob/main/data/docs/DialogueHeader.jpg)<br>
For this game I settled on a Dialogue system that makes use of 3 main objects: [Scenes](https://github.com/Eastquote/LostAndFound/blob/main/src/GameLoop.cpp#L333) (line data), [DialogueSpeakers](https://github.com/Eastquote/LostAndFound/blob/main/src/Dialogue.h#L40) (character data), and [Dialogues](https://github.com/Eastquote/LostAndFound/blob/main/src/Dialogue.h#L76) (machines for executing conversations between Speakers, using Scene data.)

This approach allowed me to compartmentalize the complexity of a multi-character dramatic scene with good maintainability -- when I had to quickly re-write the [teletyping line parser](https://github.com/Eastquote/LostAndFound/blob/main/src/Dialogue.cpp#L138) for a public demo, no data format changes were necessary and all the relevant implementation lived inside the PlayLine() function:

```cpp
  Task<> DialogueSpeaker::PlayLine(const SceneLine& in_line) {
	// Create necessary locals
	std::shared_ptr<ButtonState> advanceButton = m_instigator->GetAdvanceButton();
	m_textHeading->SetText(m_def->m_name);
	std::wstring fullText = in_line.second;
	std::wstring displayText1;
	std::wstring displayText2;
	std::wstring textLine1;
	std::wstring textLine2;
	int64_t maxWidth = 40;

	// If it's 2 lines and doesn't break cleanly right on the maxWidth, use the closest previous break
	if((int64_t)fullText.size() > maxWidth && (fullText[maxWidth] != L' ' && fullText[(int64_t)maxWidth - 1] != L' ')) {
		int64_t breakIndex;
		for(auto i = maxWidth - 2; i >= 0; --i) {
			if(fullText[i] == L' ') {
				breakIndex = i;
				break;
			}
		}
		textLine1 = fullText.substr(0, breakIndex + 1);
		textLine2 = fullText.substr(breakIndex + 1, std::string::npos);
	}
	else if((int64_t)fullText.size() > maxWidth) { //< Otherwise you can just break it clean!
		textLine1 = fullText.substr(0, maxWidth);
		textLine2 = fullText.substr(maxWidth, std::string::npos);
	}
	else{ //< Or maybe it's just one line
		textLine1 = fullText;
	}

	// If it IS 2 lines, pop off any leading spaces from line 2
	while(textLine2.size() && textLine2[0] == L' ') {
		textLine2 = textLine2.substr(1, std::string::npos);
	}
	
	// Okay, let's teletype!
	co_await TeletypeLine(displayText1, textLine1, advanceButton);
	if(textLine2.size()) { // Line 2, if applicable
		co_await TeletypeLine(displayText2, textLine2, advanceButton, false);
	}

	// Manage AdvanceButton visibility
	auto showAdvanceButtonTask = m_taskMgr.RunManaged(ShowAdvancePrompt());
	co_await WaitUntil([advanceButton] {
		return advanceButton->JustPressed();
		});
	auto hideAdvanceButtonTask = m_taskMgr.RunManaged(HideAdvancePrompt());

	// Cleanup
	m_dialogueLine1->SetText(L"");
	m_dialogueLine2->SetText(L"");
}
Task<> DialogueSpeaker::TeletypeLine(std::wstring& in_text, std::wstring& in_line, std::shared_ptr<ButtonState> in_buttonState,
                                     bool in_firstLine) {
	for(auto i = 0; i < in_line.size(); ++i) {
		float teletypeDelay = 0.05f;
		std::shared_ptr<AudioManager> audioMgr = AudioManager::Get();
		auto displayLine = in_firstLine ? m_dialogueLine1 : m_dialogueLine2;

		in_text += in_line[i]; //< Add the next character of the full line
		displayLine->SetText(in_text);
		if(i % 2 == 0) { //< Play sound every other frame
			audioMgr->PlaySound("Teletype", 0.05f);
		}
		if(float delay = in_buttonState->IsPressed() ? 0.0f : teletypeDelay) { //< Lets players Lets players speed it up
			co_await WaitSeconds(delay);
		}
		co_await Suspend();
	}
}
```

(A notable blemish on this code is the fact that SpeakerDef and SceneDef data are still [populated in hardcode](https://github.com/Eastquote/LostAndFound/blob/main/src/Dialogue.cpp#L17) -- best practice would be for them to be read in from JSON files or similar, so other team members could create/edit them without touching the codebase directly. As I was solo-ing this project with a lot of other features on my plate, I deprioritized this task, but it would be Job #1 if I were to return to this system for further improvements.)

## CODE SAMPLE 3 : [LIGHT SYSTEM](https://github.com/Eastquote/LostAndFound/blob/main/src/Light.h)
![](https://github.com/Eastquote/LostAndFound/blob/main/data/docs/LightHeader.jpg)<br>
This circular Light actor has potential to used in a wide variety of contexts, so it was important to give it exactly the right set of features to do its job as elegantly as possible. 

Of particular note is a nice bit of coroutine business in the [InterpScale()](https://github.com/Eastquote/LostAndFound/blob/main/src/Light.cpp#L28) and [ManageInterpScale()](https://github.com/Eastquote/LostAndFound/blob/main/src/Light.cpp#L32) functions, which allow me to change the size of a given Light actor over a specified duration in seconds:

```cpp
TaskHandle<> Light::InterpScale(float in_targetScale, float in_duration) {
	// Interpolate entire Light actor to target scale over the specified duration (in seconds)
	m_scaleTask = m_taskMgr.Run(ManageInterpScale(in_targetScale, in_duration)); //< Dispatch task to TaskManager
	return m_scaleTask;
}
Task<> Light::ManageInterpScale(float in_targetScale, float in_duration) {
	co_await WaitForAll({ //< Guarantees that both LightElements are done scaling before proceeding
		m_core->InterpScale(in_targetScale, in_duration), //< The light's bright center
		m_penumbra->InterpScale(in_targetScale, in_duration), //< The dim "falloff" halo around the center
	});
}
```

InterpScale() actually invokes ManageInterpScale() as a sub-task, a handle to which is then added to the Light Actor's m_taskMgr list -- this means the ManageInterpScale() sub-task survives the collapse of the InterpScale() stack frame, so you can "fire and forget" InterpScale() from anywhere. Even though it may be configured to take many seconds or even minutes to finish executing, you can trust that it'll do so safely.

For its part, ManageInterpScale() passes the targetScale and duration parameters to both the m_core and m_penumbra LightElements of the Light actor, and co_awaits their scale interpolations to completion. The LightElements themselves employ this same pattern for their respective [InterpScale()](https://github.com/Eastquote/LostAndFound/blob/main/src/LightElement.cpp#L23) tasks, and implement the scale interpolation itself:

```cpp
TaskHandle<> LightElement::InterpScale(float in_targetScale, float in_duration) {
	// Interp this particular LightElement's uniform scale to the target value, over the specified duration
	m_scaleTask = m_taskMgr.Run(ManageInterpScale(in_targetScale, in_duration)); //< Dispatch task to TaskManager
	return m_scaleTask;
}
Task<> LightElement::ManageInterpScale(float in_targetScale, float in_duration) {
	std::shared_ptr<GameWorld> world = GetWorld();
	float startScale = m_sprite->GetRelativeScale().x;
	float scaleDelta = in_targetScale - startScale;
	float elapsedTime = 0.0f;
	float interpAlpha = 0.0f;
	while(elapsedTime < in_duration) {
		elapsedTime += DT();
		// Calculate this frame's location on a smooth, normalized interpolation curve
		interpAlpha = std::clamp(world->EaseAlpha(elapsedTime, in_duration, Math::EaseInOutSmootherstep), 0.0f, 1.0f);
		// Set new scale for this frame
		float newScale = startScale + (interpAlpha * scaleDelta);
		m_sprite->SetRelativeScale(Vec2f{ newScale, newScale });
		co_await Suspend();
	}
	// Make sure there's no floating point funny business
	m_sprite->SetRelativeScale(Vec2f{ in_targetScale, in_targetScale });
}
```

In practice, a [Light](https://github.com/Eastquote/LostAndFound/blob/main/src/Light.h) actor can trivially (and smoothly) appear/disappear or scale up/down from wherever you call InterpScale(). Critically, it can also be called from *outside* Task<> (i.e. coroutine) functions, as we do [here](https://github.com/Eastquote/LostAndFound/blob/main/src/Player.cpp#L138) in the Player's regular Initialize() function.

As another typical use example, here we intentionally create a large light at the moment of the player's death (to accompany the explosion effect) and then interp it down to a scale of 0.0f in half a second:

```cpp
Task<> GameLoop::DestroyPlayerSuit(bool in_isFish) {
	std::shared_ptr<GameWorld> world = GameWorld::Get();
	world->DisablePlayerControls();
	Vec2f playerPos = world->GetPlayerWorldPos();
	
	AudioManager::Get()->PlaySound("Die");
	auto deathLight = Spawn<Light>({ playerPos }, "LightLarge/Core", "LightLarge/Penumbra"); //< Create a light on player
	deathLight->SetScale(1.25f); // Make it big
	deathLight->InterpScale(0.0f, 0.5f); // Start scaling it down to 0, over a half-second duration
	world->GetCameraManager()->AddDistress(5.0f); // Camerashake
	Actor::Spawn<Effect>(world, { playerPos + Vec2f{ 4.0f, 0.0f } }, "Explosion2/Explosion");
```
