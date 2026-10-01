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

namespace GamePlay::Ui
{
    class SettingsScreenUi;
}

namespace GamePlay::Ui
{
    class SettingsScreenPresenter final : public Component::ComponentBase,
                                          public LifeCycleCallback::IStartable,
                                          public LifeCycleCallback::IUpdatable
    {
    public:
        static std::weak_ptr<SettingsScreenPresenter> Open(Asset::PrefabGameObjectFile& prefab);
        [[nodiscard]] static bool IsOpen() { return isOpen_; }

    private:
        void OnStart () override;
        void OnUpdate() override;
        void OnDestroy() override;

        void MoveRow(int delta);
        void ChangeValue(int delta);
        void ChangeCategory(int delta);
        void Refresh();
        void Close();

        [[serialize(0)]] FIELD(Asset::UiSoundBankData) uiSounds_;

        static inline bool isOpen_ = false;

        std::shared_ptr<UiFlow::UiScreen> screen_;
        std::shared_ptr<SettingsScreenUi> view_;
        size_t category_     = 0;
        size_t selectedRow_  = 0;
        size_t firstVisible_ = 0;
        bool   isOwner_  = false;
        bool   isClosed_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::SettingsScreenPresenter, 0);
