#include "FunFactsWidget.h"
#include "GameWorld.h"
#include "PlayerStatus.h"
#include "Engine/Font.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/TextComponent.h"

namespace {
	std::map<int32_t, std::wstring> s_factData = {
		{0,
		L"This is a fun fact for Room 0."
		},
		{1,
		L"Room 1's fact. You have five of these\n"
		"lines, of approximately this length, to\n"
		"work with, and because the font isn't\n"
		"monospaced, test to verify it fits within\n"
		"the actual window in-game. (I didn't! :)\n"
		},
	};
}

void FunFactsWidget::Initialize() {
	GameActor::Initialize();
	// Use current room Id to set initial m_index value
	m_index = GetWorld()->GetEnclosingRoom(GetWorld()->GetPlayerWorldPos())->Id;

	// Setup bkg sprite
	m_bkgSprite = MakeSprite();
	m_bkgSprite->GetActor()->SetDrawLayer(4);
	m_bkgSprite->PlayAnim("FactBkg/FactBkg", false, 0);
	m_bkgSprite->SetRelativePos({ 9.0f, -160.0f });

	// Setup widget heading 0
	m_headingText0 = MakeText(Transform::Identity);
	m_headingText0->GetActor()->SetDrawLayer(4);
	m_headingText0->SetFont("PixicaMicro-Regular");
	auto font = m_headingText0->GetFont();
	font->SetSmooth(false);
	m_headingText0->SetFontSizePixels(16);
	auto roomNumberText = m_index < 9 ? L"0" + std::to_wstring(m_index + 1) : std::to_wstring(m_index + 1);
	m_headingText0->SetText(L"ROOM " + roomNumberText);
	m_headingText0->SetAlignment({ 0.0f, 0.0f });
	m_headingText0->SetRelativePos({ 19.0f, -162.0f });
	m_headingText0->SetColor(sf::Color{ 255, 255, 255, 255 });

	// Setup widget heading 1
	m_headingText1 = MakeText(Transform::Identity);
	m_headingText1->GetActor()->SetDrawLayer(4);
	m_headingText1->SetFont("PixicaMicro-Regular");
	font = m_headingText1->GetFont();
	font->SetSmooth(false);
	m_headingText1->SetFontSizePixels(16);
	auto roomTotalText = L" / " + std::to_wstring(s_factData.size());
	m_headingText1->SetText(roomTotalText);
	m_headingText1->SetAlignment({ 0.0f, 0.0f });
	m_headingText1->SetRelativePos({ 53.0f, -162.0f });
	m_headingText1->SetColor(sf::Color{ 100, 176, 255, 255 });

	// Setup fact text
	m_factText = MakeText(Transform::Identity);
	m_factText->GetActor()->SetDrawLayer(4);
	m_factText->SetFont("Pixica-Regular");
	font = m_factText->GetFont();
	font->SetSmooth(false);
	m_factText->SetFontSizePixels(16);
	auto factText = s_factData.find(m_index);
	if(factText != s_factData.end()) {
		m_factText->SetText(factText->second);
	}
	m_factText->SetAlignment({ 0.0f, 0.0f });
	m_factText->SetRelativePos({ 15.0f, -172.0f });
	m_factText->SetColor(sf::Color{ 234, 158, 34, 255 });
	m_factText->SetLineSpacingFactor(0.8f);

	// Setup Jibit sprites
	for(auto i = 0; i < s_factData.size(); i++) {
		CreateJibit();
	}
	UpdateJibits();
	SetTimeStream(eTimeStream::Real);
}
void FunFactsWidget::CreateJibit() {
	auto spriteComponent = MakeSprite();
	spriteComponent->GetActor()->SetDrawLayer(4);
	spriteComponent->SetPlayRate(0.0f);
	//spriteComponent->PlayAnim("FactJibit/Jibit", false, 0);
	spriteComponent->SetRelativePos({ 80.0f + m_jibitSprites.size() * 4.0f, -158.0f });
	m_jibitSprites.push_back(spriteComponent);
}
void FunFactsWidget::UpdateJibits() {
	auto playerStatus = GetWorld()->GetPlayerStatus();
	m_unlockedIds.clear();
	for(auto factDatum : s_factData) {
		m_jibitSprites[factDatum.first]->PlayAnim("FactJibit/Jibit", true, 0);
		if(playerStatus->IsRoomUnlocked(factDatum.first)) {
			m_jibitSprites[factDatum.first]->PlayAnim("FactJibit/Jibit", true, 1);
			m_unlockedIds.push_back(factDatum.first);
		}
		if(m_index == factDatum.first) {
			m_jibitSprites[factDatum.first]->PlayAnim("FactJibit/Jibit", true, 2);
		}
	}
}
void FunFactsWidget::ChangeIndex(bool in_rightwards) {
	auto offset = in_rightwards ? 1 : -1;
	auto foundIndex = false;
	if(m_unlockedIds.size() > 1) {
		for(auto index = m_index + offset; index < s_factData.size() && index >= 0; (index += offset)) {
			if(foundIndex) {
				break;
			}
			for(auto id : m_unlockedIds) {
				if(index == id) {
					m_index = index;
					foundIndex = true;
					break;
				}
			}
		}
	}
	auto roomNumberText = m_index < 9 ? L"0" + std::to_wstring(m_index + 1) : std::to_wstring(m_index + 1);
	m_headingText0->SetText(L"ROOM " + roomNumberText);
	auto factText = s_factData.find(m_index);
	if(factText != s_factData.end()) {
		m_factText->SetText(factText->second);
	}
	UpdateJibits();
}
