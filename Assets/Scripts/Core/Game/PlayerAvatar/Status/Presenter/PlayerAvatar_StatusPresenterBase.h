#pragma once
#include "../../../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Ui
{
    class PlayerStatus;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStatus;
}

namespace GamePlay::PlayerAvatar
{
    class StatusPresenterBase : public Component::ComponentBase
    {
    public:
        ~StatusPresenterBase() override = default;

    protected:
        void Initialize(
            const Ui::PlayerStatus& playerStatusView,
            const GameCore::PlayerAvatar::IPlayerAvatarStatus& playerStatusModel);

    private:
        void SubscribeModelEventToView(
            const Ui::PlayerStatus& view,
            const GameCore::PlayerAvatar::IPlayerAvatarStatus& model);
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::StatusPresenterBase, 0)