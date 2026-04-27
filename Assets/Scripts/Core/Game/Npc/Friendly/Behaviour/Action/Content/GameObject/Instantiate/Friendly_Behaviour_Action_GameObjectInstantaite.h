#pragma once
#include "vec3.hpp"
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../../../../TickStatus/Friendly_Behaviour_TickStatus.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class GameObjectInstantiate final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] glm::vec3 offset_ = {};
        [[serialize(0)]] bool useAbsolutePosition_ = false;
        [[serialize(0)]] bool inheritRotation_ = true;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) prefab_;

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(useAbsolutePosition_));
            archive(CEREAL_NVP(inheritRotation_));
            archive(CEREAL_NVP(prefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(offset_));
            if (version >= 0) archive(CEREAL_NVP(useAbsolutePosition_));
            if (version >= 0) archive(CEREAL_NVP(inheritRotation_));
            if (version >= 0) archive(CEREAL_NVP(prefab_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(
        GameObjectInstantiate,
        "GameObject::Instantiate")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::GameObjectInstantiate, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::GameObjectInstantiate)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Friendly::Behaviour::ActionBase,
    GameCore::Npc::Friendly::Behaviour::Action::GameObjectInstantiate)