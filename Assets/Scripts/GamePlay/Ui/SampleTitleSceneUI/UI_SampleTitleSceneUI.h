#pragma once
#include <memory>
#include <string>
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "../../../../../Packages/AssetUpdater/Task/AssetUpdateTask.h"

namespace GamePlay::Ui
{
    class SampleTitleScene final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable,
                                   public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart() override;
        void OnUpdate() override;
        void OnGameStart();

        // NOTE: 配信アセットの更新。実装を差し替えるときは CreateAssetUpdater だけを変える。
        //       ダイアログとウィンドウタイトルでの表示は仮
        [[nodiscard]] std::unique_ptr<AssetUpdater::IAssetUpdater> CreateAssetUpdater(const AssetUpdater::AssetUpdaterPaths& paths) const;
        void OnAssetUpdateStateChanged(AssetUpdater::AssetUpdateState state);
        void AskAssetUpdate();
        void AskAssetUpdateRetry();
        void ShowAssetUpdateProgress();
        void RestoreWindowTitle();
        void RestartAfterAssetUpdate();

        [[serialize(0)]] FIELD(NanamiUi::Button) gameStartButton_;
        [[serialize(0)]] FIELD(NanamiUi::Button) gameExitButton_;

        std::unique_ptr<AssetUpdater::AssetUpdateTask> assetUpdate_;
        AssetUpdater::AssetUpdateState                 shownAssetUpdateState_   = AssetUpdater::AssetUpdateState::Idle;
        int                                            shownAssetUpdatePercent_ = -1;
        std::wstring                                   windowTitleBeforeUpdate_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(gameStartButton_));
            archive(CEREAL_NVP(gameExitButton_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(gameStartButton_));
            if (version >= 0) archive(CEREAL_NVP(gameExitButton_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SampleTitleScene, 0)
