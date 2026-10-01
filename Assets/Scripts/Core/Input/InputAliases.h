#pragma once
#include "Engine/Core/Platform/Input/Input.h"

// ゲームコードでの入力の短い書き方 (Keyboard::IsDown(Key::Return) など)
namespace GameCore::InputAliases
{
    using NanamiEngine::Platform::Input::Key;
    using NanamiEngine::Platform::Input::MouseButton;
    using NanamiEngine::Platform::Input::GamepadButton;
    using NanamiEngine::Platform::Input::GamepadState;
    namespace Keyboard = NanamiEngine::Platform::Input::Keyboard;
    namespace Mouse    = NanamiEngine::Platform::Input::Mouse;
    namespace Gamepad  = NanamiEngine::Platform::Input::Gamepad;
}
using namespace GameCore::InputAliases;
