#include "Data_UiSoundBankData.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    using GamePlay::Sound::UiSe;

    UiSoundBankData::UiSoundBankData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    const FIELD(SoundFile)* UiSoundBankData::Find(const UiSe se) const
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

    void UiSoundBankData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("cursor_", cursor_);
        LibCore::ImGuiHelper::OnDrawInputField("confirm_", confirm_);
        LibCore::ImGuiHelper::OnDrawInputField("cancel_", cancel_);
        LibCore::ImGuiHelper::OnDrawInputField("open_", open_);
        LibCore::ImGuiHelper::OnDrawInputField("close_", close_);
        LibCore::ImGuiHelper::OnDrawInputField("tab_", tab_);
        LibCore::ImGuiHelper::OnDrawInputField("stamp_", stamp_);
        LibCore::ImGuiHelper::OnDrawInputField("refuse_", refuse_);
        LibCore::ImGuiHelper::OnDrawInputField("digit_", digit_);
        LibCore::ImGuiHelper::OnDrawInputField("gameStart_", gameStart_);
        LibCore::ImGuiHelper::OnDrawInputField("stoneCursor_", stoneCursor_);
        LibCore::ImGuiHelper::OnDrawInputField("stoneConfirm_", stoneConfirm_);
        LibCore::ImGuiHelper::OnDrawInputField("hoofTick_", hoofTick_);
        LibCore::ImGuiHelper::OnDrawInputField("loadingDone_", loadingDone_);
        LibCore::ImGuiHelper::OnDrawInputField("hudSelect_", hudSelect_);
        LibCore::ImGuiHelper::OnDrawInputField("hudPaletteOpen_", hudPaletteOpen_);
        LibCore::ImGuiHelper::OnDrawInputField("hudPageShift_", hudPageShift_);
        LibCore::ImGuiHelper::OnDrawInputField("hudLockOn_", hudLockOn_);
        LibCore::ImGuiHelper::OnDrawInputField("hudLockOff_", hudLockOff_);
        LibCore::ImGuiHelper::OnDrawInputField("hudReady_", hudReady_);
        LibCore::ImGuiHelper::OnDrawInputField("hudNotice_", hudNotice_);
        LibCore::ImGuiHelper::OnDrawInputField("hudClear_", hudClear_);
        LibCore::ImGuiHelper::OnDrawInputField("hudBossAppear_", hudBossAppear_);
        LibCore::ImGuiHelper::OnDrawInputField("hudInteract_", hudInteract_);
        LibCore::ImGuiHelper::OnDrawInputField("chatOpen_", chatOpen_);
        LibCore::ImGuiHelper::OnDrawInputField("chatBlip_", chatBlip_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(UiSoundBankData, UI_SOUND_BANK_EXTENSION_LABEL, "Ui")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::UiSoundBankData, NanamiEngine::Module::ScriptableObject);
#pragma endregion
