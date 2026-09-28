#pragma once
#include <cstdint>
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    class AssetUpdatePresenter;
    class SettingsScreenPresenter;
    class TitleScreenUi;

    class TitleScreenPresenter final : public Component::ComponentBase,
                                       public LifeCycleCallback::IStartable,
                                       public LifeCycleCallback::IUpdatable
    {
    private:
        enum class Phase : std::uint8_t
        {
            Press,
            Menu,
            /** 設定画面を重ねている。閉じたら Menu へ戻る */
            Settings,
            Leaving,
        };

        struct Keys
        {
            bool any     = false;
            bool up      = false;
            bool down    = false;
            bool confirm = false;
            bool cancel  = false;
        };

        void OnStart () override;
        void OnUpdate() override;

        [[nodiscard]] static Keys ReadKeys();
        [[nodiscard]] bool IsAssetUpdatePrompting() const;
        void Select(int index);
        void Decide(int index);
        void StartGame();
        void OpenSettings();
        void ExitGame();

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) assetUpdatePrefab_;
        [[serialize(0)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) settingsPrefab_;

        std::shared_ptr<TitleScreenUi> view_;
        std::weak_ptr<AssetUpdatePresenter> assetUpdate_;
        std::weak_ptr<SettingsScreenPresenter> settings_;
        Phase phase_ = Phase::Press;
        int selection_ = 0;
        Keys previousKeys_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(assetUpdatePrefab_));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(settingsPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(assetUpdatePrefab_));
            if (version >= 0) archive(CEREAL_NVP(uiSounds_));
            if (version >= 1) archive(CEREAL_NVP(settingsPrefab_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::TitleScreenPresenter, 1);
