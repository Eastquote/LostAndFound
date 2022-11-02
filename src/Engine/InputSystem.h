#pragma once

#include "Vec2.h"
#include "Object.h"
#include "TokenList.h"
#include "HistoryBuffer.h"
#include "MinMax.h"

#include <set>
#include <map>
#include <string>
#include <functional>
#include <cassert>

namespace sf
{
	class Event;
}

// Mappable Inputs
enum class eText
{
	Keyboard,

	NUM_INPUTS,
};
enum class eCursor
{
	RelativeMouse,
	AbsoluteMouse, // NOTE: Overrides all other cursor inputs

	NUM_INPUTS,
};
enum class eAxis
{
	RightStick,
	LeftStick,
	DPAD,
	WASD,
	ArrowKeys,

	RightTrigger,
	LeftTrigger,

	MouseWheel,

	NUM_INPUTS,
};
enum class eButton
{
	// Keyboard
	Keyboard_Unknown = -1,
	A = 0, B, C,
	D, E, F, G,
	H, I, J, K,
	L, M, N, O,
	P, Q, R, S,
	T, U, V, W,
	X, Y, Z, Num0,
	Num1, Num2, Num3, Num4,
	Num5, Num6, Num7, Num8,
	Num9, Escape, LControl, LShift,
	LAlt, LSystem, RControl, RShift,
	RAlt, RSystem, Menu, LBracket,
	RBracket, Semicolon, Comma, Period,
	Quote, Slash, Backslash, Tilde,
	Equal, Hyphen, Space, Enter,
	Backspace, Tab, PageUp, PageDown,
	End, Home, Insert, Delete,
	Add, Subtract, Multiply, Divide,
	Left, Right, Up, Down,
	Numpad0, Numpad1, Numpad2, Numpad3,
	Numpad4, Numpad5, Numpad6, Numpad7,
	Numpad8, Numpad9, F1, F2,
	F3, F4, F5, F6,
	F7, F8, F9, F10,
	F11, F12, F13, F14,
	F15, Pause, KeyCount, Dash,
	BackSpace, BackSlash, SemiColon, Return,
	Keyboard_MAX,

	// Mouse
	Mouse_Unknown,
	Mouse_Left, Mouse_Right, Mouse_Middle,
	Mouse_MAX,

	// Joystick
	Joy_Unknown,
	Joy_DiamondBottom, Joy_DiamondRight, Joy_DiamondLeft, Joy_DiamondTop,
	Joy_Select, Joy_Start,
	Joy_LeftBumper, Joy_RightBumper,
	Joy_LeftTrigger, Joy_RightTrigger,
	Joy_LeftStick, Joy_RightStick,
	Joy_Up, Joy_Down, Joy_Left, Joy_Right,
	Joy_TouchPad,
	Joy_MAX,

	NUM_INPUTS,
};

// Input States
template <typename tInputEnum>
struct InputState
{
	InputState(const std::string& in_name, const std::vector<tInputEnum>& in_inputs, bool in_consumeInput = true)
		: m_name(in_name)
		, m_inputs(in_inputs)
		, m_consumeInput(in_consumeInput)
	{
	}
	virtual ~InputState() {}

protected:
	friend class InputComponent;
	virtual void Update() {}
	std::string m_name;
	std::vector<tInputEnum> m_inputs;
	bool m_consumeInput = true;
};
struct TextState : public InputState<eText>
{
	using InputState<eText>::InputState;

	bool HasText() const { return text.size(); }

	std::wstring text;
	std::wstring lastText;
};
struct CursorState : public InputState<eCursor>
{
	using InputState<eCursor>::InputState;

	bool JustMoved() const { return pos.Dist(lastPos) > 0.0f; }

	Vec2f pos = Vec2f::Zero;
	Vec2f lastPos = Vec2f::Zero;

private:
	friend class InputComponent;
	Vec2f m_lastMousePos = Vec2f::Zero;
};
struct AxisState : public InputState<eAxis>
{
	using InputState<eAxis>::InputState;

