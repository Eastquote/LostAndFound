#include "Hud.h"

#include "GameWorld.h"
#include "GameLoop.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "CameraManager.h"
#include "Engine/GameWindow.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Font.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/MathEasings.h"
#include <iostream>

FadeQuad::FadeQuad() {
}
void FadeQuad::Initialize() {
	GameActor::Initialize();
	SetDrawLayer(10);
	m_sprite = MakeSprite();
	m_sprite->SetRenderLayer("hud");
	m_sprite->PlayAnim("Square/10px", true);
	m_sprite->SetWorldScale({ 34.4f, 24.4f });
	m_sprite->SetColor(0, 0, 0, 255);
}
Task<> FadeQuad::ManageActor() {
	while(true) {
		if(m_sprite) {
			auto color = m_sprite->GetColor();
		}
		co_await Suspend();
	}
}

void Hud::Initialize() {
	GameActor::Initialize();
	SetDrawLayer(1);

	// Setup fade quad
	m_fadeQuad = Spawn<FadeQuad>({});
	m_fadeQuad->AttachToActor(AsShared<Hud>());

	// Setup skybox
	m_skyboxQuad = MakeSprite(Transform::Identity);
	m_skyboxQuad->SetRenderLayer("skybox");
	m_skyboxQuad->PlayAnim("Skybox/Space2", true);
	auto windowSize = GetRenderSize();
	m_skyboxQuad->SetRelativePos({(float)windowSize.x / 2, -(float)windowSize.y / 2 });

	// Setup letterbox bars (defaults to off-screen)
	m_letterboxTop = MakeSprite(Transform::Identity);
	m_letterboxTop->SetRenderLayer("hud");
	m_letterboxTop->SetComponentDrawOrder(1000);
	m_letterboxTop->PlayAnim("Square/10px", true);
	m_letterboxTop->SetWorldScale({ 34.4f, 3.6f });
	m_letterboxTop->SetRelativePos(Vec2f{ 0.0f, 36.0f });
	m_letterboxTop->SetColor(0, 0, 0, 255);
	m_letterboxBottom = MakeSprite(Transform::Identity);
	m_letterboxBottom->SetRenderLayer("hud");
	m_letterboxBottom->SetComponentDrawOrder(1000);
	m_letterboxBottom->PlayAnim("Square/10px", true);
	m_letterboxBottom->SetWorldScale({ 34.4f, 3.6f });
	m_letterboxBottom->SetRelativePos(Vec2f{ 0.0f, -208.0f } + Vec2f{ 0.0f, -36.0f });
	m_letterboxBottom->SetColor(0, 0, 0, 255);

	// Create energy label sprite
	m_hudLabelComponent = MakeSprite();
	m_hudLabelComponent->SetRenderLayer("hud");
	m_hudLabelComponent->PlayAnim("Hud2/En", true, 0);

	// Create energy number sprites
	for(int32_t i = 0; i < 2; i++) {
		auto numberSpriteComponent = MakeSprite();
		numberSpriteComponent->SetRenderLayer("hud");
		numberSpriteComponent->SetPlayRate(0.0f);
		numberSpriteComponent->PlayAnim("Hud/Text", true, 2);
		m_healthNumberSprites.push_back(numberSpriteComponent);
	}

	// Create energy tank icon sprites
	for(int32_t i = 0; i < 6; i++) {
		auto eTankSpriteComponent = MakeSprite();
		eTankSpriteComponent->SetRenderLayer("hud");
		eTankSpriteComponent->SetPlayRate(0.0f);
		eTankSpriteComponent->PlayAnim("EnergyTankIcons/Icons", true, 0);
		eTankSpriteComponent->SetHidden(true);
		m_hudEnergyTankSprites.push_back(eTankSpriteComponent);
	}

	// Create LT button sprite
	m_ltButtonComponent = MakeSprite();
	m_ltButtonComponent->SetRenderLayer("hud");
	m_ltButtonComponent->PlayAnim("HudTriggerButtonIcons/HudTriggerButtonIcons", false, 0);
	m_ltButtonComponent->SetHidden(true);
	// Create RT button sprite
	m_rtButtonComponent = MakeSprite();
	m_rtButtonComponent->SetRenderLayer("hud");
	m_rtButtonComponent->PlayAnim("HudTriggerButtonIcons/HudTriggerButtonIcons", false, 1);
	m_rtButtonComponent->SetHidden(true);

	// Create X button sprite
	m_xButtonComponent = MakeSprite();
	m_xButtonComponent->SetRenderLayer("hud");
	m_xButtonComponent->PlayAnim("HudButtonIcons/HudButtonIcons", false, 0);
	m_xButtonComponent->SetHidden(true);

	// Create Y button sprite
	m_yButtonComponent = MakeSprite();
	m_yButtonComponent->SetRenderLayer("hud");
	m_yButtonComponent->PlayAnim("HudButtonIcons/HudButtonIcons", false, 1);
	m_yButtonComponent->SetHidden(true);

	// Create primary weapon icons
	for(int32_t i = 0; i < 4; i++) {
		auto pWeaponSpriteComponent = MakeSprite();
		pWeaponSpriteComponent->SetRenderLayer("hud");
		pWeaponSpriteComponent->SetPlayRate(0.0f);
		pWeaponSpriteComponent->PlayAnim("HudWeaponIcons/WeaponIcons", false, i);
		pWeaponSpriteComponent->SetHidden(true);
		m_primaryWeaponSprites.push_back(pWeaponSpriteComponent);
	}

	// Create support weapon icons
	for(int32_t i = 0; i < 1; i++) {
		auto sWeaponSpriteComponent = MakeSprite();
		sWeaponSpriteComponent->SetRenderLayer("hud");
		sWeaponSpriteComponent->SetPlayRate(0.0f);
		sWeaponSpriteComponent->PlayAnim("HudWeaponIcons/Weapon2Icons", false, i);
		sWeaponSpriteComponent->SetHidden(true);
		m_secondaryWeaponSprites.push_back(sWeaponSpriteComponent);
	}

	// Setup missile text
	m_missileCountTextComp = MakeText(Transform::Identity);
	m_missileCountTextComp->SetRenderLayer("hud");
	m_missileCountTextComp->SetFont("PixicaMicro-Regular");
	m_missileCountTextComp->GetFont()->SetSmooth(false);
	m_missileCountTextComp->SetFontSizePixels(16);
	m_missileCountTextComp->SetText(L"000");
	m_missileCountTextComp->SetAlignment({ 0.5f, 0.0f });
	m_missileCountTextComp->SetColor(sf::Color::White);
	m_missileCountTextComp->SetHidden(true);
	m_missileCountTextComp->SetComponentDrawOrder(10);

	// Setup grenade text
	m_grenadeCountTextComp = MakeText(Transform::Identity);
	m_grenadeCountTextComp->SetRenderLayer("hud");
	m_grenadeCountTextComp->SetFont("PixicaMicro-Regular");
	m_grenadeCountTextComp->GetFont()->SetSmooth(false);
	m_grenadeCountTextComp->SetFontSizePixels(16);
	m_grenadeCountTextComp->SetText(L"00");
	m_grenadeCountTextComp->SetAlignment({ 0.5f, 0.0f });
	m_grenadeCountTextComp->SetColor(sf::Color::White);
	m_grenadeCountTextComp->SetHidden(true);
	m_grenadeCountTextComp->SetComponentDrawOrder(10);

	SetUpdateStage(eUpdateStage::Final);
	printf("HUD initialized!\n");
}
void Hud::Destroy() {
	printf("HUD destroyed!\n");
	GameActor::Destroy();
}

