#pragma once

#include "GameActor.h"
#include <map>
#include <deque>

class InputComponent;
struct ButtonState;
class Dialogue;

using SceneLine = std::pair<std::wstring, std::wstring>;
using SceneDef = std::deque<SceneLine>;

// Stores a speaker's name and points to all their associated art assets
struct SpeakerDef {
	using tPortraitTable = std::map<std::string, std::string>;
	SpeakerDef() = default;
	SpeakerDef(std::wstring in_name, tPortraitTable in_portraitNames, std::string in_dialogueBkgFilename,
		sf::Color in_headingColor, sf::Color in_lineColor)
		: m_name(std::move(in_name))
		, m_portraitNames(std::move(in_portraitNames))
		, m_dialogueBkgFilename(std::move(in_dialogueBkgFilename))
		, m_headingColor(in_headingColor)
		, m_lineColor(in_lineColor)
	{
	}
	std::wstring m_name;
	tPortraitTable m_portraitNames;
	std::string m_dialogueBkgFilename;
	sf::Color m_headingColor;
	sf::Color m_lineColor;
};

enum class eDialogueSide {
	Left,
	Right,
};

// Represents a speaker in a dialogue scene -- shows/hides art assets, stores screen pos, and teletypes lines.
class DialogueSpeaker : public GameActor {
public:
	DialogueSpeaker(std::shared_ptr<SpeakerDef> in_def, std::shared_ptr<Dialogue> in_instigator);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	std::shared_ptr<SpeakerDef> GetDef() const { return m_def; }
	void SetSide(eDialogueSide in_side);
	void SetScreenPos(Vec2i in_pos);
	void SetPortrait(std::string in_path);
	Task<> ShowSpeaker(float in_duration = 0.5f);
	Task<> HideSpeaker(float in_duration = 0.5f);
	Task<> ShowAdvancePrompt(float in_duration = 0.15f);
	Task<> HideAdvancePrompt(float in_duration = 0.10f);
	Task<> ShowBkg(float in_duration = 0.5f);
	Task<> HideBkg(float in_duration = 0.5f);
	void HideName();
	// Teletypes line onto screen during dialogue
	Task<> PlayLine(const SceneLine& in_line);

private:
	Task<> FadeSprite(std::shared_ptr<SpriteComponent> in_sprite, float in_duration, bool in_fadeToOpaque);
	Task<> TeletypeLine(std::wstring& in_text, std::wstring& in_line, std::shared_ptr<ButtonState> in_buttonState, bool in_firstLine = true);

	std::shared_ptr<SpeakerDef> m_def;
	std::shared_ptr<SpriteComponent> m_portrait;
	std::shared_ptr<SpriteComponent> m_textBkg;
	std::shared_ptr<SpriteComponent> m_advancePrompt;
	std::shared_ptr<TextComponent> m_textHeading;
	std::shared_ptr<TextComponent> m_dialogueLine1;
	std::shared_ptr<TextComponent> m_dialogueLine2;
	std::shared_ptr<InputComponent> m_inputComp;
	std::shared_ptr<ButtonState> m_advanceInput;
	std::shared_ptr<Dialogue> m_instigator;
	Vec2f m_screenPos = Vec2f{0.0f, 0.0f};
};

// A machine for executing a conversation. Feed it a SceneDef and then press the buttons to advance through lines until the end.
class Dialogue : public GameActor {
public:
	Dialogue(SceneDef in_lines, float in_startDelay = 0.0f);
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual Task<> ManageActor() override;

	using tEventFn = std::function<std::optional<Task<>>(std::wstring)>;
	void SetEventFn(tEventFn in_eventFn) { m_eventFn = std::move(in_eventFn); }
	std::shared_ptr<ButtonState> GetAdvanceButton() { return m_advanceInput; }

private:
	std::shared_ptr<SpeakerDef> GetSpeaker(const std::wstring& in_name) const;
	// Sets the first line and activates the Dialogue (typically only used on first line during ManageActor()'s setup)
	void StartLine(int32_t in_lineIndex);
	// Transition visuals (portraits, backgrounds) to correct state for this line, then teletype it to completion
	Task<> ExecuteLine(const SceneLine& in_line);
	// Activates Dialogue execution (i.e. lines will advance automatically until it's done or deactivated)
	void SetActive(bool in_state = true);

	using SpeakerMap = std::map<std::wstring, std::shared_ptr<SpeakerDef>>;

	SpeakerMap m_speakerMap;
	std::shared_ptr<DialogueSpeaker> m_leftSpeaker;
	std::shared_ptr<DialogueSpeaker> m_rightSpeaker;
	std::shared_ptr<DialogueSpeaker> m_prevSpeaker;
	std::shared_ptr<InputComponent> m_inputComp;
	std::shared_ptr<ButtonState> m_advanceInput;

	float m_startDelay = 0.0f;
	tEventFn m_eventFn;
	int32_t m_lineIndex = 0;
	int32_t m_prevIndex = -1;
	int32_t m_totalLines = 0;
	SceneDef m_lines;
	bool m_bDone = false;
	bool m_bActive = true;
};
