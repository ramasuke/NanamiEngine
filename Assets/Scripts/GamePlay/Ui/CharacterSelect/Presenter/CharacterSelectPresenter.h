#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../Model/CharacterSelectModel.h"
#include "../../../Sound/UiSoundBank.h"
#include "Packages/UiFlow/UiFlow.h"

namespace GamePlay::Ui
{
    class CharacterSelectUi;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    class CharacterPodium;
}

namespace GamePlay::Ui
{
    class CharacterSelectPresenter final : public Component::ComponentBase,
                                           public LifeCycleCallback::IStartable,
                                           public LifeCycleCallback::IUpdatable
    {
    public:
        void Bind(const std::weak_ptr<Prop::CharacterPodium>& podium);

    private:
        void OnStart  () override;
        void OnUpdate () override;

        void Open();
        void Discard();
        void Confirm();
        void Close(bool didSwitch);

        std::shared_ptr<UiFlow::UiScreen> screen_;
        std::shared_ptr<CharacterSelectUi> view_;
        std::unique_ptr<CharacterSelectModel> model_;
        std::weak_ptr<GameCore::IPlayerAvatar> suspendedAvatar_;
        std::weak_ptr<Prop::CharacterPodium> podium_;

        bool isClosed_ = false;
        bool hasStarted_ = false;

        [[serialize(1)]] FIELD(Asset::UiSoundBankData) uiSounds_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::CharacterSelectPresenter, 1);
