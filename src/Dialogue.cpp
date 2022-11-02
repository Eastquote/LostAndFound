#include "Dialogue.h"

#include <string>

#include "AudioManager.h"
#include "Engine/InputSystem.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/GameWindow.h"
#include "Engine/MathEasings.h"
#include "GameWorld.h"
#include "Hud.h"

//--- SPEAKER DEFS ---//
// (NOTE: In the future, I would definitely pull this data out into JSON files)

auto gariSpkr = std::make_shared<SpeakerDef>(
	L"DANIEL",
	SpeakerDef::tPortraitTable{
		{"neutral", "PortraitsGari/neutral"},
		{"neutralTalk", "PortraitsGari/neutralTalk"},
		{"worried", "PortraitsGari/worried"},
		{"worriedTalk", "PortraitsGari/worriedTalk"},
		{"happy", "PortraitsGari/happy"},
		{"happyTalk", "PortraitsGari/happyTalk"},
	},
	"DialogueBkg/Gari",
	sf::Color(158, 40, 53, 255),
	sf::Color(254, 174, 52, 255)
);
auto annaSpkr = std::make_shared<SpeakerDef>(
	L"GERALDINE",
	SpeakerDef::tPortraitTable{
		{"neutral", "PortraitsAnna/neutral"},
		{"neutralTalk", "PortraitsAnna/neutralTalk"},
		{"worried", "PortraitsAnna/worried"},
		{"worriedTalk", "PortraitsAnna/worriedTalk"},
		{"happy", "PortraitsAnna/happy"},
		{"happyTalk", "PortraitsAnna/happyTalk"},
	},
	"DialogueBkg/Anna",
	sf::Color(38, 92, 66, 255),
	sf::Color(99, 199, 77, 255)
);

//--- SPEAKER CODE ---//

