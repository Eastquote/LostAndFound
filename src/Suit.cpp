#include "Suit.h"

#include "Player.h"
#include "GameWorld.h"
#include "GameEnums.h"
#include "Engine/Components/SpriteComponent.h"

//--- SUIT CODE ---//

void Suit::Initialize() {
	GameActor::Initialize();

	// Setup sprites
	SetDrawLayer(0);
	m_sprite = MakeSprite();
	m_sprite->SetRenderLayer("bkgTiles");
	m_sprite->PlayAnim("GariSuitEntry/Idle", true);

	// Foreground sprite enables "sandwiching" player between two sprites to create illusion of falling "into" the suit
	m_fgSprite = MakeSprite();
	m_fgSprite->SetRenderLayer("fgGameplay");
	m_fgSprite->PlayAnim("GariSuitEntry/Idle", true); //< TODO: Needs a custom anim here, tho. Still looks good without it!
	m_fgSprite->SetComponentDrawOrder(10);

	// Setup sensor
	SensorShape suitSensorShape;
	suitSensorShape.SetBox({ 10.0f, 8.0f });
	auto playerSensor = MakeSensor({1.0f, -11.0f}, suitSensorShape);
	playerSensor->SetFiltering(CL_Trigger, CL_Player);
	playerSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
			if(in_beginning) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()), in_other);
			}
		}
		});
	m_playerSensor = playerSensor;
}
Task<> Suit::ManageActor() {
	while(true) {
		if(m_bActive) {
			co_await WaitSeconds(0.02f);
			DeferredDestroy();
		}
		co_await Suspend();
	}
}
void Suit::OnTouchPlayer(std::shared_ptr<Player> in_player, std::shared_ptr<SensorComponent> in_sensor) {
	// Entering-suit transition is only triggerable when player is in Fish state
	if(in_player->IsFish()) {
		// Configure Player to start the handoff from this prop to the actual Player actor
		in_player->SetWorldPos(GetWorldPos());
		in_player->SetDirection(1);
		in_player->SetSuitTransition(true);
		m_bActive = true;
	}
}