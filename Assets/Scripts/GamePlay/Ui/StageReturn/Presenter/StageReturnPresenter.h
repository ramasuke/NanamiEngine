#pragma once
#include <cstdint>
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../Sound/UiSoundBank.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Ui
{
    class SettingsScreenPresenter;
    class StageReturnNoticeUi;
}

namespace GamePlay::Ui
{
    /**
     * @brief ステージで ESC (パッドは Start) を押したら「帰 還」の貼り紙を出し、島へ帰るかを尋ねる。
     *
     * 帰るのは自分だけで、仲間へは何も送らない。ホストが帰ると部屋ごと閉じるので、そのときは断り書きを出す。
     * 開いている間もゲームは止めず、手元のアバターの State だけを止める。
     * 「設定」を選ぶと貼り紙を隠して設定画面を開き、閉じたら貼り紙へ戻る。
     */
    class StageReturnPresenter final : public Component::ComponentBase,
                                       public LifeCycleCallback::IStartable,
                                       public LifeCycleCallback::IUpdatable
    {
    private:
        enum class Phase : std::uint8_t
        {
            Closed,
            Opened,
            /** 設定画面を開いている。閉じたら Opened へ戻る */
            Settings,
            /** 帰ると決めた。シーンが切り替わるまで何も受け付けない */
            Leaving,
        };

        struct Keys
        {
            bool toggle  = false;
            bool prev    = false;
            bool next    = false;
            bool confirm = false;
            bool cancel  = false;
        };

        void OnStart () override;
        void OnUpdate() override;

        void UpdateOpened(const Keys& keys);
        void Open(GameCore::IPlayerAvatar& avatar);
        void Close(bool withSound = true);
        void Select(int index);
        void Decide();
        void OpenSettings();

        [[nodiscard]] static Keys ReadKeys();
        [[nodiscard]] static bool CanOpen(const GameCore::IPlayerAvatar& avatar);
        /** @brief ホストで、ほかのプレイヤーが部屋にいる */
        [[nodiscard]] static bool IsHostLeavingOthers();

        [[serialize(0)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) settingsPrefab_;

        std::shared_ptr<StageReturnNoticeUi> view_;
        std::weak_ptr<SettingsScreenPresenter> settings_;
        Phase phase_ = Phase::Closed;
        int   selection_ = 0;
        Keys  previousKeys_{};
        bool  isResumePending_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(settingsPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(uiSounds_));
            if (version >= 1) archive(CEREAL_NVP(settingsPrefab_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageReturnPresenter, 1);