	bool HasInput(float in_threshold = 0.0f) const { return GetAmount().Len() > in_threshold; }
	bool HasInputRaw(float in_threshold = 0.05f) const { return m_amountRaw.Len() > in_threshold; }

	Vec2f GetAmount() const		{ return MapRawInput(m_amountRaw); }
	Vec2f GetLastAmount() const { return MapRawInput(m_lastAmountRaw); }

	Vec2f GetAmountRaw() const { return m_amountRaw; }
	Vec2f GetLastAmountRaw() const { return m_lastAmountRaw; }

	// radial deadzone and exponent. if deadzones is unset, exponent will also be ignored.
	std::optional<MinMaxf> deadzones = {};
	float exponent = 1.0f;

	void SetAmount(Vec2f in_newAmount)
	{
		m_lastAmountRaw = m_amountRaw;
		m_amountRaw = in_newAmount;
	}

private:
	Vec2f MapRawInput(Vec2f in_raw) const { 
		if (deadzones) {
			return m_amountRaw.Trunc(std::powf(deadzones->MapUnitRange(m_amountRaw.Len()), exponent));
		}
		else {
			return m_amountRaw;
		}
	}

	Vec2f m_amountRaw = Vec2f::Zero;
	Vec2f m_lastAmountRaw = Vec2f::Zero;
};

struct ButtonStateBase : public InputState<eButton>
{
	using InputState<eButton>::InputState;

	virtual bool IsPressed() const = 0;
	virtual bool WasPressed() const = 0;
	virtual bool JustPressed() const = 0;
	virtual bool JustReleased() const = 0;

protected:
	friend class InputComponent;
	virtual void SetPressed(bool in_state) = 0;
};

struct ButtonState : public ButtonStateBase
{
	using ButtonStateBase::ButtonStateBase;

	virtual bool IsPressed() const override {
		return m_isPressed;
	}
	virtual bool WasPressed() const override {
		return m_wasPressed;
	}
	virtual bool JustPressed() const override {
		return IsPressed() && !WasPressed();
	}
	virtual bool JustReleased() const override {
		return !IsPressed() && WasPressed();
	}

protected:
	virtual void SetPressed(bool in_state) override {
		m_wasPressed = m_isPressed;
		m_isPressed = in_state;
	}
	bool m_isPressed = false;
	bool m_wasPressed = false;
};

// Input Component
class InputComponent : public Object
{
public:
	virtual void Initialize() override;
	virtual void Destroy() override;

	void SetPriority(int32_t in_priority);
	int32_t GetPriority() const;

	template <typename tButtonState = ButtonState, typename... tArgs>
	std::shared_ptr<tButtonState> Button(const std::string& in_name, const std::vector<eButton>& in_inputs, tArgs... in_args)
	{
		auto state = std::make_shared<tButtonState>(in_name, in_inputs, in_args...);
		m_buttonStates[in_name] = std::static_pointer_cast<ButtonStateBase>(state);
		return state;
	}

	std::shared_ptr<TextState> Text(const std::string& in_name, const std::vector<eText>& in_inputs, bool in_consumeInput = true);
	std::shared_ptr<CursorState> Cursor(const std::string& in_name, const std::vector<eCursor>& in_inputs, bool in_consumeInput = true);
	std::shared_ptr<AxisState> Axis(const std::string& in_name, const std::vector<eAxis>& in_inputs, bool in_consumeInput = true);

	std::shared_ptr<TextState> GetText(const std::string& in_name);
	std::shared_ptr<CursorState> GetCursor(const std::string& in_name);
	std::shared_ptr<AxisState> GetAxis(const std::string& in_name);
	std::shared_ptr<ButtonStateBase> GetButton(const std::string& in_name);

private:
	friend class InputSystem;
	int32_t m_priority = 0;

	void Update(InputSystem* in_inputSys, std::set<int32_t>& in_consumedInputs);

