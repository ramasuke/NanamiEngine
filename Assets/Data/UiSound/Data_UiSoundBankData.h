#pragma once
#include "../../Scripts/GamePlay/Sound/UiSe.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto UI_SOUND_BANK_EXTENSION_LABEL = ".uiSoundBank";

    class UiSoundBankData final : public ScriptableObject
    {
    public:
        explicit UiSoundBankData(const std::string& contentPath = "");

        [[nodiscard]] const FIELD(SoundFile)* Find(GamePlay::Sound::UiSe se) const;

    private:
        [[serialize(0)]] FIELD(SoundFile) cursor_;
        [[serialize(0)]] FIELD(SoundFile) confirm_;
        [[serialize(0)]] FIELD(SoundFile) cancel_;
        [[serialize(0)]] FIELD(SoundFile) open_;
        [[serialize(0)]] FIELD(SoundFile) close_;
        [[serialize(0)]] FIELD(SoundFile) tab_;
        [[serialize(0)]] FIELD(SoundFile) stamp_;
        [[serialize(0)]] FIELD(SoundFile) refuse_;
        [[serialize(0)]] FIELD(SoundFile) digit_;
        [[serialize(0)]] FIELD(SoundFile) gameStart_;
        [[serialize(0)]] FIELD(SoundFile) stoneCursor_;
        [[serialize(0)]] FIELD(SoundFile) stoneConfirm_;
        [[serialize(0)]] FIELD(SoundFile) hoofTick_;
        [[serialize(0)]] FIELD(SoundFile) loadingDone_;
        [[serialize(0)]] FIELD(SoundFile) hudSelect_;
        [[serialize(0)]] FIELD(SoundFile) hudPaletteOpen_;
        [[serialize(0)]] FIELD(SoundFile) hudPageShift_;
        [[serialize(0)]] FIELD(SoundFile) hudLockOn_;
        [[serialize(0)]] FIELD(SoundFile) hudLockOff_;
        [[serialize(0)]] FIELD(SoundFile) hudReady_;
        [[serialize(0)]] FIELD(SoundFile) hudNotice_;
        [[serialize(0)]] FIELD(SoundFile) hudClear_;
        [[serialize(0)]] FIELD(SoundFile) hudBossAppear_;
        [[serialize(0)]] FIELD(SoundFile) hudInteract_;
        [[serialize(0)]] FIELD(SoundFile) chatOpen_;
        [[serialize(0)]] FIELD(SoundFile) chatBlip_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(cursor_));
            archive(CEREAL_NVP(confirm_));
            archive(CEREAL_NVP(cancel_));
            archive(CEREAL_NVP(open_));
            archive(CEREAL_NVP(close_));
            archive(CEREAL_NVP(tab_));
            archive(CEREAL_NVP(stamp_));
            archive(CEREAL_NVP(refuse_));
            archive(CEREAL_NVP(digit_));
            archive(CEREAL_NVP(gameStart_));
            archive(CEREAL_NVP(stoneCursor_));
            archive(CEREAL_NVP(stoneConfirm_));
            archive(CEREAL_NVP(hoofTick_));
            archive(CEREAL_NVP(loadingDone_));
            archive(CEREAL_NVP(hudSelect_));
            archive(CEREAL_NVP(hudPaletteOpen_));
            archive(CEREAL_NVP(hudPageShift_));
            archive(CEREAL_NVP(hudLockOn_));
            archive(CEREAL_NVP(hudLockOff_));
            archive(CEREAL_NVP(hudReady_));
            archive(CEREAL_NVP(hudNotice_));
            archive(CEREAL_NVP(hudClear_));
            archive(CEREAL_NVP(hudBossAppear_));
            archive(CEREAL_NVP(hudInteract_));
            archive(CEREAL_NVP(chatOpen_));
            archive(CEREAL_NVP(chatBlip_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(cursor_));
            if (version >= 0) archive(CEREAL_NVP(confirm_));
            if (version >= 0) archive(CEREAL_NVP(cancel_));
            if (version >= 0) archive(CEREAL_NVP(open_));
            if (version >= 0) archive(CEREAL_NVP(close_));
            if (version >= 0) archive(CEREAL_NVP(tab_));
            if (version >= 0) archive(CEREAL_NVP(stamp_));
            if (version >= 0) archive(CEREAL_NVP(refuse_));
            if (version >= 0) archive(CEREAL_NVP(digit_));
            if (version >= 0) archive(CEREAL_NVP(gameStart_));
            if (version >= 0) archive(CEREAL_NVP(stoneCursor_));
            if (version >= 0) archive(CEREAL_NVP(stoneConfirm_));
            if (version >= 0) archive(CEREAL_NVP(hoofTick_));
            if (version >= 0) archive(CEREAL_NVP(loadingDone_));
            if (version >= 0) archive(CEREAL_NVP(hudSelect_));
            if (version >= 0) archive(CEREAL_NVP(hudPaletteOpen_));
            if (version >= 0) archive(CEREAL_NVP(hudPageShift_));
            if (version >= 0) archive(CEREAL_NVP(hudLockOn_));
            if (version >= 0) archive(CEREAL_NVP(hudLockOff_));
            if (version >= 0) archive(CEREAL_NVP(hudReady_));
            if (version >= 0) archive(CEREAL_NVP(hudNotice_));
            if (version >= 0) archive(CEREAL_NVP(hudClear_));
            if (version >= 0) archive(CEREAL_NVP(hudBossAppear_));
            if (version >= 0) archive(CEREAL_NVP(hudInteract_));
            if (version >= 0) archive(CEREAL_NVP(chatOpen_));
            if (version >= 0) archive(CEREAL_NVP(chatBlip_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::UiSoundBankData, 0);
#pragma endregion
