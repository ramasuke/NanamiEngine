#include "Ui_LoadingHintCard.h"

#include <cstdlib>

#include "Engine/Core/Platform/Input/Input.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    /** @brief パッドに触られているか。ロード中は入力が無いので「出発」を押した瞬間の状態を拾う */
    bool LoadingHintIsGamepadActive()
    {
        const auto xInput = NanamiEngine::Platform::Input::Gamepad::Get();
        if (!xInput.connected)
            return false;

        for (const bool button : xInput.buttons)
        {
            if (button)
                return true;
        }

        constexpr int stickDeadZone = 8000;
        return std::abs(xInput.thumbLX) > stickDeadZone || std::abs(xInput.thumbLY) > stickDeadZone;
    }
}

namespace GamePlay::Ui
{
    void LoadingHintCard::Reset()
    {
        if (const auto text = text_.get())
            text->SetText(hintText_);

        ApplyGlyph(LoadingHintIsGamepadActive()
            ? GameCore::PlayerAvatar::PlayerAvatarInputDevice::Gamepad
            : GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse);
    }

    void LoadingHintCard::ApplyGlyph(const GameCore::PlayerAvatar::PlayerAvatarInputDevice device) const
    {
        const auto glyph = glyph_.get();
        if (!glyph)
            return;

        const bool isPad = device == GameCore::PlayerAvatar::PlayerAvatarInputDevice::Gamepad;
        const auto sprite = isPad ? padGlyphSprite_.get() : keyboardGlyphSprite_.get();
        if (sprite)
            glyph->SetSprite(sprite);
    }

    void LoadingHintCard::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("text_", text_);
        ImGuiHelper::OnDrawInputField("glyph_", glyph_);
        ImGuiHelper::OnDrawInputField("keyboardGlyphSprite_", keyboardGlyphSprite_);
        ImGuiHelper::OnDrawInputField("padGlyphSprite_", padGlyphSprite_);
        ImGuiHelper::OnDrawInputField("hintText_", hintText_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LoadingHintCard);
#pragma endregion
