#include "InputSystem.h"

#include "Game.h"
#include "GameWindow.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

//--- InputComponent ---//
void InputComponent::Initialize()
{
	Object::Initialize();

	auto window = GameBase::Get()->GetWindow();
	auto inputSys = window->GetInputSystem();
	inputSys->AddInputComponent(AsShared<InputComponent>());
}
void InputComponent::Destroy()
{
	Object::Destroy();
}
void InputComponent::SetPriority(int32_t in_priority)
{
	m_priority = in_priority;
}
int32_t InputComponent::GetPriority() const
{
	return m_priority;
}
std::shared_ptr<TextState> InputComponent::Text(const std::string& in_name, const std::vector<eText>& in_inputs, bool in_consumeInput)
{
	auto state = std::make_shared<TextState>(in_name, in_inputs, in_consumeInput);
	m_textStates[in_name] = state;
	return state;
}
std::shared_ptr<CursorState> InputComponent::Cursor(const std::string& in_name, const std::vector<eCursor>& in_inputs, bool in_consumeInput)
{
	auto state = std::make_shared<CursorState>(in_name, in_inputs, in_consumeInput);
	m_cursorStates[in_name] = state;
	return state;
}
std::shared_ptr<AxisState> InputComponent::Axis(const std::string& in_name, const std::vector<eAxis>& in_inputs, bool in_consumeInput)
{
	auto state = std::make_shared<AxisState>(in_name, in_inputs, in_consumeInput);
	m_axisStates[in_name] = state;
	return state;
}

