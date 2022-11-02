#include "ImguiIntegration.h"

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Touch.hpp>
#include <SFML/Window/Window.hpp>

#include <cassert>
#include <cstddef> // offsetof, NULL
#include <cstring> // memcpy

#include "Engine/GameWindow.h"
#include "Engine/Time.h"

#include <SFML/OpenGL.hpp>

// various helper functions
static ImColor toImColor(sf::Color in_c);
static ImVec2 getTopLeftAbsolute(const sf::FloatRect& in_rect);
static ImVec2 getDownRightAbsolute(const sf::FloatRect& in_rect);

static ImTextureID convertGLTextureHandleToImTextureID(GLuint in_glTextureHandle);
static GLuint convertImTextureIDToGLTextureHandle(ImTextureID in_textureID);

ImColor toImColor(sf::Color in_c) {
	return ImColor(static_cast<int>(in_c.r), static_cast<int>(in_c.g), static_cast<int>(in_c.b),
		static_cast<int>(in_c.a));
}
ImVec2 getTopLeftAbsolute(const sf::FloatRect& in_rect) {
	ImVec2 pos = ImGui::GetCursorScreenPos();
	return ImVec2(in_rect.left + pos.x, in_rect.top + pos.y);
}
ImVec2 getDownRightAbsolute(const sf::FloatRect& in_rect) {
	ImVec2 pos = ImGui::GetCursorScreenPos();
	return ImVec2(in_rect.left + in_rect.width + pos.x, in_rect.top + in_rect.height + pos.y);
}

ImTextureID convertGLTextureHandleToImTextureID(GLuint in_glTextureHandle) {
	ImTextureID textureID = (ImTextureID)NULL;
	std::memcpy(&textureID, &in_glTextureHandle, sizeof(GLuint));
	return textureID;
}
GLuint convertImTextureIDToGLTextureHandle(ImTextureID in_textureID) {
	GLuint glTextureHandle;
	std::memcpy(&glTextureHandle, &in_textureID, sizeof(GLuint));
	return glTextureHandle;
}

