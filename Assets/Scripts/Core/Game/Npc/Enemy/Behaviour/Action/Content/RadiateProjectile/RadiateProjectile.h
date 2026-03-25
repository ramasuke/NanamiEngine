#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class RadiateProjectile final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;
        Coroutine::Task<void> MoveProjectileAsync(TickContext context, const std::weak_ptr<GameObject::IGameObject>& projectileObject);


        [[serialize(0)]] glm::vec3 instantiateOffsetPos_ = {};
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) projectilePrefab_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(instantiateOffsetPos_));
            archive(CEREAL_NVP(projectilePrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(instantiateOffsetPos_));
            if (version >= 0) archive(CEREAL_NVP(projectilePrefab_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION(RadiateProjectile)
}
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile)