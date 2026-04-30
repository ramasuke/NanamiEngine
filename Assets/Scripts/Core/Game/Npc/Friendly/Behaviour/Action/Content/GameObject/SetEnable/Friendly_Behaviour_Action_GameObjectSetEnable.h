#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class GameObjectSetEnable final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] FIELD(GameObject::IGameObject) enableGameObject_;
        [[serialize(0)]] bool isEnable_ = false;

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(enableGameObject_));
            archive(CEREAL_NVP(isEnable_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(enableGameObject_));
            if (version >= 0) archive(CEREAL_NVP(isEnable_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(
        GameObjectSetEnable,
        "GameObject::SetEnable")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::GameObjectSetEnable, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::GameObjectSetEnable)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Friendly::Behaviour::ActionBase,
    GameCore::Npc::Friendly::Behaviour::Action::GameObjectSetEnable)