ImguiIntegration::ImguiIntegration(const GameWindow& in_window) 
{
	m_ctx = ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	// tell ImGui which features we support
	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendPlatformName = "imgui_impl_sfml";

	// init keyboard mapping
	io.KeyMap[ImGuiKey_Tab] = sf::Keyboard::Tab;
	io.KeyMap[ImGuiKey_LeftArrow] = sf::Keyboard::Left;
	io.KeyMap[ImGuiKey_RightArrow] = sf::Keyboard::Right;
	io.KeyMap[ImGuiKey_UpArrow] = sf::Keyboard::Up;
	io.KeyMap[ImGuiKey_DownArrow] = sf::Keyboard::Down;
	io.KeyMap[ImGuiKey_PageUp] = sf::Keyboard::PageUp;
	io.KeyMap[ImGuiKey_PageDown] = sf::Keyboard::PageDown;
	io.KeyMap[ImGuiKey_Home] = sf::Keyboard::Home;
	io.KeyMap[ImGuiKey_End] = sf::Keyboard::End;
	io.KeyMap[ImGuiKey_Insert] = sf::Keyboard::Insert;
	io.KeyMap[ImGuiKey_Delete] = sf::Keyboard::Delete;
	io.KeyMap[ImGuiKey_Backspace] = sf::Keyboard::BackSpace;
	io.KeyMap[ImGuiKey_Space] = sf::Keyboard::Space;
	io.KeyMap[ImGuiKey_Enter] = sf::Keyboard::Return;
	io.KeyMap[ImGuiKey_Escape] = sf::Keyboard::Escape;
	io.KeyMap[ImGuiKey_A] = sf::Keyboard::A;
	io.KeyMap[ImGuiKey_C] = sf::Keyboard::C;
	io.KeyMap[ImGuiKey_V] = sf::Keyboard::V;
	io.KeyMap[ImGuiKey_X] = sf::Keyboard::X;
	io.KeyMap[ImGuiKey_Y] = sf::Keyboard::Y;
	io.KeyMap[ImGuiKey_Z] = sf::Keyboard::Z;

	m_joystickId = getConnectedJoystickId();

	for (unsigned int i = 0; i < ImGuiNavInput_COUNT; i++) {
		m_joystickMapping[i] = NULL_JOYSTICK_BUTTON;
	}

	initDefaultJoystickMapping();

	// init rendering
	io.DisplaySize = ImVec2((float)in_window.GetWindowSize().x, (float)in_window.GetWindowSize().y);

	// clipboard
	// TODO
	//io.SetClipboardTextFn = setClipboardText;
	//io.GetClipboardTextFn = getClipboardText;

	// load mouse cursors
	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i) {
		m_mouseCursorLoaded[i] = false;
	}

	loadMouseCursor(ImGuiMouseCursor_Arrow, sf::Cursor::Arrow);
	loadMouseCursor(ImGuiMouseCursor_TextInput, sf::Cursor::Text);
	loadMouseCursor(ImGuiMouseCursor_ResizeAll, sf::Cursor::SizeAll);
	loadMouseCursor(ImGuiMouseCursor_ResizeNS, sf::Cursor::SizeVertical);
	loadMouseCursor(ImGuiMouseCursor_ResizeEW, sf::Cursor::SizeHorizontal);
	loadMouseCursor(ImGuiMouseCursor_ResizeNESW, sf::Cursor::SizeBottomLeftTopRight);
	loadMouseCursor(ImGuiMouseCursor_ResizeNWSE, sf::Cursor::SizeTopLeftBottomRight);
	loadMouseCursor(ImGuiMouseCursor_Hand, sf::Cursor::Hand);

	m_fontTexture = std::make_shared<sf::Texture>();
	UpdateFontTexture();

	m_windowHasFocus = in_window.GetSFMLWindow()->hasFocus();
}

ImguiIntegration::~ImguiIntegration() 
{
	ImGui::GetIO().Fonts->SetTexID(0);

	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i) {
		if (m_mouseCursorLoaded[i]) {
			delete m_mouseCursors[i];
			m_mouseCursors[i] = NULL;

			m_mouseCursorLoaded[i] = false;
		}
	}

	ImGui::DestroyContext(m_ctx);
}

void ImguiIntegration::HandleEvent(const sf::Event& in_event) {
	if (m_windowHasFocus) {
		ImGuiIO& io = ImGui::GetIO();

		switch (in_event.type) {
		case sf::Event::MouseMoved:
			m_mouseMoved = true;
			break;
		case sf::Event::MouseButtonPressed: // fall-through
		case sf::Event::MouseButtonReleased: {
			int button = in_event.mouseButton.button;
			if (in_event.type == sf::Event::MouseButtonPressed && button >= 0 && button < 3) {
				m_mousePressed[in_event.mouseButton.button] = true;
			}
		} break;
		case sf::Event::TouchBegan: // fall-through
		case sf::Event::TouchEnded: {
			m_mouseMoved = false;
			int button = in_event.touch.finger;
			if (in_event.type == sf::Event::TouchBegan && button >= 0 && button < 3) {
				m_touchDown[in_event.touch.finger] = true;
			}
		} break;
		case sf::Event::MouseWheelScrolled:
			if (in_event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel ||
				(in_event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel && io.KeyShift)) {
				io.MouseWheel += in_event.mouseWheelScroll.delta;
			}
			else if (in_event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) {
				io.MouseWheelH += in_event.mouseWheelScroll.delta;
			}
			break;
		case sf::Event::KeyPressed: // fall-through
		case sf::Event::KeyReleased: {
			int key = in_event.key.code;
			if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown)) {
				io.KeysDown[key] = (in_event.type == sf::Event::KeyPressed);
			}
			io.KeyCtrl = in_event.key.control;
			io.KeyAlt = in_event.key.alt;
			io.KeyShift = in_event.key.shift;
			io.KeySuper = in_event.key.system;
		} break;
		case sf::Event::TextEntered:
			// Don't handle the event for unprintable characters
			if (in_event.text.unicode < ' ' || in_event.text.unicode == 127) {
				break;
			}
			io.AddInputCharacter(in_event.text.unicode);
			break;
		case sf::Event::JoystickConnected:
			if (m_joystickId == NULL_JOYSTICK_ID) {
				m_joystickId = in_event.joystickConnect.joystickId;
			}
			break;
		case sf::Event::JoystickDisconnected:
			if (m_joystickId == in_event.joystickConnect.joystickId) { // used gamepad was
																	// disconnected
				m_joystickId = getConnectedJoystickId();
			}
			break;
		default:
			break;
		}
	}

	switch (in_event.type) {
	case sf::Event::LostFocus: {
		// reset all input - SFML doesn't send KeyReleased
		// event when window goes out of focus
		ImGuiIO& io = ImGui::GetIO();
		for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); ++i) {
			io.KeysDown[i] = false;
		}
		io.KeyCtrl = false;
		io.KeyAlt = false;
		io.KeyShift = false;
		io.KeySuper = false;

		m_windowHasFocus = false;
	} break;
	case sf::Event::GainedFocus:
		m_windowHasFocus = true;
		break;
	default:
		break;
	}
}

