#include "GameLoop.h"

#include "GameWorld.h"
#include "Effect.h"
#include "Light.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "Hud.h"
#include "Creature.h"
#include "Creatures/Ship.h"
#include "ParticleSpawnerDefs.h"
#include "AudioManager.h"
#include "CameraManager.h"
#include "MenuItemDefs.h"
#include "Dialogue.h"
#include "AsteroidField.h"
#include "TitleChar.h"
#include "Engine/LayerManager.h"
#include "Engine/Shader.h"
#include "Engine/Game.h"
#include "Engine/GameWindow.h"
#include "Engine/InputSystem.h"
#include "Engine/TileMap.h"
#include "Engine/AssetCache.h"
#include "Engine/Font.h"
#include "Engine/Editor/EditorMode.h"
#include "Engine/Components/ShapeComponent.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/EaseTo.h"
#include "Engine/MathEasings.h"
#include "Engine/Curve.h"
#include "Engine/Editor/DebugTimeControls.h"
#include <iostream>
#include <fstream>

//--- MAIN GAME LOOP CODE ---//

bool GameLoop::s_bDisplayDebug = false;

Task<> GameLoop::ManageActor() {
	// Set up render window
	GetWindow()->SetScalingMode(eWindowScalingMode::PixelSmooth);
	GetWindow()->SetWindowClearColor(sf::Color(32, 32, 32));
	GetWindow()->SetViewClearColor(sf::Color::Black);
	GetWindow()->SetPixelAspectRatio(GetPixelAspectRatio());

	// Setup render layers & shader
	GetWindow()->GetLayerManager()->AddLayer("fgLightMask");
	GetWindow()->GetLayerManager()->AddLayer("bkgLightMask");
	GetWindow()->GetLayerManager()->AddLayer("lights");
	GetWindow()->GetLayerManager()->AddLayer("skybox");
	GetWindow()->GetLayerManager()->AddLayer("parallax");
	GetWindow()->GetLayerManager()->AddLayer("bkgTiles");
	GetWindow()->GetLayerManager()->AddLayer("fgGameplay");
	GetWindow()->GetLayerManager()->AddLayer("fgTiles");
	GetWindow()->GetLayerManager()->AddLayer("hud");
	auto postProcessShader = AssetCache<Shader>::Get()->LoadAsset("data/shaders/PostProcessLighting");
	GetWindow()->SetPostProcessShader(postProcessShader);
	bool newGame = true;

	// Outer loop
	while(true) {
		// Alternate between menu and gameplay until exit
		newGame = co_await GameState_Menu();
		co_await GameState_Gameplay(newGame);
	}
}
Task<bool> GameLoop::GameState_Menu() {
	m_bGameOver = false;

	// Setup input component
	auto world = Guard(Spawn<GameWorld>({}));
	auto hud = world->GetHud();
	auto fadeQuad = Guard(Spawn<FadeQuad>({}));
	auto inputComp = Guard(MakeChild<InputComponent>());
	auto rightInput = inputComp->Button("Right", { eButton::Joy_Right, eButton::Right });
	auto leftInput = inputComp->Button("Left", { eButton::Joy_Left, eButton::Left });
	auto upInput = inputComp->Button("Up", { eButton::Joy_Up, eButton::Up });
	auto downInput = inputComp->Button("Down", { eButton::Joy_Down, eButton::Down, eButton::Joy_Select});
	auto actionInput = inputComp->Button("Action", { eButton::X, eButton::Joy_Start, eButton::D, 
		eButton::Joy_DiamondBottom,
		eButton::Joy_DiamondRight,
		eButton::Joy_DiamondTop,
		eButton::Joy_DiamondLeft,
		eButton::Z,
		eButton::Enter});
	auto cancelInput = inputComp->Button("Cancel", { eButton::Escape, eButton::Joy_DiamondRight });

	// Move camera to origin
	auto windowSize = GetRenderSize();
	GetWindow()->SetWorldView(Box2d{ 0.0f, 0.0f, (float)windowSize.x, (float)windowSize.y });
	fadeQuad->SetWorldPos(world->ViewToWorld(Vec2i::Zero));

	// Setup bkg
	auto bkgSprite = Guard(MakeSprite({ (float)windowSize.x / 2.0f, (float)windowSize.y / 2.0f }));
	bkgSprite->PlayAnim("MenuBkg/Bkg", true);
	bkgSprite->SetRenderLayer("hud");
	bkgSprite->SetComponentDrawOrder(-2000);
	bkgSprite->SetHidden(false);

	// Fade up on main menu
	auto bkgFadeUpTime = 1.0f;
	if(!m_bFirstBoot) {
		bkgFadeUpTime = 1.0f;
	}
	m_taskMgr.RunManaged(world->Fade(fadeQuad->GetSprite(), nullptr, bkgFadeUpTime, false));

	// Play music
	AudioManager::Get()->PlayMusic("Fae", 0.0f, 0.05f);

	// Check for save game
	bool hasSaveFile = false;
	std::ifstream file{ "save.bin", std::ios_base::in | std::ios_base::binary };
	if(file.is_open()) {
		hasSaveFile = true;
	}

	// Make title appear
	std::vector<Vec2f> charOffsets = {
		{20.0f, 62.0f},
		{41.0f, 94.0f},
		{65.0f, 94.0f},
		{91.0f, 94.0f},
		{121.0f, 94.0f},
		{161.0f, 94.0f},
		{196.0f, 94.0f},
		{219.0f, 94.0f},
		{246.0f, 94.0f},
		{274.0f, 94.0f},
		{302.0f, 94.0f},
		{319.0f, 62.0f},
	};
	auto spawnOffset = Vec2f{ -5.0f, 10.0f};
	ObjectMultiGuard titleCharsGuard;
	std::vector<std::shared_ptr<TitleChar>> titleChars;
	for(auto i = 0; i < charOffsets.size(); ++i) {
		auto titleChar = Actor::Spawn<TitleChar>(Transform::Identity, charOffsets[i], "TitleChars/Chars", i, true, spawnOffset, 0.0f * 60.0f, i);
		titleChar->SetWorldPos(world->ViewToWorld(charOffsets[i]));
		titleCharsGuard.Guard(titleChar);
		titleChars.push_back(titleChar);
		co_await WaitSeconds(0.1f);
	}
	
	// Make menu items
	ObjectMultiGuard menuItemsGuard;
	std::vector<std::shared_ptr<MenuItem>> menuItems;
	auto itemPos0 = Vec2f{ 170.0f, 143.0f };
	auto itemPos1 = Vec2f{ 170.0f, 166.0f };
	auto itemPos2 = Vec2f{ 170.0f, 187.0f };
	auto newGameItem = Actor::Spawn<MenuItem>(Transform::Identity, ui_NewGameItem, 0);
	newGameItem->SetWorldPos(world->ViewToWorld(itemPos0));
	newGameItem->SetActive(true);
	menuItemsGuard.Guard(newGameItem);
	menuItems.push_back(newGameItem);
	auto firstItem = newGameItem;
	std::shared_ptr<MenuItem> continueGameItem;
	if(hasSaveFile) {
		// Create a "Continue" option if there's a save file
		continueGameItem = Actor::Spawn<MenuItem>(Transform::Identity, ui_ContinueGameItem, 0);
		continueGameItem->SetWorldPos(world->ViewToWorld(itemPos0));
		continueGameItem->SetActive(true);
		firstItem = continueGameItem;
		menuItems.push_back(continueGameItem);
		newGameItem->SetWorldPos(world->ViewToWorld(itemPos1));
		newGameItem->SetIndex(1);
		newGameItem->SetActive(false);
		auto exitGameItem = Actor::Spawn<MenuItem>(Transform::Identity, ui_ExitGameItem, 2);
		exitGameItem->SetWorldPos(world->ViewToWorld(itemPos2));
		menuItemsGuard.Guard(exitGameItem);
		menuItems.push_back(exitGameItem);
	}
	else {
		auto exitGameItem = Actor::Spawn<MenuItem>(Transform::Identity, ui_ExitGameItem, 1);
		exitGameItem->SetWorldPos(world->ViewToWorld(itemPos1));
		menuItemsGuard.Guard(exitGameItem);
		menuItems.push_back(exitGameItem);
	}
	co_await Suspend(); // HACK: allows the textComponent in the first MenuItem() to initialize and have an actual bounds to query
	
	// Setup carats
	auto caratSpriteL = Guard(MakeSprite());
	caratSpriteL->SetRenderLayer("hud");
	caratSpriteL->PlayAnim("MenuCarat/Carat", true);

	auto caratSpriteR = Guard(MakeSprite());
	caratSpriteR->SetRenderLayer("hud");
	caratSpriteR->PlayAnim("MenuCarat/Carat", true);
	caratSpriteR->SetFlipHori(true);

	auto caratOffset = std::round(firstItem->GetBounds().w / 2) + 8.0f;
	caratSpriteL->SetWorldPos(firstItem->GetWorldPos() + Vec2f{ -caratOffset, 0.0f });
	caratSpriteR->SetWorldPos(firstItem->GetWorldPos() + Vec2f{ caratOffset, 0.0f });

	auto index = 0;
	auto prevIndex = 0;
	auto menuLive = true;
	auto inputLive = false;
	auto newGame = false;
	auto inputsCleared = false;
	auto frameCounter = 0;
	while(menuLive) {
		if(!inputLive) {
			if(frameCounter == 1) {
				co_await WaitUntil([actionInput] {
					return !actionInput->IsPressed();
					});
				inputLive = true;
			}
			frameCounter++;
		}
		if(inputLive) {
			// Detect up/down input, change index accordingly
			if(downInput->JustPressed()) {
				index += 1;
				if(index >= menuItems.size()) {
					index = 0;
				}
			}
			else if(upInput->JustPressed()) {
				index -= 1;
				if(index < 0) {
					index = (int32_t)menuItems.size() - 1;
				}
			}

			// Change which item is selected if index has changed
			if(index != prevIndex) {
				for(auto item : menuItems) {
					if(item->MatchIndex(index)) {
						item->SetActive(true);
						// Update carats' positions on either side of selected item
						caratOffset = std::round(item->GetBounds().w / 2) + 8.0f;
						caratSpriteL->SetWorldPos(item->GetWorldPos() + Vec2f{ -caratOffset, 0.0f});
						caratSpriteR->SetWorldPos(item->GetWorldPos() + Vec2f{ caratOffset, 0.0f});
					}
					else if(item->MatchIndex(prevIndex)) {
						item->SetActive(false);
					}
					else {
						item->SetActive(false);
					}
				}
			}

			// Else if no changes, listen for activation
			else if(actionInput->JustPressed())
				for(auto item : menuItems) {
					if(item->MatchIndex(index)) {
						item->TryActivate();
						if(index == newGameItem->GetIndex()) {
							newGame = true;
							menuLive = false;
						}
						else if(continueGameItem && index == continueGameItem->GetIndex()) {
							co_await AudioManager::Get()->FadeMusic(1.0f, 0.0f);
							newGame = false;
							menuLive = false;
						}
					}
				}
			prevIndex = index;
		}
		co_await Suspend();
	}
	m_bFirstBoot = false;

	// Fade to black
	co_await world->Fade(fadeQuad->GetSprite(), nullptr, 1.0f, true);

	// Cleanup
	for(auto item : menuItems) {
		item->Destroy();
	}
	menuItems.clear();
	co_await WaitSeconds(0.01f);
	co_return newGame;
}
Task<> ManageToggleDebugDraw(std::shared_ptr<ButtonState> debugToggleInput, bool& io_flag) {
	while(true) {
		if(debugToggleInput->JustPressed()) {
			io_flag = !io_flag;
		}
		co_await Suspend();
	}
}
Task<> GameLoop::IntroCinematicTask(){
	auto world = GameWorld::Get();
	auto room = world->GetPlayerRoom();
	auto hud = world->GetHud();
	auto playerStatus = world->GetPlayerStatus();

	// Wait for creatures to spawn
	while(room->creatures.size() == 0) {
		co_await Suspend();
	}

	// Verify we're in the correct room for this scene, and that ship is present
	if(room->Id == 0) {
		bool shipIsPresent = false;
		std::shared_ptr<Ship> ship;
		for(auto creature : room->creatures) {
			if(std::dynamic_pointer_cast<Ship>(creature)) {
				ship = std::dynamic_pointer_cast<Ship>(creature);
				shipIsPresent = true;
				break;
			}
		}
		assert(shipIsPresent);

		// Ship setup
		auto shipTargetPos = ship->GetWorldPos();
		auto camera = world->GetCameraManager();
		camera->SetOverridden(true);
		ship->SetWorldPos(shipTargetPos + Vec2f{ 0.0f, 560.0f });
		auto shipStartPos = ship->GetWorldPos();

		// Player setup
		world->DisablePlayerControls();
		world->AttachPlayerTo(ship, true);
		world->SetPlayerWorldPos(shipStartPos + Vec2f{ 0.0, -20.0f });

		// Camera setup
		auto cameraOffset = Vec2f{ 0.0f, -9.0f };
		camera->SetCameraPos(shipTargetPos + Vec2f{ 0.0f, 560.0f } + cameraOffset);
		auto fadeFromBlackTask = m_taskMgr.RunManaged(hud->FadeScreenFromSolid());

		// Execute cinematic script
		// NOTE: This stuff really doesn't belong here -- in the future, I would pull out all these
		// scene defs into their own data external data files.
		SceneDef scene01_landing{
			{L"DANIEL", L"_LEFT"}, // _LEFT tag currently does nothing, but the character in this first index gets left portrait
			{L"GERALDINE", L"_RIGHT"}, // and this character goes on the right

			{L"DANIEL", L"Well, I hate it."},
			{L"DANIEL", L"Bots 'n rot just... everywhere."},

			{L"DANIEL", L"Remind me why we're scraping antiques off creepy derelicts now?"},

			{L"GERALDINE", L"I just follow the beacons, Dan. I'm a ship."},

			{L"DANIEL", L"Yes, well, please don't crash and strand us here, ok?"},

			{L"GERALDINE", L"How's about you get moving so I can go land?"},
		};

		// Launch dialogue scene
		auto dialogue1 = Spawn<Dialogue>(Transform::Identity, scene01_landing, 3.0f);

		// Animate ship descent and landing
		auto shipDescentCurve = Curve("data/curves/ShipDescent.rtcurve"); //< Hand-authored using in-game editor
		auto descentDuration = 20.0f; // 20.0 is default
		auto elapsedTime = DT();
		shipStartPos = ship->GetWorldPos();
		auto descentDistance = shipStartPos - shipTargetPos;
		while(elapsedTime <= descentDuration) {
			auto scaledTime = elapsedTime / descentDuration;
			auto newDisp = (1.0f - shipDescentCurve.Eval(scaledTime)) * descentDistance.y;
			auto newPos = shipStartPos + Vec2f{ 0.0f, -newDisp };
			ship->SetWorldPos(newPos);
			camera->SetCameraPos(Vec2f{ std::round(newPos.x), std::round(newPos.y) } + cameraOffset);
			elapsedTime += DT();
			co_await Suspend();
		}
		co_await WaitUntil([dialogue1]() {
			return !IsAlive(dialogue1);
			});

		auto audioMgr = AudioManager::Get();
		auto fadeMenuMusicTask = audioMgr->FadeMusic(3.0f, 0.0f);
		co_await WaitSeconds(0.5f);

		// Play open-door anim && door opening sound
		co_await ship->OpenDoor();

		// Animate player-emerging-from-ship
		auto playerTarget = ship->GetWorldPos() + Vec2f{ 0.0f, 12.0f };
		auto playerPos = world->GetPlayerWorldPos();
		while(playerPos != playerTarget) {
			auto newPos = EaseTo_Linear(playerPos, playerTarget, 0.25f);
			world->SetPlayerWorldPos(newPos);
			playerPos = world->GetPlayerWorldPos();
			co_await Suspend();
		}
		hud->InterpHudVisiblity(true, 1.0f);
		co_await WaitSeconds(0.5f);
		co_await ship->CloseDoor();

		// "Freeze" ship so it becomes terrain
		ship->SetSolid(true);

		// Detach Player from Ship
		world->DetachPlayer();
		world->MovePlayer(playerPos + Vec2f{ 0.0f, 3.0f }, false);
		world->MovePlayer(playerPos + Vec2f{ 0.0f, -5.0f }, false);

		// Cleanup
		playerStatus->CompleteCinematic(1);
		playerStatus->UnlockCinematic(2);
		AudioManager::Get()->PlayMusic("HammyEpicMyth", 0.0f, 0.04f);
		world->EnablePlayerControls();
		camera->SetOverridden(false);
	}
	else {
		printf("Can't play intro cinematic -- player is in wrong room!\n");
	}
}
// Speed-ramp function (to exit slowmo smoothly)
Task<> GameLoop::ReturnToFullSpeed(float in_totalDuration) {
	auto elapsedTime = 0.0f;
	auto world = GameWorld::Get();
	auto startTimeRate = GameBase::Get()->GetTimeDilation();
	while(elapsedTime < in_totalDuration) {
		auto alpha = world->EaseAlpha(elapsedTime, in_totalDuration, Math::EaseInOutSmoothstep);
		auto newRate = startTimeRate + ((1.0f - startTimeRate) * alpha);
		GameBase::Get()->SetTimeDilation(newRate);
		elapsedTime += DT();
		co_await Suspend();
	}
	GameBase::Get()->SetTimeDilation(1.0f);
};
Task<> GameLoop::CinematicTask(bool in_bCinematicsEnabled) {
	auto world = GameWorld::Get();
	auto hud = world->GetHud();
	auto audioMgr = AudioManager::Get();
	auto playerStatus = world->GetPlayerStatus();
	auto particlesSpawned = false;
	while(in_bCinematicsEnabled) {

		// Update cinematic heuristics
		auto inBossRoom = world->GetPlayerRoom()->Id == 0;
		auto inTriggerRoom = world->GetPlayerRoom()->Id == 2;
		auto inFinalRoom = world->GetPlayerRoom()->Id == 99;
		auto itemAcquired = world->GetPlayerStatus()->IsChargeUnlocked();
		auto grenadesAcquired = world->GetPlayerStatus()->IsGrenadeUnlocked();

		// Trigger scene 2
		if(	playerStatus->IsCinematicUnlocked(2) && !playerStatus->IsCinematicComplete(2) && inTriggerRoom 
			&& itemAcquired && world->IsPlayerGrounded()) {
			SceneDef scene02_bigFind = {
				{L"DANIEL", L"_LEFT"}, // _LEFT tag currently does nothing, but the character in this first index gets left portrait
				{L"GERALDINE", L"_RIGHT"}, // and this character goes on the right

				{L"DANIEL", L"Good news, Ger: the beacon was coming from an old Charge Cel."},
				{L"DANIEL", L"Perfect condition. It'll set us up for years!"},

				{L"DANIEL", L"Geraldine? Can you hear me?"},

				{L"DANIEL", L"Well, if you're there, I'm heading back now..."},
				{L"DANIEL", L"...WITH my sweet new Charge-jump!"},
			};
			world->DisablePlayerControls();
			co_await audioMgr->FadeMusic(1.5f, 0.00f, true);

			// Play dialogue sequence
			auto dialogue2 = Spawn<Dialogue>(Transform::Identity, scene02_bigFind);
			co_await WaitUntil([dialogue2]() {
				return !IsAlive(dialogue2);
				});
			playerStatus->CompleteCinematic(2);
			world->EnablePlayerControls();
			audioMgr->ResumeMusic();
			co_await audioMgr->FadeMusic(1.5f, 0.04f);
		}

		// Trigger scene 3 (player suit being destroyed by boss)
		if(!particlesSpawned && playerStatus->IsCinematicUnlocked(3) && !playerStatus->IsCinematicComplete(3) && inBossRoom) {
			co_await DestroyPlayerSuit(world->GetPlayer()->IsFish());
			hud->InterpHudVisiblity(false, 0.5f, false, true);
			
			co_await WaitUntil([this, world] {
				return world->IsPlayerGrounded();
				});
			playerStatus->CompleteCinematic(3);
			playerStatus->UnlockCinematic(4);
			co_await m_taskMgr.RunManaged(hud->InterpLetterboxVisibility(false, true, true, 0.5f));
			world->EnablePlayerControls();
			co_await audioMgr->FadeMusic(15.0f, 0.0f, true);
		}

		// Trigger scene 4 (final room of demo)
		if(playerStatus->IsCinematicUnlocked(4) && !playerStatus->IsCinematicComplete(4) && inFinalRoom
			&& grenadesAcquired && world->IsPlayerGrounded()) {
			SceneDef scene04_takeABow = {
				{L"DANIEL", L"_LEFT"}, // _LEFT tag currently does nothing, but the character in this first index gets left portrait
				{L"GERALDINE", L"_RIGHT"}, // and this character goes on the right

				{L"DANIEL", L"Just breaking character to tell you, the player, that I'm really impressed!"},
				{L"GERALDINE", L"Me, too! Very well done."},

				{L"DANIEL", L"The Lost & Found demo is over now, but you can still wander around/blow stuff up."},
				{L"GERALDINE", L"Your new grenades are pretty good for that -- though still experimental!"},

				{L"DANIEL", L"Thank you for playing!"},
				{L"GERALDINE", L"We hope you had fun! Bye now."},
			};
			world->DisablePlayerControls();
			co_await audioMgr->FadeMusic(1.5f, 0.00f, true);

			// Play dialogue sequence
			auto dialogue4 = Spawn<Dialogue>(Transform::Identity, scene04_takeABow);
			co_await WaitUntil([dialogue4]() {
				return !IsAlive(dialogue4);
				});
			playerStatus->CompleteCinematic(4);
			world->EnablePlayerControls();
			audioMgr->ResumeMusic();
			co_await audioMgr->FadeMusic(1.5f, 0.04f);
		}
		co_await Suspend();
	}
}
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
	if(!in_isFish) {
		Actor::Spawn<ParticleSpawner>(GameWorld::Get(), { playerPos + Vec2f{ -4.0f, 8.0f } },
			g_playerDeathExplosionDef, std::nullopt);
		Actor::Spawn<ParticleSpawner>(GameWorld::Get(), { playerPos + Vec2f{ 4.0f, 8.0f } },
			g_playerDeathExplosionDef, std::nullopt);
		Actor::Spawn<ParticleSpawner>(GameWorld::Get(), { playerPos + Vec2f{ -4.0f, -8.0f } },
			g_playerDeathExplosionDef, std::nullopt);
		Actor::Spawn<ParticleSpawner>(GameWorld::Get(), { playerPos + Vec2f{ 4.0f, -8.0f } },
			g_playerDeathExplosionDef, std::nullopt);
		co_await WaitSeconds(0.05f);
		Actor::Spawn<Effect>(world, { playerPos + Vec2f{ -4.0f, 8.0f } }, "Explosion2/Explosion");
		co_await WaitSeconds(0.05f);
		Actor::Spawn<Effect>(world, { playerPos + Vec2f{ -4.0f, -8.0f } }, "Explosion2/Explosion");
	}
	Actor::Spawn<ParticleSpawner>(GameWorld::Get(), { playerPos + Vec2f{ -4.0f, 0.0f } },
		g_playerDeathExplosionDef, std::nullopt);
	Actor::Spawn<ParticleSpawner>(GameWorld::Get(), { playerPos + Vec2f{ 4.0f, 0.0f } },
		g_playerDeathExplosionDef, std::nullopt);
	GameBase::Get()->SetTimeDilation(0.1f);
	co_await WaitSeconds(0.4f);
	co_await ReturnToFullSpeed(0.5f);
}
Task<> GameLoop::GameState_Gameplay(bool in_newGame) {
	// Create the world object (n.b. will try to load saved game if in_newGame == false)
	ObjectGuard<GameWorld> world = Guard(Spawn<GameWorld>({}, in_newGame));

	ObjectGuard<DebugTimeControls> debugTimeControls = Guard(Spawn<DebugTimeControls>({}));
	ObjectGuard<EditorModeToggle> editorToggle = Guard(Spawn<EditorModeToggle>({}));

	// Hijack the mouse cursor
	auto inputSys = GameBase::Get()->GetWindow()->GetInputSystem();

	// Setup input component
	auto inputComp = Guard(MakeChild<InputComponent>());
	inputComp->SetPriority(100);
	auto cursor = inputComp->Cursor("cursor", { eCursor::AbsoluteMouse }, true);
	auto debugToggleInput = inputComp->Button("Debug", { eButton::Insert, eButton::Joy_Select });
	auto quitInput = inputComp->Button("Quit", { eButton::Escape });

	auto toggleDebugDrawTask = m_taskMgr.Run(ManageToggleDebugDraw(debugToggleInput, s_bDisplayDebug));

	// Load map
	world->LoadMap("data/tilemaps/GO_worldMap01.tmx");

	// Asteroid setup
	auto camera = world->GetCameraManager();
	auto cameraBounds = camera->GetCameraBoundsWorld();
	auto asteroidField = Spawn<AsteroidField>( {camera->GetWorldPos() + Vec2f{cameraBounds.w / 2.0f, cameraBounds.h / 2.0f}});
	asteroidField->AttachToActor(camera, false);

	auto hud = world->GetHud();
	hud->InterpHudVisiblity(false);
	auto playerStatus = GameWorld::Get()->GetPlayerStatus();
	playerStatus->UnlockCinematic(1);
	auto cinematicsEnabled = true;

	// DEBUG: setup boss fight for rapid iteration
	//playerStatus->CompleteCinematic(1);
	//playerStatus->UnlockCinematic(2);
	//playerStatus->CompleteCinematic(2);
	//playerStatus->UnlockCharge();

	// If in_newGame, run intro cinematic script
	if(in_newGame && cinematicsEnabled && playerStatus->IsCinematicUnlocked(1) && !playerStatus->IsCinematicComplete(1)) {
		co_await IntroCinematicTask();
	}

	// Otherwise fade up and start the gameplay
	else {
		auto fadeFromBlackTask = m_taskMgr.RunManaged(hud->FadeScreenFromSolid(0.0f));
		if(playerStatus->IsNewSuitUnlocked()) {
			AudioManager::Get()->PlayMusic("TorchOfKnowledge", 0.0f, 0.035f);
		}
		hud->InterpHudVisiblity(true, 0.75f);
		world->EnablePlayerControls();
	}

	// Kick off cinematic task -- 
	// if cinematics are enabled, this will trigger all subsequent cinematics whenever relevant conditions are met
	auto cinematicTask = m_taskMgr.RunManaged(CinematicTask(cinematicsEnabled));

	auto GameOverTask = [this]()->Task<> {
		int32_t playerHealth;
		Vec2f playerPos;
		auto world = GameWorld::Get();
		while(true) {
			playerHealth = world->GetPlayerHealth();
			playerPos = world->GetPlayerWorldPos();
			if(playerHealth <= 0) {
				co_await DestroyPlayerSuit(world->GetPlayer()->IsFish());
				m_bGameOver = true;
			}
			else if(world->GetGameOverRequest()) {
				m_bGameOver = true;
			}
			co_await Suspend();
		}
	};
	auto gameOverTask = GameOverTask();
	auto gameOverTaskHandle = m_taskMgr.Run(std::move(gameOverTask));
	// Wait m_bGameOver is set to true
	co_await WaitUntil([this] { return m_bGameOver; });
}