template <typename tStateMap>
static auto FindInputState(const tStateMap& in_stateMap, const std::string& in_name)
{
	auto found = in_stateMap.find(in_name);
	if(found != in_stateMap.end())
	{
		return found->second;
	}
	return decltype(found->second){};
}
std::shared_ptr<TextState> InputComponent::GetText(const std::string& in_name)
{
	return FindInputState(m_textStates, in_name);
}
std::shared_ptr<CursorState> InputComponent::GetCursor(const std::string& in_name)
{
	return FindInputState(m_cursorStates, in_name);
}
std::shared_ptr<AxisState> InputComponent::GetAxis(const std::string& in_name)
{
	return FindInputState(m_axisStates, in_name);
}
std::shared_ptr<ButtonStateBase> InputComponent::GetButton(const std::string& in_name)
{
	return FindInputState(m_buttonStates, in_name);
}
void InputComponent::Update(InputSystem* in_inputSys, std::set<int32_t>& in_consumedInputs)
{
	auto window = GameBase::Get()->GetWindow();
	auto sfmlWindow = window->GetSFMLWindow();
	auto winDims = sfmlWindow->getSize();

	int32_t inputRangeStart = 0;
	auto ShouldProcessInput = [&in_consumedInputs, &inputRangeStart](auto input, auto state)
	{
		bool consumeInput = state->m_consumeInput;
		int32_t inputIdx = (int32_t)input + inputRangeStart;
		if(in_consumedInputs.find(inputIdx) == in_consumedInputs.end())
		{
			if(consumeInput)
			{
				in_consumedInputs.insert(inputIdx);
			}
			return true;
		}
		return false;
	};

	auto IsXBox = [](int32_t i) {
		auto devId = sf::Joystick::getIdentification(i);
		bool isXbox = devId.productId == 654 && devId.vendorId == 1118;
		return isXbox;
	};
	auto IsDS4 = [](int32_t i) {
		auto devId = sf::Joystick::getIdentification(i);
		bool isDs4 = devId.productId == 1476 && devId.vendorId == 1356;
		return isDs4;
	};

	// Handle text mappings
	for(const auto& pair : m_textStates)
	{
		// Update state
		const auto& state = pair.second;
		state->lastText = state->text;
		state->text = {};

		// Process inputs
		for(auto input : state->m_inputs)
		{
			if(ShouldProcessInput(input, state))
			{
				switch(input)
				{
				case eText::Keyboard:
				{
					if(!sfmlWindow->hasFocus())
						break;
					auto& text = in_inputSys->m_textAccum;
					if(text.size())
					{
						state->text = text;
						if(state->m_consumeInput)
						{
							text.clear();
						}
					}
					break;
				}
				}
			}
		}
		state->Update();
	}
	inputRangeStart += (int32_t)eText::NUM_INPUTS;

	// Axis amount calculation
	auto CalculateAxisAmount = [sfmlWindow, in_inputSys, IsXBox, IsDS4](eAxis input, bool consumeInput) {
		Vec2f amount = Vec2f::Zero;
		switch(input)
		{
		case eAxis::LeftStick:
		{
			for(auto i = 0; i < sf::Joystick::Count; ++i)
			{
				if(!sf::Joystick::isConnected(i))
					continue;
				amount.x += sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::X) / 100.0f;
				amount.y += sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::Y) / 100.0f * -1.0f;
			}
			break;
		}
		case eAxis::RightStick:
		{
			for(auto i = 0; i < sf::Joystick::Count; ++i)
			{
				if(!sf::Joystick::isConnected(i))
					continue;
				bool isXbox = IsXBox(i);
				amount.x += sf::Joystick::getAxisPosition(i, isXbox ? sf::Joystick::Axis::U : sf::Joystick::Axis::Z) / 100.0f;
				amount.y += sf::Joystick::getAxisPosition(i, isXbox ? sf::Joystick::Axis::V : sf::Joystick::Axis::R) / 100.0f * -1.0f;
			}
			break;
		}
		case eAxis::DPAD:
		{
			for(auto i = 0; i < sf::Joystick::Count; ++i)
			{
				if(!sf::Joystick::isConnected(i))
					continue;
				amount.x += sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::PovX) / 100.0f;
				amount.y += sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::PovY) / 100.0f;
			}
			break;
		}
		case eAxis::WASD:
		{
			if(!sfmlWindow->hasFocus())
				break;
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			{
				amount.y += 1.0f;
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			{
				amount.x -= 1.0f;
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			{
				amount.y -= 1.0f;
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			{
				amount.x += 1.0f;
			}
			break;
		}
		case eAxis::ArrowKeys:
		{
			if(!sfmlWindow->hasFocus())
				break;
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			{
				amount.y += 1.0f;
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				amount.x -= 1.0f;
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				amount.y -= 1.0f;
			}
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				amount.x += 1.0f;
			}
			break;
		}

		case eAxis::RightTrigger:
		case eAxis::LeftTrigger:
		{
			for(auto i = 0; i < sf::Joystick::Count; ++i)
			{
				if(!sf::Joystick::isConnected(i) || IsXBox(i))
					continue;
				amount.x += sf::Joystick::getAxisPosition(i, input == eAxis::RightTrigger ? sf::Joystick::Axis::V : sf::Joystick::Axis::U) / 200.0f + 0.5f;
				amount.y += 0.0f;
			}
			break;
		}

		case eAxis::MouseWheel:
		{
			if(!sfmlWindow->hasFocus())
				break;
			auto& wheel = in_inputSys->m_wheelAccum;
			amount.x = wheel;
			amount.y = 0.0f;
			if(consumeInput)
			{
				wheel = 0.0f;
			}
			break;
		}
		}
		return amount;
	};

	// Update cursor mappings
	for(const auto& pair : m_cursorStates)
	{
		// Update state
		const auto& state = pair.second;
		state->lastPos = state->pos;
		// Cursor pos persists by default

		// Process inputs
		for(auto input : state->m_inputs)
		{
			if(ShouldProcessInput(input, state))
			{
				auto inputIdx = (int32_t)input;
				if(input == eCursor::RelativeMouse)
				{
					if(!sfmlWindow->hasFocus())
						break;
					auto mousePosSf = sf::Mouse::getPosition(*sfmlWindow);
					auto mousePos = Vec2f{ (float)mousePosSf.x, (float)mousePosSf.y };
					auto mouseDelta = mousePos - state->m_lastMousePos;
					state->m_lastMousePos = mousePos;
					state->pos += mouseDelta;
				}
				else if(input == eCursor::AbsoluteMouse)
				{
					auto mousePosSf = sf::Mouse::getPosition(*sfmlWindow);
					auto mousePos = Vec2f{ (float)mousePosSf.x, (float)mousePosSf.y };
					state->pos = mousePos;
				}
			}
		}

		// Clamp cursor pos to screen dimensions
		auto& x = state->pos.x;
		auto& y = state->pos.y;
		auto w = winDims.x;
		auto h = winDims.y;
		x = x < 0 ? 0 : x >= w ? w : x;
		y = y < 0 ? 0 : y >= h ? h : y;

		state->Update();
	}
	inputRangeStart += (int32_t)eCursor::NUM_INPUTS;

	// Update axis mappings
	for(const auto& pair : m_axisStates)
	{
		// Update state
		const auto& state = pair.second;
		Vec2f totalAmount = Vec2f::Zero;

		// Process inputs
		for(auto input : state->m_inputs)
		{
			if(ShouldProcessInput(input, state))
			{
				auto amount = CalculateAxisAmount(input, state->m_consumeInput);
				totalAmount += amount;
			}
		}

		state->SetAmount(totalAmount);
		state->Update();
	}
	inputRangeStart += (int32_t)eAxis::NUM_INPUTS;

	// Update button mappings
	for(const auto& pair : m_buttonStates)
	{
		// Update state
		const auto& state = pair.second;
		bool bPressed = false;

		// Process inputs
		for(auto input : state->m_inputs)
		{
			if(ShouldProcessInput(input, state))
			{
				int32_t inputIdx = (int32_t)input;

				// Keyboard button input
				static const auto keyStartIdx = (int32_t)eButton::Keyboard_Unknown + 1;
				if(inputIdx >= keyStartIdx && inputIdx < (int32_t)eButton::Keyboard_MAX)
				{
					if(!sfmlWindow->hasFocus())
						break;
					bPressed |= sf::Keyboard::isKeyPressed((sf::Keyboard::Key)(sf::Keyboard::A + inputIdx - keyStartIdx));
				}

				// Mouse button input
				static const auto mouseStartIdx = (int32_t)eButton::Mouse_Unknown + 1;
				if(inputIdx >= mouseStartIdx && inputIdx < (int32_t)eButton::Mouse_MAX)
				{
					if(!sfmlWindow->hasFocus())
						break;
					bPressed |= sf::Mouse::isButtonPressed((sf::Mouse::Button)(sf::Mouse::Left + inputIdx - mouseStartIdx));
				}

				// Joystick button input
				auto MapJoyInput = [IsDS4, IsXBox](int32_t i, int32_t inputIdx) {
					if(IsDS4(i))
					{
						int32_t ds4Mappings[] = {
							1, 2, 0, 3, 8, 9, 4, 5, 6, 7, 10, 11, -1, -1, -1, -1, 13, 12,
						};
						return ds4Mappings[inputIdx];
					}
					else if(IsXBox(i))
					{
						int32_t xboxMappings[] = {
							0, 1, 2, 3, 6, 7, 4, 5, -1, -1, 8, 9, -1, -1, -1, -1, -1, -1,
						};
						return xboxMappings[inputIdx];
					}
					return inputIdx;
				};
				static const auto joyStartIdx = (int32_t)eButton::Joy_Unknown + 1;
				if(inputIdx >= joyStartIdx && inputIdx < (int32_t)eButton::Joy_MAX)
				{
					for(auto i = 0; i < sf::Joystick::Count; ++i)
					{
						if(!sf::Joystick::isConnected(i))
							continue;
						auto povX = sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::PovX) / 100.0f;
						auto povY = sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::PovY) / 100.0f;
						if(input == eButton::Joy_Up)
						{
							bPressed |= povY > 0.1f;
						}
						else if(input == eButton::Joy_Down)
						{
							bPressed |= povY < -0.1f;
						}
						else if(input == eButton::Joy_Left)
						{
							bPressed |= povX < -0.1f;
						}
						else if(input == eButton::Joy_Right)
						{
							bPressed |= povX > 0.1f;
						}
						else
						{
							auto joyInputIdx = MapJoyInput(i, inputIdx - joyStartIdx);
							if(joyInputIdx >= 0)
							{
								bPressed |= sf::Joystick::isButtonPressed(i, joyInputIdx);
							}
						}
					}
				}
			}
		}

		state->SetPressed(bPressed);
		state->Update();
	}
	inputRangeStart += (int32_t)eButton::NUM_INPUTS;
}