void ImguiIntegration::Update(GameWindow& in_window) {
	// Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
	updateMouseCursor(in_window);

	const sf::Vector2i mousePos = sf::Mouse::getPosition(*in_window.GetSFMLWindow());
	const Vec2f displaySize = in_window.GetWindowSize();

	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2(displaySize.x, displaySize.y);
	io.DeltaTime = Time::RealDT();

	if (m_windowHasFocus) {
		if (io.WantSetMousePos) {
			sf::Vector2i newMousePos(static_cast<int>(io.MousePos.x),
				static_cast<int>(io.MousePos.y));
			sf::Mouse::setPosition(newMousePos);
		}
		else {
			io.MousePos = ImVec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
		}
		for (unsigned int i = 0; i < 3; i++) {
			io.MouseDown[i] = m_touchDown[i] || sf::Touch::isDown(i) || m_mousePressed[i] ||
				sf::Mouse::isButtonPressed((sf::Mouse::Button)i);
			m_mousePressed[i] = false;
			m_touchDown[i] = false;
		}
	}

	assert(io.Fonts->Fonts.Size > 0); // You forgot to create and set up font
									  // atlas (see createFontTexture)

									  // gamepad navigation
	if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) && m_joystickId != NULL_JOYSTICK_ID) {
		updateJoystickActionState(io, ImGuiNavInput_Activate);
		updateJoystickActionState(io, ImGuiNavInput_Cancel);
		updateJoystickActionState(io, ImGuiNavInput_Input);
		updateJoystickActionState(io, ImGuiNavInput_Menu);

		updateJoystickActionState(io, ImGuiNavInput_FocusPrev);
		updateJoystickActionState(io, ImGuiNavInput_FocusNext);

		updateJoystickActionState(io, ImGuiNavInput_TweakSlow);
		updateJoystickActionState(io, ImGuiNavInput_TweakFast);

		updateJoystickDPadState(io);
		updateJoystickLStickState(io);
	}

	ImGui::NewFrame();

	if (ImGui::GetIO().MouseDrawCursor) {
		// Hide OS mouse cursor if imgui is drawing it
		in_window.GetSFMLWindow()->setMouseCursorVisible(false);
	}
}

