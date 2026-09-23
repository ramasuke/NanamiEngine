#pragma once
#include <memory>
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Awake/IAwakable.h"

namespace GamePlay::Sound
{
    /**
     * @brief UI の効果音。語彙は docs/UIDesign.md の2系統に合わせてある
     * NOTE: 音は tools/art/ui_sfx.py が作る (Assets/Audio/UI/Ui_* / Hud_* / Chat_*)
     */
    enum class UiSe
    {
        // 系統 A: 手で触れる物 (紙・木・鉄・石)
        Cursor,
        Confirm,
        Cancel,
        Open,
        Close,
        Tab,
        Stamp,
        Refuse,
        Digit,
        GameStart,
        StoneCursor,
        StoneConfirm,
        HoofTick,
        LoadingDone,
        // 系統 B: HUD (革袋・布・鉄の留め具・低い空気のうなり)
        HudSelect,
        HudPaletteOpen,
        HudPageShift,
        HudLockOn,
        HudLockOff,
        HudReady,
        HudNotice,
        HudClear,
        HudBossAppear,
        HudInteract,
        // 会話
        ChatOpen,
        ChatBlip,
    };

    /**
     * @brief UI の効果音をまとめて持つ。GameManage.scene に 1 つ置き、どのシーンからでも鳴らせるようにする
     * SoundPlayer と違い 3D 位置を使わず DxLib で直接 2D 再生するので、SoundPlayer の無いシーン
     * (GameOverScene / ChattingUiScene など) でも鳴る
     */
    class UiSoundBank final : public Component::ComponentBase,
                              public LifeCycleCallback::IAwakable
    {
    public:
        ~UiSoundBank() override;

        /**
         * @param volume 0〜255。負なら .meta の音量のまま
         */
        static void Play(UiSe se, int volume = -1);
        /**
         * @brief UI 個別に差し替えた音があればそれを、無ければ共通の音を鳴らす
         */
        static void Play(const FIELD(Asset::SoundFile)& overrideSound, UiSe fallback);
        static void Play(const std::shared_ptr<Asset::SoundFile>& sound, int volume = -1);

    private:
        void OnAwake() override;
        void OnDestroy() override;

        [[nodiscard]] const FIELD(Asset::SoundFile)* Find(UiSe se) const;

        static UiSoundBank* instance_;

        [[serialize(0)]] FIELD(Asset::SoundFile) cursor_;
        [[serialize(0)]] FIELD(Asset::SoundFile) confirm_;
        [[serialize(0)]] FIELD(Asset::SoundFile) cancel_;
        [[serialize(0)]] FIELD(Asset::SoundFile) open_;
        [[serialize(0)]] FIELD(Asset::SoundFile) close_;
        [[serialize(0)]] FIELD(Asset::SoundFile) tab_;
        [[serialize(0)]] FIELD(Asset::SoundFile) stamp_;
        [[serialize(0)]] FIELD(Asset::SoundFile) refuse_;
        [[serialize(0)]] FIELD(Asset::SoundFile) digit_;
        [[serialize(0)]] FIELD(Asset::SoundFile) gameStart_;
        [[serialize(0)]] FIELD(Asset::SoundFile) stoneCursor_;
        [[serialize(0)]] FIELD(Asset::SoundFile) stoneConfirm_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hoofTick_;
        [[serialize(0)]] FIELD(Asset::SoundFile) loadingDone_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudSelect_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudPaletteOpen_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudPageShift_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudLockOn_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudLockOff_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudReady_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudNotice_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudClear_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudBossAppear_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hudInteract_;
        [[serialize(0)]] FIELD(Asset::SoundFile) chatOpen_;
        [[serialize(0)]] FIELD(Asset::SoundFile) chatBlip_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
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
            archive(cereal::base_class<Component::ComponentBase>(this));
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
            instance_ = this;
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Sound::UiSoundBank, 0);