	// States
	std::map<std::string, std::shared_ptr<TextState>> m_textStates;
	std::map<std::string, std::shared_ptr<CursorState>> m_cursorStates;
	std::map<std::string, std::shared_ptr<AxisState>> m_axisStates;
	std::map<std::string, std::shared_ptr<ButtonStateBase>> m_buttonStates;
};

// Input System
class InputSystem
{
public:
	void HandleEvent(sf::Event* in_event);
	void Update();

	TokenList<> m_hijackCursor;

private:
	friend class InputComponent;
	void AddInputComponent(std::shared_ptr<InputComponent> in_comp);
	std::vector<std::shared_ptr<InputComponent>> m_comps;
	std::wstring m_textAccum;
	float m_wheelAccum = 0.0f;
};


// TODO: Move into its own header (too specific for the Engine):
struct ButtonStateWithHistory : public ButtonStateBase
{
public:
	using ButtonStateBase::ButtonStateBase;

	bool IsPressed() const { return WasPressedAtPastFrame(0); }
	bool WasPressed() const { return WasPressedAtPastFrame(1); }

	bool JustPressed() const {
		return IsPressed() && !WasPressedAtPastFrame(1) && m_consumedTime < GetCurTime();
	}
	bool JustPressedRaw() const {
		return IsPressed() && !WasPressedAtPastFrame(1);
	}
	bool JustReleased() const {
		return !IsPressed() && WasPressedAtPastFrame(1);
	}

	bool JustPressed(float in_seconds) const {
		for(int frameIdx = 0;
			frameIdx < m_pressedHistory.NumItems() - 1
			&& m_pressedHistory.GetTimeAtPastFrame(frameIdx) > m_consumedTime
			&& m_pressedHistory.GetTimeAtPastFrame(frameIdx) >= GetCurTime() - in_seconds
			; frameIdx++)
		{
			if(WasPressedAtPastFrame(frameIdx) && !WasPressedAtPastFrame(frameIdx + 1))
			{
				return true;
			}
		}
		return false;
	}
	bool JustReleased(float in_seconds) const {
		for(int frameIdx = 0;
			frameIdx < m_pressedHistory.NumItems() - 1
			&& m_pressedHistory.GetTimeAtPastFrame(frameIdx) > m_consumedTime
			&& m_pressedHistory.GetTimeAtPastFrame(frameIdx) >= GetCurTime() - in_seconds
			; frameIdx++)
		{
			if(!WasPressedAtPastFrame(frameIdx) && WasPressedAtPastFrame(frameIdx + 1))
			{
				return true;
			}
		}
		return false;
	}

	float TimeHeld() const {
		return IsPressed() ? m_stateAge : 0.0f;
	}
	float TimeSincePressed() const {
		return IsPressed() ? 0.0f : m_stateAge;
	}

	void Consume() {
		m_consumedTime = GetCurTime();
	}

	// used internally by the input system:
	void Stomp(bool NewState) {
		m_pressedHistory.Stomp(NewState);

		if(NewState != WasPressedAtPastFrame(1)) m_stateAge = 0.0f;
	}
	virtual void Update() override {
		if(IsPressed() == WasPressed()) {
			m_stateAge += Time::DT();
		}
		else m_stateAge = 0.0f;
	}
	virtual void SetPressed(bool in_state) override {
		m_pressedHistory.Push(in_state);
	}
	void SetRealTime(bool in_bRealTime)
	{
		m_bRealTime = in_bRealTime;
		m_pressedHistory = HistoryBuffer_WithTimes<bool>(m_historySize);
	}

private:
	static const int m_historySize = 128;
	HistoryBuffer_WithTimes<bool> m_pressedHistory = HistoryBuffer_WithTimes<bool>(m_historySize);
	float m_consumedTime = -1.0f;
	float m_stateAge = 0.0f;
	bool m_bRealTime = false;

	float GetCurTime() const { return m_bRealTime ? Time::RealTime() : Time::Time(); }

	bool WasPressedAtPastFrame(int in_frameIdx) const {
		if(in_frameIdx >= m_pressedHistory.NumItems()) return false;
		return m_pressedHistory.GetValAtPastFrame(in_frameIdx);
	}
};