void ImguiIntegration::Draw(GameWindow& in_window) {
	in_window.GetSFMLWindow()->resetGLStates();
	in_window.GetSFMLWindow()->pushGLStates();
	ImGui::Render();
	RenderDrawLists(ImGui::GetDrawData());
	in_window.GetSFMLWindow()->popGLStates();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ImguiIntegration::UpdateFontTexture() {
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;

	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	sf::Texture& texture = *m_fontTexture;
	texture.create(width, height);
	texture.update(pixels);

	ImTextureID texID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());
	io.Fonts->SetTexID(texID);
}

sf::Texture& ImguiIntegration::GetFontTexture() {
	return *m_fontTexture;
}

void ImguiIntegration::SetActiveJoystickId(unsigned int in_joystickId) {
	assert(in_joystickId < sf::Joystick::Count);
	m_joystickId = in_joystickId;
}

void ImguiIntegration::SetJoytickDPadThreshold(float in_threshold) {
	assert(in_threshold >= 0.f && in_threshold <= 100.f);
	m_dPadInfo.threshold = in_threshold;
}

void ImguiIntegration::SetJoytickLStickThreshold(float in_threshold) {
	assert(in_threshold >= 0.f && in_threshold <= 100.f);
	m_lStickInfo.threshold = in_threshold;
}

void ImguiIntegration::SetJoystickMapping(int action, unsigned int joystickButton) {
	assert(action < ImGuiNavInput_COUNT);
	assert(joystickButton < sf::Joystick::ButtonCount);
	m_joystickMapping[action] = joystickButton;
}

void ImguiIntegration::SetDPadXAxis(sf::Joystick::Axis in_dPadXAxis, bool in_inverted) {
	m_dPadInfo.xAxis = in_dPadXAxis;
	m_dPadInfo.xInverted = in_inverted;
}

void ImguiIntegration::SetDPadYAxis(sf::Joystick::Axis in_dPadYAxis, bool in_inverted) {
	m_dPadInfo.yAxis = in_dPadYAxis;
	m_dPadInfo.yInverted = in_inverted;
}

void ImguiIntegration::SetLStickXAxis(sf::Joystick::Axis in_lStickXAxis, bool in_inverted) {
	m_lStickInfo.xAxis = in_lStickXAxis;
	m_lStickInfo.xInverted = in_inverted;
}

void ImguiIntegration::SetLStickYAxis(sf::Joystick::Axis in_lStickYAxis, bool in_inverted) {
	m_lStickInfo.yAxis = in_lStickYAxis;
	m_lStickInfo.yInverted = in_inverted;
}

// copied from imgui/backends/imgui_impl_opengl2.cpp
void ImguiIntegration::SetupRenderState(ImDrawData* in_draw_data, int in_fb_width, int in_fb_height) {
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor
	// enabled, vertex/texcoord/color pointers, polygon fill.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); //
	// In order to composite our output buffer we need to preserve alpha
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_SMOOTH);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Setup viewport, orthographic projection matrix
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to
	// draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single
	// viewport apps.
	glViewport(0, 0, (GLsizei)in_fb_width, (GLsizei)in_fb_height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
#ifdef GL_VERSION_ES_CL_1_1
	glOrthof(in_draw_data->DisplayPos.x, in_draw_data->DisplayPos.x + in_draw_data->DisplaySize.x,
		in_draw_data->DisplayPos.y + in_draw_data->DisplaySize.y, in_draw_data->DisplayPos.y, -1.0f,
		+1.0f);
#else
	glOrtho(in_draw_data->DisplayPos.x, in_draw_data->DisplayPos.x + in_draw_data->DisplaySize.x,
		in_draw_data->DisplayPos.y + in_draw_data->DisplaySize.y, in_draw_data->DisplayPos.y, -1.0f,
		+1.0f);
#endif
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

// Rendering callback
void ImguiIntegration::RenderDrawLists(ImDrawData* in_draw_data) {
	ImGui::GetDrawData();
	if (in_draw_data->CmdListsCount == 0) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	assert(io.Fonts->TexID != (ImTextureID)NULL); // You forgot to create and set font texture

													// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
													// framebuffer coordinates)
	int fb_width = (int)(in_draw_data->DisplaySize.x * in_draw_data->FramebufferScale.x);
	int fb_height = (int)(in_draw_data->DisplaySize.y * in_draw_data->FramebufferScale.y);
	if (fb_width == 0 || fb_height == 0) return;
	in_draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// Backup GL state
	// Backup GL state
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_polygon_mode[2];
	glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4];
	glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4];
	glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	GLint last_shade_model;
	glGetIntegerv(GL_SHADE_MODEL, &last_shade_model);
	GLint last_tex_env_mode;
	glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &last_tex_env_mode);

