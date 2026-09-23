#include "UiSoundBank.h"

#include <algorithm>
#include "DxLib.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Sound
{
    UiSoundBank* UiSoundBank::instance_ = nullptr;

    UiSoundBank::~UiSoundBank()
    {
        // NOTE: RemoveComponent は OnDestroy を呼ばないので、デストラクタでも解除する
        if (instance_ == this)
            instance_ = nullptr;
    }

    void UiSoundBank::Play(const UiSe se, const int volume)
    {
        if (!instance_)
            return;

        if (const auto field = instance_->Find(se))
            Play(field->get(), volume);
    }

    void UiSoundBank::Play(const FIELD(Asset::SoundFile)& overrideSound, const UiSe fallback)
    {
        if (const auto sound = overrideSound.get())
        {
            Play(sound);
            return;
        }
        Play(fallback);
    }

    void UiSoundBank::Play(const std::shared_ptr<Asset::SoundFile>& sound, const int volume)
    {
        if (!sound)
            return;

        const int handle = sound->GetDxLibHandle();
        if (handle == -1)
            return;

        if (volume >= 0)
            ChangeNextPlayVolumeSoundMem(std::clamp(volume, 0, 255), handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK, TRUE);
    }

    void UiSoundBank::OnAwake()
    {
        instance_ = this;
    }

    void UiSoundBank::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;
    }

    const FIELD(Asset::SoundFile)* UiSoundBank::Find(const UiSe se) const
    {
        switch (se)
        {
        case UiSe::Cursor: return &cursor_;
        case UiSe::Confirm: return &confirm_;
        case UiSe::Cancel: return &cancel_;
        case UiSe::Open: return &open_;
        case UiSe::Close: return &close_;
        case UiSe::Tab: return &tab_;
        case UiSe::Stamp: return &stamp_;
        case UiSe::Refuse: return &refuse_;
        case UiSe::Digit: return &digit_;
        case UiSe::GameStart: return &gameStart_;
        case UiSe::StoneCursor: return &stoneCursor_;
        case UiSe::StoneConfirm: return &stoneConfirm_;
        case UiSe::HoofTick: return &hoofTick_;
        case UiSe::LoadingDone: return &loadingDone_;
        case UiSe::HudSelect: return &hudSelect_;
        case UiSe::HudPaletteOpen: return &hudPaletteOpen_;
        case UiSe::HudPageShift: return &hudPageShift_;
        case UiSe::HudLockOn: return &hudLockOn_;
        case UiSe::HudLockOff: return &hudLockOff_;
        case UiSe::HudReady: return &hudReady_;
        case UiSe::HudNotice: return &hudNotice_;
        case UiSe::HudClear: return &hudClear_;
        case UiSe::HudBossAppear: return &hudBossAppear_;
        case UiSe::HudInteract: return &hudInteract_;
        case UiSe::ChatOpen: return &chatOpen_;
        case UiSe::ChatBlip: return &chatBlip_;
        }
        return nullptr;
    }

    void UiSoundBank::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("cursor_", cursor_);
        ImGuiHelper::OnDrawInputField("confirm_", confirm_);
        ImGuiHelper::OnDrawInputField("cancel_", cancel_);
        ImGuiHelper::OnDrawInputField("open_", open_);
        ImGuiHelper::OnDrawInputField("close_", close_);
        ImGuiHelper::OnDrawInputField("tab_", tab_);
        ImGuiHelper::OnDrawInputField("stamp_", stamp_);
        ImGuiHelper::OnDrawInputField("refuse_", refuse_);
        ImGuiHelper::OnDrawInputField("digit_", digit_);
        ImGuiHelper::OnDrawInputField("gameStart_", gameStart_);
        ImGuiHelper::OnDrawInputField("stoneCursor_", stoneCursor_);
        ImGuiHelper::OnDrawInputField("stoneConfirm_", stoneConfirm_);
        ImGuiHelper::OnDrawInputField("hoofTick_", hoofTick_);
        ImGuiHelper::OnDrawInputField("loadingDone_", loadingDone_);
        ImGuiHelper::OnDrawInputField("hudSelect_", hudSelect_);
        ImGuiHelper::OnDrawInputField("hudPaletteOpen_", hudPaletteOpen_);
        ImGuiHelper::OnDrawInputField("hudPageShift_", hudPageShift_);
        ImGuiHelper::OnDrawInputField("hudLockOn_", hudLockOn_);
        ImGuiHelper::OnDrawInputField("hudLockOff_", hudLockOff_);
        ImGuiHelper::OnDrawInputField("hudReady_", hudReady_);
        ImGuiHelper::OnDrawInputField("hudNotice_", hudNotice_);
        ImGuiHelper::OnDrawInputField("hudClear_", hudClear_);
        ImGuiHelper::OnDrawInputField("hudBossAppear_", hudBossAppear_);
        ImGuiHelper::OnDrawInputField("hudInteract_", hudInteract_);
        ImGuiHelper::OnDrawInputField("chatOpen_", chatOpen_);
        ImGuiHelper::OnDrawInputField("chatBlip_", chatBlip_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Sound::UiSoundBank);
#pragma endregion
