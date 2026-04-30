#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class SampleFirstEventDragonChat final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void AppearFirstEventDragon(const TickContext& context);
        
        FIELD(Asset::PrefabGameObjectFile) firstEventDragonPrefab_;
        glm::vec3                          appearFirstEventDragonPosition_;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(firstEventDragonPrefab_));
            archive(CEREAL_NVP(appearFirstEventDragonPosition_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(firstEventDragonPrefab_));
            if (version >= 0) archive(CEREAL_NVP(appearFirstEventDragonPosition_));
        }
#pragma endregion
    };
    REGISTER_FRIENDLY_ACTION_WITH_NAME(SampleFirstEventDragonChat, "Other::SampleFirstEventDragonChat")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::SampleFirstEventDragonChat, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SampleFirstEventDragonChat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::SampleFirstEventDragonChat)