#ifdef GL_VERSION_ES_CL_1_1
	GLint last_array_buffer;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
#else
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
#endif

	// Setup desired GL state
	SetupRenderState(in_draw_data, fb_width, fb_height);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = in_draw_data->DisplayPos; // (0,0) unless using multi-viewports
	ImVec2 clip_scale = in_draw_data->FramebufferScale; // (1,1) unless using retina display which are
														// often (2,2)

														// Render command lists
	for (int n = 0; n < in_draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = in_draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert),
			(const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert),
			(const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert),
			(const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback) {
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to
				// request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					SetupRenderState(in_draw_data, fb_width, fb_height);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clip_rect;
				clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
				clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
				clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
				clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

				if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f &&
					clip_rect.w >= 0.0f) {
					// Apply scissor/clipping rectangle
					glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w),
						(int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

					// Bind texture, Draw
					GLuint textureHandle = convertImTextureIDToGLTextureHandle(pcmd->TextureId);
					glBindTexture(GL_TEXTURE_2D, textureHandle);
					glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
						sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
						idx_buffer);
				}
			}
			idx_buffer += pcmd->ElemCount;
		}
	}

	// Restore modified GL state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]);
	glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2],
		(GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2],
		(GLsizei)last_scissor_box[3]);
	glShadeModel(last_shade_model);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, last_tex_env_mode);

#ifdef GL_VERSION_ES_CL_1_1
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glDisable(GL_SCISSOR_TEST);
#endif
}

unsigned int ImguiIntegration::getConnectedJoystickId() {
	for (unsigned int i = 0; i < (unsigned int)sf::Joystick::Count; ++i) {
		if (sf::Joystick::isConnected(i)) return i;
	}

	return NULL_JOYSTICK_ID;
}

void ImguiIntegration::initDefaultJoystickMapping() {
	SetJoystickMapping(ImGuiNavInput_Activate, 0);
	SetJoystickMapping(ImGuiNavInput_Cancel, 1);
	SetJoystickMapping(ImGuiNavInput_Input, 3);
	SetJoystickMapping(ImGuiNavInput_Menu, 2);
	SetJoystickMapping(ImGuiNavInput_FocusPrev, 4);
	SetJoystickMapping(ImGuiNavInput_FocusNext, 5);
	SetJoystickMapping(ImGuiNavInput_TweakSlow, 4);
	SetJoystickMapping(ImGuiNavInput_TweakFast, 5);

	SetDPadXAxis(sf::Joystick::PovX);
	// D-pad Y axis is inverted on Windows
#ifdef _WIN32
	SetDPadYAxis(sf::Joystick::PovY, true);
#else
	SetDPadYAxis(sf::Joystick::PovY);
#endif

	SetLStickXAxis(sf::Joystick::X);
	SetLStickYAxis(sf::Joystick::Y);

	SetJoytickDPadThreshold(5.f);
	SetJoytickLStickThreshold(5.f);
}

void ImguiIntegration::updateJoystickActionState(ImGuiIO& in_io, ImGuiNavInput_ in_action) {
	bool isPressed = sf::Joystick::isButtonPressed(m_joystickId, m_joystickMapping[in_action]);
	in_io.NavInputs[in_action] = isPressed ? 1.0f : 0.0f;
}

