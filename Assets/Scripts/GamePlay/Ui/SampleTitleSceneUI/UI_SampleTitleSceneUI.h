#pragma once
#include <memory>
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"

namespace GamePlay::Ui
{
    class AssetUpdatePresenter;

    class SampleTitleScene final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable
    {
    private:
        void OnStart() override;
        void OnGameStart();

        [[serialize(0)]] FIELD(NanamiUi::Button) gameStartButton_;
        [[serialize(0)]] FIELD(NanamiUi::Button) gameExitButton_;
        /** 配信アセットの更新「早馬の荷札」(AssetUpdatePresenter)。更新が済むまではゲームを始めさせない */
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) assetUpdatePrefab_;

        std::weak_ptr<AssetUpdatePresenter> assetUpdate_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(gameStartButton_));
            archive(CEREAL_NVP(gameExitButton_));
            archive(CEREAL_NVP(assetUpdatePrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(gameStartButton_));
            if (version >= 0) archive(CEREAL_NVP(gameExitButton_));
            if (version >= 1) archive(CEREAL_NVP(assetUpdatePrefab_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SampleTitleScene, 1)
