#pragma once
#include <cstdint>
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../Sound/UiSoundBank.h"
#include "Packages/UiFlow/UiFlow.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Ui
{
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
        void OnStart () override;
        void OnUpdate() override;

        void Open();
        void Close(bool withSound = true);
        void Select(int index);
        void Decide();
        void OpenSettings();

        [[nodiscard]] static bool CanOpen(const GameCore::IPlayerAvatar& avatar);
        /** @brief ホストで、ほかのプレイヤーが部屋にいる */
        [[nodiscard]] static bool IsHostLeavingOthers();

        [[serialize(0)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) settingsPrefab_;

        std::shared_ptr<UiFlow::UiScreen> screen_;
        std::shared_ptr<StageReturnNoticeUi> view_;
        // 閉じている間に開くキーを読む。開いている間の入力は screen_ から読む
        UiFlow::UiInputReader toggleInput_;
        int   selection_ = 0;
        // 帰ると決めた。シーンが切り替わるまで何も受け付けない
        bool  isLeaving_ = false;

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