void ImguiIntegration::updateJoystickDPadState(ImGuiIO& in_io) {
	float dpadXPos = sf::Joystick::getAxisPosition(m_joystickId, m_dPadInfo.xAxis);
	if (m_dPadInfo.xInverted) dpadXPos = -dpadXPos;

	float dpadYPos = sf::Joystick::getAxisPosition(m_joystickId, m_dPadInfo.yAxis);
	if (m_dPadInfo.yInverted) dpadYPos = -dpadYPos;

	in_io.NavInputs[ImGuiNavInput_DpadLeft] = dpadXPos < -m_dPadInfo.threshold ? 1.0f : 0.0f;
	in_io.NavInputs[ImGuiNavInput_DpadRight] = dpadXPos > m_dPadInfo.threshold ? 1.0f : 0.0f;

	in_io.NavInputs[ImGuiNavInput_DpadUp] = dpadYPos < -m_dPadInfo.threshold ? 1.0f : 0.0f;
	in_io.NavInputs[ImGuiNavInput_DpadDown] = dpadYPos > m_dPadInfo.threshold ? 1.0f : 0.0f;
}

void ImguiIntegration::updateJoystickLStickState(ImGuiIO& in_io) {
	float lStickXPos = sf::Joystick::getAxisPosition(m_joystickId, m_lStickInfo.xAxis);
	if (m_lStickInfo.xInverted) lStickXPos = -lStickXPos;

	float lStickYPos = sf::Joystick::getAxisPosition(m_joystickId, m_lStickInfo.yAxis);
	if (m_lStickInfo.yInverted) lStickYPos = -lStickYPos;

	if (lStickXPos < -m_lStickInfo.threshold) {
		in_io.NavInputs[ImGuiNavInput_LStickLeft] = std::abs(lStickXPos / 100.f);
	}

	if (lStickXPos > m_lStickInfo.threshold) {
		in_io.NavInputs[ImGuiNavInput_LStickRight] = lStickXPos / 100.f;
	}

	if (lStickYPos < -m_lStickInfo.threshold) {
		in_io.NavInputs[ImGuiNavInput_LStickUp] = std::abs(lStickYPos / 100.f);
	}

	if (lStickYPos > m_lStickInfo.threshold) {
		in_io.NavInputs[ImGuiNavInput_LStickDown] = lStickYPos / 100.f;
	}
}

void ImguiIntegration::setClipboardText(void* /*userData*/, const char* in_text) {
	sf::Clipboard::setString(sf::String::fromUtf8(in_text, in_text + std::strlen(in_text)));
}

const char* ImguiIntegration::getClipboardText(void* /*userData*/) {
	std::basic_string<sf::Uint8> tmp = sf::Clipboard::getString().toUtf8();
	m_clipboardText = std::string(tmp.begin(), tmp.end());
	return m_clipboardText.c_str();
}

void ImguiIntegration::loadMouseCursor(ImGuiMouseCursor in_imguiCursorType, sf::Cursor::Type in_sfmlCursorType) {
	m_mouseCursors[in_imguiCursorType] = new sf::Cursor();
	m_mouseCursorLoaded[in_imguiCursorType] =
		m_mouseCursors[in_imguiCursorType]->loadFromSystem(in_sfmlCursorType);
}

void ImguiIntegration::updateMouseCursor(GameWindow& in_window) {
	ImGuiIO& io = ImGui::GetIO();
	if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0) {
		ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
		if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
			in_window.GetSFMLWindow()->setMouseCursorVisible(false);
		}
		else {
			in_window.GetSFMLWindow()->setMouseCursorVisible(true);

			sf::Cursor& c = m_mouseCursorLoaded[cursor] ? *m_mouseCursors[cursor] :
				*m_mouseCursors[ImGuiMouseCursor_Arrow];
			in_window.GetSFMLWindow()->setMouseCursor(c);
		}
	}
}