//--- InputSystem ---//
void InputSystem::HandleEvent(sf::Event* in_event)
{
	switch(in_event->type)
	{
	case sf::Event::TextEntered:
	{
		m_textAccum += std::wstring(sf::String(in_event->text.unicode));
		break;
	}
	case sf::Event::MouseWheelScrolled:
	{
		m_wheelAccum += in_event->mouseWheelScroll.delta;
		break;
	}
	}
}
void InputSystem::Update()
{
	// Hijack or release mouse cursor
	auto window = GameBase::Get()->GetWindow();
	auto sfmlWindow = window->GetSFMLWindow();
	if(m_hijackCursor)
	{
		sfmlWindow->setMouseCursorGrabbed(true);
		sfmlWindow->setMouseCursorVisible(false);
	}
	else
	{
		sfmlWindow->setMouseCursorGrabbed(false);
		sfmlWindow->setMouseCursorVisible(true);
	}

	// Erase dead components
	m_comps.erase(std::remove_if(m_comps.begin(), m_comps.end(), [](auto in_mapping) {
		return !IsAlive(in_mapping);
	}), m_comps.end());

	// Update all mappings (in descending priority order)
	auto comps = m_comps;
	std::sort(comps.begin(), comps.end(), [](auto lhs, auto rhs) { return lhs->GetPriority() > rhs->GetPriority(); });
	std::set<int32_t> consumedInputs;
	for(auto comp : comps)
	{
		comp->Update(this, consumedInputs);
	}

	// Clear accumulators
	m_textAccum.clear();
	m_wheelAccum = 0.0f;
}
void InputSystem::AddInputComponent(std::shared_ptr<InputComponent> in_mapping)
{
	m_comps.push_back(in_mapping);
}