DialogueSpeaker::DialogueSpeaker(std::shared_ptr<SpeakerDef> in_def, std::shared_ptr<Dialogue> in_instigator)
	: m_def(in_def)
	, m_instigator(in_instigator)
{
	SetUpdateStage(eUpdateStage::Final);
}
void DialogueSpeaker::Initialize() {
	GameActor::Initialize();

	// Setup sprites
	SetDrawLayer(4);
	m_portrait = MakeSprite(Transform::Identity);
	m_portrait->SetRenderLayer("hud");
	m_textBkg = MakeSprite(Transform::Identity);
	m_textBkg->SetRenderLayer("hud");
	m_portrait->SetColor(255, 255, 255, 0);
	m_textBkg->SetColor(255, 255, 255, 0);
	m_textBkg->SetRelativePos({ 170.0f, -12.0f });
	m_textBkg->PlayAnim(m_def->m_dialogueBkgFilename, true);
	m_advancePrompt = MakeSprite({ 265.0f, -31.0f });
	m_advancePrompt->SetRenderLayer("hud");
	m_advancePrompt->PlayAnim("FaceButtonPrompt/DiamondLeft", true);
	m_advancePrompt->SetColor(255, 255, 255, 0);

	// Setup text
	m_textHeading = MakeText(Transform::Identity);
	m_textHeading->SetRenderLayer("hud");
	m_textHeading->SetFont("Pixica-Bold");
	m_textHeading->SetFontSizePixels(16);
	m_dialogueLine1 = MakeText({ 73.0f, -15.0f });
	m_dialogueLine1->SetRenderLayer("hud");
	m_dialogueLine1->SetFont("cc.yal.6w4");
	m_dialogueLine1->SetFontSizePixels(16);
	m_dialogueLine2 = MakeText({ 73.0f, -25.0f });
	m_dialogueLine2->SetRenderLayer("hud");
	m_dialogueLine2->SetFont("cc.yal.6w4");
	m_dialogueLine2->SetFontSizePixels(16);
	m_textHeading->SetColor(m_def->m_headingColor);
	m_dialogueLine1->SetColor(m_def->m_lineColor);		
	m_dialogueLine2->SetColor(m_def->m_lineColor);
}
Task<> DialogueSpeaker::ManageActor() {
	auto world = GetWorld();
	Vec2f worldPos = Vec2f::Zero;
	while(true) {
		worldPos = world->ViewToWorld(m_screenPos);
		SetWorldPos(worldPos);
		co_await Suspend();
	}
}
void DialogueSpeaker::SetSide(eDialogueSide in_side) {
	if(in_side == eDialogueSide::Right) {
		m_portrait->SetRelativePos({292.0f, -2.0f});
		m_textHeading->SetAlignment({ 1.0f, 0.0f });
		m_textHeading->SetRelativePos({ 268.0f, -4.0f });
		m_portrait->SetFlipHori(true);
		m_textBkg->SetFlipHori(true);
	}
	else {
		m_portrait->SetRelativePos({ 47.0f, -2.0f});
		m_textHeading->SetRelativePos({ 72.0f, -4.0f });
	}
}
void DialogueSpeaker::SetScreenPos(Vec2i in_pos) {
	m_screenPos = in_pos;
}
void DialogueSpeaker::SetPortrait(std::string in_path) {
	m_portrait->PlayAnim(m_def->m_portraitNames.find(in_path)->second, true);
}
Task<> DialogueSpeaker::ShowSpeaker(float in_duration) {
	co_await FadeSprite(m_portrait, in_duration, true);
}
Task<> DialogueSpeaker::HideSpeaker(float in_duration) {
	co_await FadeSprite(m_portrait, in_duration, false);
}
Task<> DialogueSpeaker::ShowAdvancePrompt(float in_duration) {
	co_await FadeSprite(m_advancePrompt, in_duration, true);
}
Task<> DialogueSpeaker::HideAdvancePrompt(float in_duration) {
	co_await FadeSprite(m_advancePrompt, in_duration, false);
}
Task<> DialogueSpeaker::ShowBkg(float in_duration) {
	co_await FadeSprite(m_textBkg, in_duration, true);
}
Task<> DialogueSpeaker::HideBkg(float in_duration) {
	co_await FadeSprite(m_textBkg, in_duration, false);
}
void DialogueSpeaker::HideName() { 
	m_textHeading->SetText(L"");
}
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
		if(float delay = in_buttonState->IsPressed() ? 0.0f : teletypeDelay) { //< Lets players speed it up
			co_await WaitSeconds(delay);
		}
		co_await Suspend();
	}
}
Task<> DialogueSpeaker::FadeSprite(std::shared_ptr<SpriteComponent> in_sprite, float in_duration, bool in_fadeToOpaque) {
	auto world = GetWorld();
	auto fadeTime = in_duration;
	auto elapsedTime = 0.0f;
	int32_t targetAlpha = in_fadeToOpaque ? 255 : 0;
	while(true) {
		elapsedTime += DT();
		auto fadeAlpha = int32_t(world->EaseAlpha(elapsedTime, fadeTime, Math::EaseInOutSmoothstep) * 255.0f);
		if(!in_fadeToOpaque) {
			fadeAlpha = 255 - fadeAlpha;
		}
		in_sprite->SetColor(255, 255, 255, (uint8_t)fadeAlpha);
		if(elapsedTime >= fadeTime) {
			co_return;
		}
		co_await Suspend();
	}
}

//--- DIALOGUE CODE ---//