Task<> Hud::ManageActor() {
	auto world = GetWorld();
	co_await InterpLetterboxVisibility(false);
	int32_t previousHealth = 0;
	int32_t previousMissiles = 0;
	auto playerStatus = GetWorld()->GetPlayerStatus();
	Vec2f hudRelativePos = Vec2f{ 0.0f, 0.0f };
	std::string healthTensVal = "0";
	std::string healthOnesVal = "0";

	// Spacing & offset constants (in pixels)
	auto primaryOffset = 23;
	auto ltButtonIconOffset = 22;
	auto iconGapOffset = 16;
	while(true) {
		auto cameraTransform = world->GetCameraManager()->GetCameraWorldTransform();

		// Display correct number of etanks
		auto energyTankCount = playerStatus->GetEnergyTankCount();
		for(auto sprite : m_hudEnergyTankSprites) {
			sprite->SetHidden(true);
		}
		for(int32_t i = 0; i < energyTankCount; i++) {
			m_hudEnergyTankSprites[i]->SetHidden(false);
		}

		// Update health data
		auto currentHealth = playerStatus->GetEnergy();
		if(currentHealth != previousHealth) {
			auto healthString = L"EN  " + std::to_wstring(currentHealth);
			for(auto sprite : m_hudEnergyTankSprites) {
				sprite->PlayAnim("EnergyTankIcons/Icons", true, 0);
			}

			// Use healthNumberSprites to assemble and display numerical health value
			// (as this use-case is too limited to justify creating/using a full custom font)
			auto healthValueString = std::to_string(currentHealth);
			if(currentHealth <= 0) {
				currentHealth = 0;
				m_healthNumberSprites[0]->PlayAnim("Hud/Text", true, 0);
				m_healthNumberSprites[1]->PlayAnim("Hud/Text", true, 0);
			}
			else if(currentHealth <= 9) {
				m_healthNumberSprites[0]->PlayAnim("Hud/Text", true, 0);
				m_healthNumberSprites[1]->PlayAnim("Hud/Text", true, std::stoi(healthValueString));
			}
			else if(currentHealth > 99) {
				
				healthTensVal = healthValueString[1];
				healthOnesVal = healthValueString[2];
				m_healthNumberSprites[0]->PlayAnim("Hud/Text", true, std::stoi(healthTensVal));
				m_healthNumberSprites[1]->PlayAnim("Hud/Text", true, std::stoi(healthOnesVal));
				for(int32_t i = 0; i < energyTankCount; i++) {
					m_hudEnergyTankSprites[i]->PlayAnim("EnergyTankIcons/Icons", true, 1);
				}
			}

			// Nominal case (10-99)
			else if((9 < currentHealth) && (currentHealth < 100)) {
				healthTensVal = healthValueString[0];
				healthOnesVal = healthValueString[1];
				m_healthNumberSprites[0]->PlayAnim("Hud/Text", true, std::stoi(healthTensVal));
				m_healthNumberSprites[1]->PlayAnim("Hud/Text", true, std::stoi(healthOnesVal));
			}
			previousHealth = currentHealth;
		}

		// Update unlocked & selected weapon data
		auto pWeaponShowList = playerStatus->GetPWeaponUnlocks();
		auto sWeaponShowList = playerStatus->GetSWeaponUnlocks();
		auto pWeaponActive = playerStatus->GetSelectedPrimary();
		auto sWeaponActive = playerStatus->GetSelectedSecondary();
		auto offset = primaryOffset;

		// Update item count data & text
		auto missileCount = playerStatus->GetMissileCount();
		auto missileCountString = std::to_wstring(missileCount);
		if(missileCount < 10) {
			missileCountString = L"0" + missileCountString;
		}
		auto grenadeCount = playerStatus->GetGrenadeCount();
		auto grenadeCountString = std::to_wstring(grenadeCount);
		if(grenadeCount < 10) {
			grenadeCountString = L"0" + grenadeCountString;
		}
		m_missileCountTextComp->SetText(missileCountString);
		m_grenadeCountTextComp->SetText(grenadeCountString);

		// Start laying out weapon icon elements
		if(pWeaponShowList.size() > 1) {
			m_ltButtonComponent->SetHidden(false);
			offset -= ltButtonIconOffset;
			m_ltButtonComponent->SetRelativePos(Vec2i{ offset, -226 } + m_bottomRowAnchor);
			offset += ltButtonIconOffset;
		}
		else {
			m_ltButtonComponent->SetHidden(true);
		}
		auto hasMissile = false;
		auto hasGrenade = false;
		if(pWeaponShowList.size() > 0) {
			for(auto sprite : m_primaryWeaponSprites) {
				sprite->SetHidden(true);
			}
			for(auto item : pWeaponShowList) {
				auto sprite = m_primaryWeaponSprites[item];
				sprite->SetHidden(false);
				sprite->SetRelativePos(Vec2i{ offset, -222 } + m_bottomRowAnchor);
				if(item == 1) {
					hasMissile = true;
					m_missileCountTextComp->SetHidden(false);
					m_missileCountTextComp->SetRelativePos(sprite->GetRelativePos() + Vec2i{ 13, -9 } + m_bottomRowAnchor);
					if(missileCount <= 0) {
						sprite->PlayAnim("HudWeaponIcons/WeaponIconsDis", true, item);
					}
				}
				if(!hasMissile) {
					m_missileCountTextComp->SetHidden(true);
				}
				offset += 17;
			}
		}
		offset += iconGapOffset;
		if(sWeaponShowList.size() > 0) {
			for(auto item : sWeaponShowList) {
				auto sprite = m_secondaryWeaponSprites[item];
				sprite->SetHidden(false);
				sprite->SetRelativePos(Vec2i{ offset, -222 } + m_bottomRowAnchor);
				if(item == 0) {
					hasGrenade = true;
					m_grenadeCountTextComp->SetHidden(false);
					m_grenadeCountTextComp->SetRelativePos(sprite->GetRelativePos() + Vec2i{ 13, -9 } + m_bottomRowAnchor);
				}
				offset += (19 + 26);
			}
		}
		//if(sWeaponShowList.size() > 1) {
		//	m_rtButtonComponent->SetHidden(false);
		//	m_rtButtonComponent->SetWorldPos(ViewToWorld(Vec2i{ offset, 4 }));
		//	offset += ltButtonIconOffset;
		//}

		// Set weapon icon palettes
		for(auto i = 0; i < m_primaryWeaponSprites.size(); i++) {
			if(i == pWeaponActive) {
				//m_primaryWeaponSprites[i]->SetPalette("Selected");
				m_primaryWeaponSprites[i]->PlayAnim("HudWeaponIcons/WeaponIconsSel", false, i);
			}
			else {
				m_primaryWeaponSprites[i]->SetPalette("Base");
				m_primaryWeaponSprites[i]->PlayAnim("HudWeaponIcons/WeaponIcons", false, i);
			}
		}
		for(auto i = 0; i < m_secondaryWeaponSprites.size(); i++) {
			m_secondaryWeaponSprites[i]->SetPalette("Selected");
		}
		if(playerStatus->GetGrenadeCount()) {
			m_secondaryWeaponSprites[0]->PlayAnim("HudWeaponIcons/Weapon2IconsSel", false, 0);
			if(sWeaponShowList.size() > 0) {
				m_yButtonComponent->SetHidden(false);
			}
		}
		else {
			m_secondaryWeaponSprites[0]->PlayAnim("HudWeaponIcons/Weapon2IconsDis", false, 0);
			m_yButtonComponent->SetHidden(true);
		}

		// Lay out X and Y buttons
		auto buttonOffset = Vec2i{ 2, 12 };
		auto sprite = m_primaryWeaponSprites[pWeaponActive];
		auto spritePos = sprite->GetRelativePos();
		m_xButtonComponent->SetRelativePos(spritePos + buttonOffset + m_bottomRowAnchor);
		m_xButtonComponent->SetHidden(false);

		// Lay out Lt and Rt buttons
		if(sWeaponShowList.size() > 0) {
			sprite = m_secondaryWeaponSprites[0];
			spritePos = sprite->GetRelativePos();
			m_yButtonComponent->SetRelativePos(spritePos + buttonOffset + m_bottomRowAnchor);
			//m_yButtonComponent->SetHidden(false);
		}
		if(sWeaponShowList.size() > 1) {
			sprite = m_secondaryWeaponSprites[1];
			spritePos = sprite->GetRelativePos();
			m_rtButtonComponent->SetRelativePos(spritePos + buttonOffset + m_bottomRowAnchor);
			m_rtButtonComponent->SetHidden(false);
		}

		// Lay out etanks
		auto eTankPos = Vec2i{ 51, -9 };
		for(auto sprite : m_hudEnergyTankSprites) {
			sprite->SetRelativePos(eTankPos + m_topRowAnchor);
			eTankPos.x -= 5;
		}

		m_hudLabelComponent->SetRelativePos(Vec2i{ 23, -14 } + m_topRowAnchor);

		// Lay out HUD numbers
		auto healthNumberPos = Vec2i{ 43, -17 };
		for(auto sprite : m_healthNumberSprites) {
			sprite->SetRelativePos(healthNumberPos + m_topRowAnchor);
			healthNumberPos.x += 8;
		}

		// Debug elements
		if(GameLoop::ShouldDisplayDebug()) {
			// Display FPS
			auto frameRate = (int32_t)std::round(1.0f / DT());
			auto fpsColor = sf::Color::Green;
			if(frameRate < 59) {
				fpsColor = sf::Color::White;
			}
			if(frameRate < 50) {
				fpsColor = sf::Color::Yellow;
			}
			if(frameRate < 40) {
				fpsColor = sf::Color::Red;
			}
			auto fpsPlacement = Vec2i{ 10, 20 };
			DrawDebugString(world->ViewToWorld(fpsPlacement + Vec2i{ 0, 1 }), std::to_string(frameRate), sf::Color::Black);
			DrawDebugString(world->ViewToWorld(fpsPlacement + Vec2i{ 0, -1 }), std::to_string(frameRate), sf::Color::Black);
			DrawDebugString(world->ViewToWorld(fpsPlacement + Vec2i{ 1, 0 }), std::to_string(frameRate), sf::Color::Black);
			DrawDebugString(world->ViewToWorld(fpsPlacement + Vec2i{ -1, 0 }), std::to_string(frameRate), sf::Color::Black);
			DrawDebugString(world->ViewToWorld(fpsPlacement + Vec2i{ -1, 1 }), std::to_string(frameRate), sf::Color::Black);
			DrawDebugString(world->ViewToWorld(fpsPlacement + Vec2i{ 1, 1 }), std::to_string(frameRate), sf::Color::Black);
			DrawDebugString(world->ViewToWorld(fpsPlacement), std::to_string(frameRate), fpsColor);
		}

		// Move entire HUD to camera location using View-to-world position conversion
		SetWorldPos(world->ViewToWorld(Vec2i::Zero + hudRelativePos));
		
		co_await Suspend();
	}
}
Task<> Hud::EasePos(Vec2i& in_currentPos, const Vec2i& in_targetPos, float in_duration) {
	auto world = GetWorld();
	auto startPos = (Vec2f)in_currentPos;
	auto posDelta = (Vec2f)in_targetPos - startPos;
	auto elapsedTime = 0.0f;
	auto interpAlpha = 0.0f;
	while(elapsedTime < in_duration) {
		elapsedTime += DT();
		interpAlpha = std::clamp(world->EaseAlpha(elapsedTime, in_duration, Math::EaseInOutSmootherstep), 0.0f, 1.0f);
		if(interpAlpha > 0.5f) {
			//printf("");
		}
		auto framePosDelta = posDelta * interpAlpha;
		auto result = (startPos + framePosDelta);
		in_currentPos = (Vec2i)result;
		co_await Suspend();
	}
	in_currentPos = in_targetPos;
}
Task<> Hud::HudTransitionManager(bool in_show, float in_duration, bool in_topZone, bool in_bottomZone) {
	auto topTarget = in_show ? 0 : 30;
	auto bottomTarget = in_show ? 0 : -30;
	if(in_topZone && in_bottomZone) {
		co_await WaitForAll({
			EasePos(m_topRowAnchor, Vec2i{ 0, topTarget }, in_duration),
			EasePos(m_bottomRowAnchor, Vec2i{ 0, bottomTarget }, in_duration),
			});
	}
	else if(in_topZone) {
		co_await EasePos(m_topRowAnchor, Vec2i{ 0, topTarget }, in_duration);
	}
	else {
		co_await EasePos(m_bottomRowAnchor, Vec2i{ 0, bottomTarget }, in_duration);
	}
}
TaskHandle<> Hud::InterpHudVisiblity(bool in_show, float in_duration, bool in_topZone, bool in_bottomZone){
	m_transitionHudTask = m_taskMgr.Run(HudTransitionManager(in_show, in_duration, in_topZone, in_bottomZone));
	return m_transitionHudTask;
}
Task<> Hud::InterpLetterboxVisibility(bool in_show, bool in_topBar, bool in_bottomBar, float in_duration){
	auto topTarget = in_show ? Vec2f::Zero : Vec2f{ 0.0f, 36.0f };
	auto bottomTarget = in_show ? Vec2f{ 0.0f, -208.0f } : Vec2f{ 0.0f, -208.0f } + Vec2f{ 0.0f, -36.0f };

	// Early-out if duration is 0.0 seconds
	if(in_duration == 0.0f) {
		if(in_topBar) {
			m_letterboxTop->SetHidden(in_show ? false : true);
			m_letterboxTop->SetRelativePos(topTarget);
		}
		if(in_bottomBar) {
			m_letterboxBottom->SetHidden(in_show ? false : true);
			m_letterboxBottom->SetRelativePos(bottomTarget);
		}
		co_return;
	}
	else {
		TaskHandle<> topMoveTask;
		TaskHandle<> bottomMoveTask;
		if(in_topBar) {
			m_letterboxTop->SetHidden(false);
			topMoveTask = m_taskMgr.Run(AnimateSpritePos(m_letterboxTop, topTarget, in_duration));
		}
		if(in_bottomBar) {
			m_letterboxBottom->SetHidden(false);
			bottomMoveTask = m_taskMgr.Run(AnimateSpritePos(m_letterboxBottom, bottomTarget, in_duration));
		}
		co_await WaitForAll({
				topMoveTask,
				bottomMoveTask,
			});
		if(in_topBar) {
			m_letterboxTop->SetRelativePos(topTarget);
			m_letterboxTop->SetHidden(in_show ? false : true);
		}
		if(in_bottomBar) {
			m_letterboxBottom->SetRelativePos(bottomTarget);
			m_letterboxBottom->SetHidden(in_show ? false : true);
		}
	}
	co_return;
}
Task<> Hud::FadeScreenToSolid(float in_duration, std::optional<sf::Color> in_color) {
	auto fadeSprite = m_fadeQuad->GetSprite();
	if(in_color.has_value()) {
		auto color = in_color.value();
		fadeSprite->SetColor(color.r, color.g, color.b, color.a);
	}
	co_await GetWorld()->Fade(fadeSprite, nullptr, in_duration, true);
}
Task<> Hud::FadeScreenFromSolid(float in_duration, std::optional<sf::Color> in_color) {
	auto fadeSprite = m_fadeQuad->GetSprite();
	if(in_color.has_value()) {
		auto color = in_color.value();
		fadeSprite->SetColor(color.r, color.g, color.b, color.a);
	}
	co_await GetWorld()->Fade(fadeSprite, nullptr, in_duration, false);
}
void Hud::SetSkyboxWorldTransform(const Transform& in_transform) { 
	m_skyboxQuad->SetWorldTransform(in_transform);
}
Task<> Hud::AnimateSpritePos(std::shared_ptr<SpriteComponent> in_sprite, Vec2f in_targetPos, float in_duration){
	auto world = GetWorld();
	auto startPos = in_sprite->GetRelativePos();
	auto totalDisp = in_targetPos - startPos;
	auto elapsedTime = 0.0f;
	auto dispAlpha = 0.0f;
	while(elapsedTime <= in_duration) {
		elapsedTime += DT();
		dispAlpha = world->EaseAlpha(elapsedTime, in_duration, Math::EaseInOutSmoothstep);
		in_sprite->SetRelativePos(startPos + (totalDisp * dispAlpha));
		co_await Suspend();
	}
}
