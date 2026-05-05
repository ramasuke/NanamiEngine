#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../Position/Enemy_Behaviour_Action_Position.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class RadiateProjectile final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        Coroutine::Task<void> MoveProjectileAsync(
            TickContext context,
            const std::weak_ptr<GameObject::IGameObject>& projectileObject);

    private:
        [[serialize(0)]] float moveSpeed_ = 5.0f;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) projectilePrefab_;
        [[serialize(3)]] Position spawnPosition_;
        [[serialize(3)]] Position targetPosition_;
        

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(spawnPosition_));
            archive(CEREAL_NVP(targetPosition_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            //TODO: 後で消す
            if (version >= 3) archive(CEREAL_NVP(spawnPosition_));
            if (version >= 3) archive(CEREAL_NVP(targetPosition_)); 

            if (version >= 3)
                return;
            [[serialize(0)]] glm::vec3 instantiateOffsetPos_ = {};
            [[serialize(0)]] bool useAbsoluteSpawnPosition_ = false;
            [[serialize(0)]] float moveSpeed_ = 5.0f;
            [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) projectilePrefab_;
            [[serialize(1)]] glm::vec3 targetOffsetPos_ = {};
            [[serialize(2)]] Position::Mode targetMode_ = Position::Mode::EnemyOffset;
            [[serialize(2)]] FIELD(GameObject::IGameObject) targetObject_;
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(instantiateOffsetPos_));
            archive(CEREAL_NVP(useAbsoluteSpawnPosition_));
            bool useAbsoluteTargetPosition_;
            if (version <= 1) archive(CEREAL_NVP(useAbsoluteTargetPosition_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(projectilePrefab_));
            archive(CEREAL_NVP(targetOffsetPos_));
            if (version >= 2) archive(CEREAL_NVP(targetMode_));
            if (version >= 2) archive(CEREAL_NVP(targetObject_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(RadiateProjectile, "GameObject::RadiateProjectile")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile, 3)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::RadiateProjectile)