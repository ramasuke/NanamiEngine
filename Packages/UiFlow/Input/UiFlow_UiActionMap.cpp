#include "UiFlow_UiActionMap.h"

#include <algorithm>

namespace NanamiEngine::UiFlow
{
    namespace
    {
        using Platform::Input::GamepadButton;
        using Platform::Input::Key;

        std::size_t UiActionIndex(const UiAction action)
        {
            return static_cast<std::size_t>(action);
        }
    }

    UiActionMap UiActionMap::Default()
    {
        UiActionMap map;
        map.Set(UiAction::Up,      { { Key::Up,    Key::W }, { GamepadButton::DPadUp    }, StickDirection::Up    });
        map.Set(UiAction::Down,    { { Key::Down,  Key::S }, { GamepadButton::DPadDown  }, StickDirection::Down  });
        map.Set(UiAction::Left,    { { Key::Left,  Key::A }, { GamepadButton::DPadLeft  }, StickDirection::Left  });
        map.Set(UiAction::Right,   { { Key::Right, Key::D }, { GamepadButton::DPadRight }, StickDirection::Right });
        map.Set(UiAction::Submit,  { { Key::Return }, { GamepadButton::A             }, StickDirection::None });
        map.Set(UiAction::Cancel,  { { Key::Escape }, { GamepadButton::B             }, StickDirection::None });
        map.Set(UiAction::TabPrev, { { Key::Q      }, { GamepadButton::LeftShoulder  }, StickDirection::None });
        map.Set(UiAction::TabNext, { { Key::E      }, { GamepadButton::RightShoulder }, StickDirection::None });
        map.Set(UiAction::Menu,    { { Key::Escape }, { GamepadButton::Start         }, StickDirection::None });
        return map;
    }

    UiActionMap& UiActionMap::Set(const UiAction action, UiBinding binding)
    {
        bindings_[UiActionIndex(action)] = std::move(binding);
        return *this;
    }

    UiActionMap& UiActionMap::AddKey(const UiAction action, const Platform::Input::Key key)
    {
        bindings_[UiActionIndex(action)].keys.push_back(key);
        return *this;
    }

    UiActionMap& UiActionMap::AddButton(const UiAction action, const Platform::Input::GamepadButton button)
    {
        bindings_[UiActionIndex(action)].buttons.push_back(button);
        return *this;
    }

    UiActionMap& UiActionMap::Clear(const UiAction action)
    {
        bindings_[UiActionIndex(action)] = UiBinding{};
        return *this;
    }

    UiActionMap& UiActionMap::SetStickThreshold(const std::int16_t threshold)
    {
        stickThreshold_ = threshold;
        return *this;
    }

    bool UiActionMap::IsDown(const UiAction action, const Platform::Input::GamepadState& pad) const
    {
        const auto& binding = bindings_[UiActionIndex(action)];

        if (std::ranges::any_of(binding.keys, [](const Key key) { return Platform::Input::Keyboard::IsDown(key); }))
            return true;
        if (std::ranges::any_of(binding.buttons, [&pad](const GamepadButton button) { return pad.IsDown(button); }))
            return true;

        switch (binding.stick)
        {
        case StickDirection::Up:    return pad.thumbLY >  stickThreshold_;
        case StickDirection::Down:  return pad.thumbLY < -stickThreshold_;
        case StickDirection::Left:  return pad.thumbLX < -stickThreshold_;
        case StickDirection::Right: return pad.thumbLX >  stickThreshold_;
        case StickDirection::None:  return false;
        }
        return false;
    }
}
