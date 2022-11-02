#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Window.hpp>

#include "imgui.h"

#include <string>

namespace sf 
{
	class Event;
	class Texture;
}

class GameWindow;

// mostly copied from https://github.com/eliasdaler/imgui-sfml

class ImguiIntegration
{
public:
	ImguiIntegration(const GameWindow& in_window);
	~ImguiIntegration();

	void HandleEvent(const sf::Event& in_event);
	void Update(GameWindow& in_window);
	void Draw(GameWindow& in_window);

private:
	void UpdateFontTexture();
	sf::Texture& GetFontTexture();

	void SetActiveJoystickId(unsigned int in_joystickId);
	void SetJoytickDPadThreshold(float in_threshold);
	void SetJoytickLStickThreshold(float in_threshold);

	void SetJoystickMapping(int in_action, unsigned int in_joystickButton);
	void SetDPadXAxis(sf::Joystick::Axis in_dPadXAxis, bool in_inverted = false);
	void SetDPadYAxis(sf::Joystick::Axis in_dPadYAxis, bool in_inverted = false);
	void SetLStickXAxis(sf::Joystick::Axis in_lStickXAxis, bool in_inverted = false);
	void SetLStickYAxis(sf::Joystick::Axis in_lStickYAxis, bool in_inverted = false);

	ImGuiContext* m_ctx = nullptr;

	bool m_windowHasFocus = false;
	bool m_mousePressed[3] = { false, false, false };
	bool m_touchDown[3] = { false, false, false };
	bool m_mouseMoved = false;
	sf::Vector2i m_touchPos;
	std::shared_ptr<sf::Texture> m_fontTexture = nullptr;

	const unsigned int NULL_JOYSTICK_ID = sf::Joystick::Count;
	unsigned int m_joystickId = NULL_JOYSTICK_ID;

	const unsigned int NULL_JOYSTICK_BUTTON = sf::Joystick::ButtonCount;
	unsigned int m_joystickMapping[ImGuiNavInput_COUNT];

	struct StickInfo {
		sf::Joystick::Axis xAxis;
		sf::Joystick::Axis yAxis;

		bool xInverted;
		bool yInverted;

		float threshold;
	};

	StickInfo m_dPadInfo;
	StickInfo m_lStickInfo;

	void SetupRenderState(ImDrawData* in_draw_data, int in_fb_width, int in_fb_height);
	void RenderDrawLists(ImDrawData* in_draw_data); 
	
	// Default mapping is XInput gamepad mapping
	void initDefaultJoystickMapping();

	// Returns first id of connected joystick
	unsigned int getConnectedJoystickId();

	void updateJoystickActionState(ImGuiIO& in_io, ImGuiNavInput_ in_action);
	void updateJoystickDPadState(ImGuiIO& in_io);
	void updateJoystickLStickState(ImGuiIO& in_io);

	// clipboard functions
	void setClipboardText(void* in_userData, const char* in_text);
	const char* getClipboardText(void* in_userData);
	std::string m_clipboardText;

	// mouse cursors
	void loadMouseCursor(ImGuiMouseCursor in_imguiCursorType, sf::Cursor::Type in_sfmlCursorType);
	void updateMouseCursor(GameWindow& in_window);

	sf::Cursor* m_mouseCursors[ImGuiMouseCursor_COUNT];
	bool m_mouseCursorLoaded[ImGuiMouseCursor_COUNT];
};

