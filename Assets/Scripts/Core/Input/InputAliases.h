#pragma once
#include "Engine/Core/Platform/Input/Input.h"

// ゲームコードでの入力の短い書き方 (EngineNamespace.h と同じく using で見せる):
//   Keyboard::IsDown(Key::Return)   Gamepad::Get().IsDown(GamepadButton::A)   Mouse::IsDown(MouseButton::Left)
// DxLib の CheckHitKey / GetJoypadXInputState は使わない (docs/HotReload.md §2、tools/dxlib_guard)
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
