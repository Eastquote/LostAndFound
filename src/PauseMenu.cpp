#include "PauseMenu.h"

#include "TokenList.h"
#include "MenuItemDefs.h"
#include "GameWorld.h"
#include "AudioManager.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ShapeComponent.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/InputSystem.h"
#include "Engine/Font.h"
#include "Engine/GameWindow.h"
#include "Engine/Game.h"

//--- PAUSE MENU CODE ---//

void PauseMenu::Initialize() {
	GameActor::Initialize();
	auto world = GetWorld();

	// Setup bkg
	auto worldView = GetWindow()->GetWorldView();
	SetDrawLayer(4);
	m_bkgQuad = MakeSprite(Transform::Identity);
	m_bkgQuad->SetRenderLayer("hud");
	m_bkgQuad->SetComponentDrawOrder(0);
	m_bkgQuad->PlayAnim("Square/10px", true);
	m_bkgQuad->SetWorldScale({ 34.4f, 24.4f });
	m_bkgQuad->SetColor(0, 0, 0, 175);
	m_bkgQuad->SetWorldPos(world->ViewToWorld(Vec2f::Zero));

	// Setup menu heading
	m_headingTextComp = MakeText(Transform::Identity);
	m_headingTextComp->SetFont("Pixica-Bold");
	m_headingTextComp->SetRenderLayer("hud");
	m_headingTextComp->SetComponentDrawOrder(10);
	auto font = m_headingTextComp->GetFont();
	font->SetSmooth(false);
	m_headingTextComp->SetFontSizePixels(16);
	m_headingTextComp->SetText(L"PAUSED");
	m_headingTextComp->SetAlignment({ 0.5f, 0.5f });
	m_headingTextComp->SetWorldPos(world->ViewToWorld({ worldView.w / 2.0f, 57.0f }));
	m_headingTextComp->SetColor(sf::Color{ 234, 158, 34, 255 });

	// Setup MenuItems
	auto resumeItem = Actor::Spawn<MenuItem>(Transform::Identity, ui_ResumeItem, 0);
	resumeItem->SetWorldPos(world->ViewToWorld({ worldView.w / 2.0f, 95.0f }));
	resumeItem->SetActive(true);
	m_menuItems.push_back(resumeItem);
	auto mainMenuItem = Actor::Spawn<MenuItem>(Transform::Identity, ui_MainMenuItem, 1);
	mainMenuItem->SetWorldPos(world->ViewToWorld({ worldView.w / 2.0f, 117.0f }));
	m_menuItems.push_back(mainMenuItem);
}
Task<> PauseMenu::ManageActor() {
	SetTimeStream(eTimeStream::Real);
	SetPause(true);
	auto prevIndex = 0;

	// Setup input
	m_inputComp = MakeChild<InputComponent>();
	m_inputComp->SetPriority(10);
	auto pauseInput = m_inputComp->Button<ButtonStateWithHistory>("Start", { eButton::Joy_Start, eButton::Escape }, false);
	auto rightInput = m_inputComp->Button<ButtonStateWithHistory>("Right", { eButton::Joy_Right, eButton::Right }, false);
	auto leftInput = m_inputComp->Button<ButtonStateWithHistory>("Left", { eButton::Joy_Left, eButton::Left }, false);
	auto upInput = m_inputComp->Button<ButtonStateWithHistory>("Up", { eButton::Joy_Up, eButton::Up }, false);
	auto downInput = m_inputComp->Button<ButtonStateWithHistory>("Down", {	eButton::Joy_Down, eButton::Down, 
																			eButton::Joy_Select, eButton::S }, false);
	auto actionInput = m_inputComp->Button<ButtonStateWithHistory>("Action", {	eButton::Joy_DiamondLeft, eButton::X, 
																				eButton::Joy_Start, eButton::D, 
																				eButton::Joy_DiamondBottom, eButton::Z }, false);
	auto cancelInput = m_inputComp->Button<ButtonStateWithHistory>("Cancel", { eButton::Escape, eButton::Joy_DiamondRight }, false);
	pauseInput->SetRealTime(true);
	rightInput->SetRealTime(true);
	leftInput->SetRealTime(true);
	upInput->SetRealTime(true);
	downInput->SetRealTime(true);
	actionInput->SetRealTime(true);
	cancelInput->SetRealTime(true);

	// Setup caratSprites
	auto caratSpriteL = Guard(MakeSprite());
	caratSpriteL->SetRenderLayer("hud");
	caratSpriteL->PlayAnim("MenuCarat/Carat", true);
	auto caratSpriteR = Guard(MakeSprite());
	caratSpriteR->SetRenderLayer("hud");
	caratSpriteR->PlayAnim("MenuCarat/Carat", true);
	caratSpriteR->SetFlipHori(true);
	co_await Suspend(); //< HACK: wait one frame for menuItems to initialize, and therefore have an actual Bounds to Get()
	auto menuItemBounds = m_menuItems[m_index]->GetBounds();
	auto caratOffset = std::round(m_menuItems[m_index]->GetBounds().w / 2) + 8.0f;
	caratSpriteL->SetWorldPos(m_menuItems[m_index]->GetWorldPos() + Vec2f{ -caratOffset, 0.0f });
	caratSpriteR->SetWorldPos(m_menuItems[m_index]->GetWorldPos() + Vec2f{ caratOffset, 0.0f });

	auto menuLive = false;
	auto framecounter = 0;
	auto interpTimer = 0.0f;
	while(true) {
		if(!menuLive) {
			if(framecounter == 1) {
				co_await WaitUntil([pauseInput] {
					return !pauseInput->IsPressed();
					});
				menuLive = true;
			}
			framecounter++;
		}
		if(menuLive) {
			// Detect up/down input, change index accordingly
			if(downInput->JustPressed()) {
				m_index += 1;
				if(m_index > 1) {
					m_index = 0;
				}
			}
			else if(upInput->JustPressed()) {
				m_index -= 1;
				if(m_index < 0) {
					m_index = 1;
				}
			}

			// Change which item is selected if necessary
			if(m_index != prevIndex) {
				for(auto item : m_menuItems) {
					if(item->MatchIndex(m_index)) {
						item->SetActive(true);
						item->TryHighlight();
						caratOffset = std::round(item->GetBounds().w / 2) + 8.0f;
						caratSpriteL->SetWorldPos(item->GetWorldPos() + Vec2f{ -caratOffset, 0.0f });
						caratSpriteR->SetWorldPos(item->GetWorldPos() + Vec2f{ caratOffset, 0.0f });
					}
					else if(item->MatchIndex(prevIndex)) {
						item->SetActive(false);
						item->TryUnhighlight();
					}
					else {
						item->SetActive(false);
					}
				}
			}

			// Listen for activation
			if(actionInput->JustPressed()) {
				for(auto item : m_menuItems) {
					if(item->MatchIndex(m_index)) {
						if(m_index == 0) {
							SetPause(false);
							//m_confirmWidget->TryActivate();
						}
						else if(m_index == 1) {
							GetWorld()->GameOverRequest();
							SetPause(false);
						}
						else {
							item->TryActivate();
						}
					}
				}
			}
			prevIndex = m_index;
		}
		co_await Suspend();
	}
}
void PauseMenu::SetPause(bool bPaused)
{
	if(bPaused)
	{
		if(!m_pauseToken)
		{
			m_pauseToken = GameBase::Get()->ShouldPause.TakeToken(__FUNCTION__);
		}
		GameBase::Get()->ShouldPause.AddToken(m_pauseToken);
		AudioManager::Get()->PauseMusic();
		AudioManager::Get()->PlaySound("Pause");
	}
	else
	{
		AudioManager::Get()->ResumeMusic();
		GameBase::Get()->ShouldPause.RemoveToken(m_pauseToken);
		printf("PAUSEMENU: Destroyed!\n");
		DeferredDestroy();
	}
}