Dialogue::Dialogue(SceneDef in_lines, float in_startDelay)
	: m_lines(std::move(in_lines))
	, m_startDelay(in_startDelay)
{
}
void Dialogue::Initialize() {
	GameActor::Initialize();
	m_inputComp = MakeChild<InputComponent>();
	m_inputComp->SetPriority(10);
	m_advanceInput = m_inputComp->Button<ButtonState>("AdvanceInput", { eButton::Joy_DiamondLeft, eButton::Joy_DiamondBottom, 
																		eButton::X });
	m_speakerMap = {
		{L"DANIEL", gariSpkr},
		{L"GERALDINE", annaSpkr}
	};

	// Inspect first two lines (which are just metadata) to determine who's speaking and from which side
	m_leftSpeaker = Spawn<DialogueSpeaker>(Transform::Identity, GetSpeaker(m_lines[0].first), AsShared<Dialogue>());
	m_rightSpeaker = Spawn<DialogueSpeaker>(Transform::Identity, GetSpeaker(m_lines[1].first), AsShared<Dialogue>());
	m_leftSpeaker->SetSide(eDialogueSide::Left);
	m_rightSpeaker->SetSide(eDialogueSide::Right);
	m_prevSpeaker = m_leftSpeaker;

	// Now that we have the speaker ID's, remove those first two metadata-bearing lines of the scene
	m_lines.pop_front();
	m_lines.pop_front();
}
void Dialogue::Destroy() {
	GameActor::Destroy();
}
Task<> Dialogue::ManageActor() {
	auto hud = GetWorld()->GetHud();
	co_await WaitSeconds(m_startDelay);
	co_await m_taskMgr.RunManaged(hud->InterpLetterboxVisibility(true, true, true, 0.5f));
	StartLine(0);
	while(!m_bDone) {
		// Execute each line in sequence until they're all done
		if(m_bActive && m_lineIndex != m_prevIndex) {
			auto line = m_lines[m_lineIndex];
			co_await ExecuteLine(line);
			m_prevIndex = m_lineIndex;
			m_lineIndex++;
		}
		co_await Suspend();
	}
	co_await hud->InterpLetterboxVisibility(false, true, true, 2.0f);
	DeferredDestroy();
}
std::shared_ptr<SpeakerDef> Dialogue::GetSpeaker(const std::wstring& in_name) const {
	auto result = m_speakerMap.find(in_name);
	if(result != m_speakerMap.end()) {
		return result->second;
	}
	printf("DIALOGUE ERROR: Couldn't find Speaker in m_speakerMap!");
	return m_speakerMap.begin()->second;
}
void Dialogue::StartLine(int32_t in_lineIndex) {
	m_lineIndex = in_lineIndex;
	m_prevIndex = in_lineIndex - 1;
	SetActive();
}
Task<> Dialogue::ExecuteLine(const SceneLine& in_line){
	// Handle non-dialogue setup events here
	if(m_eventFn && (in_line.first == L"*")) {
		if(auto eventTask = m_eventFn(in_line.second)) {
			co_await std::move(eventTask.value());
		}
		co_return;
	}

	// Actual dialogue setup (defaults to left speaker going first)
	auto speaker = m_leftSpeaker;
	auto nonSpeaker = m_rightSpeaker;

	// Set speaker on right to go first if they've got the first line
	if(in_line.first == m_rightSpeaker->GetDef()->m_name) {
		speaker = m_rightSpeaker;
		nonSpeaker = m_leftSpeaker;
	}
	speaker->SetPortrait("neutralTalk");
	nonSpeaker->SetPortrait("neutral");

	// Fade visuals in if this is line one
	if(m_lineIndex == 0) {
		auto showSpeakerTask = m_taskMgr.Run(speaker->ShowSpeaker());
		auto showSpeakerBkgTask = m_taskMgr.Run(speaker->ShowBkg());
		co_await WaitForAll({
			showSpeakerTask,
			showSpeakerBkgTask,
			});
		m_prevSpeaker = speaker;
	}

	// Swap speakers as necessary
	if(speaker != m_prevSpeaker) {
		m_prevSpeaker->HideName();
		auto hidePrevSpeakerTask = m_taskMgr.RunManaged(m_prevSpeaker->HideSpeaker(0.2f));
		auto showSpeakerTask = m_taskMgr.RunManaged(speaker->ShowSpeaker(0.2f));
		co_await m_prevSpeaker->HideBkg(0.1f);
		co_await speaker->ShowBkg();
	}
	m_prevSpeaker = speaker;
	co_await speaker->PlayLine(in_line);

	// When out of lines, hide visuals gracefully and tell the Dialogue it's time to wrap things up
	if(m_lineIndex == m_lines.size() - 1) {
		m_prevSpeaker->HideName();
		auto showSpeakerTask = m_taskMgr.RunManaged(speaker->HideSpeaker(0.5f));
		auto showSpeakerBkgTask = m_taskMgr.RunManaged(speaker->HideBkg(0.5f));
		m_bDone = true;
	}
}
void Dialogue::SetActive(bool in_state) {
	m_bActive = in_state;
}
