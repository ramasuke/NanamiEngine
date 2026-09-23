#include "GamePlay_MagicAim.h"

#include <cmath>

#include "geometric.hpp"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../Core/Game/Damage/Physics/Game_Damage_Physics.h"
#include "../../Core/Game/Magic/IMagicCaster.h"
#include "../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../Core/Game/Npc/Friendly/IFriendlyNpc.h"
#include "../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"
#include "../../Core/Game/PlayerAvatar/LockOnTarget/ILockOnTarget.h"
#include "../Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"

namespace GamePlay::Magic
{
    namespace
    {
        constexpr float MAGIC_AIM_GROUND_PROBE_UP   = 40.0f;
        constexpr float MAGIC_AIM_GROUND_PROBE_DOWN = 200.0f;

        /** @brief 撃ち手をこの画面が持っていて、hitObject の持ち主が攻撃を受ける相手(村人以外)なら、その持ち主を返す */
        std::shared_ptr<GameObject::IGameObject> FindHitEnemyForLocalCaster(const std::weak_ptr<GameObject::IGameObject>& caster,
                                                                            const std::shared_ptr<GameObject::IGameObject>& hitObject)
        {
            // 魔法は全員の画面で実行されるので、他人の魔法には反応しない
            const auto casterObject = caster.lock();
            if (!casterObject || !IsSpellApplicableTarget(*casterObject))
                return nullptr;

            const auto owner = Physics::FindBodyOwner(hitObject);
            if (!owner || owner->Components().Catch<GameCore::PlayerAvatar::ITakablePlayerAttack>().expired())
                return nullptr;

            // 村人は驚くだけでダメージは受けない
            if (!owner->Components().Catch<GameCore::Npc::IFriendlyNpc>().expired())
                return nullptr;

            return owner;
        }
    }

    glm::vec3 CasterForward(const GameCore::Magic::IMagicCaster& caster)
    {
        glm::vec3 forward = caster.CastRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
        forward.y = 0.0f;
        const float length = glm::length(forward);
        return length > 0.0001f ? forward / length : glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 AimPoint(const GameCore::Magic::IMagicCaster& caster, const float range)
    {
        if (const auto target = caster.AimTarget().lock())
        {
            // 部位グループはコライダーを持たず、Transform も本体の原点にある
            const auto collider = target->Components().Catch<Physics::ICollider>().lock();
            return collider
                ? collider->CenterOfMassPosition().value_or(target->Transform().GetWorldPos())
                : GameCore::PlayerAvatar::LockOnPositionOf(*target);
        }
        return caster.CastOrigin() + CasterForward(caster) * range;
    }

    glm::vec3 ProjectToGround(const glm::vec3& point)
    {
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const glm::vec3 start = point + glm::vec3(0.0f, MAGIC_AIM_GROUND_PROBE_UP, 0.0f);
        const auto hit = Physics::Raycast(start, glm::vec3(0.0f, -1.0f, 0.0f),
                                          MAGIC_AIM_GROUND_PROBE_UP + MAGIC_AIM_GROUND_PROBE_DOWN, mask);
        return hit.Hit() ? hit.Position() : point;
    }

    glm::quat LookRotation(const glm::vec3& from, const glm::vec3& to, const glm::quat& fallback)
    {
        const glm::vec3 diff = to - from;
        const float length = glm::length(diff);
        if (length < 0.0001f)
            return fallback;

        const glm::vec3 direction = diff / length;
        if (std::abs(direction.y) > 0.999f)
            return fallback;

        return glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    GameCore::Damage::PhysicsPower ScaledPower(const GameCore::Damage::PhysicsPower base, const float rate)
    {
        return GameCore::Damage::PhysicsPower(static_cast<int>(std::lround(static_cast<float>(base.Value()) * rate)));
    }

    bool IsSpellApplicableTarget(GameObject::IGameObject& targetObject)
    {
        return AttackArea<GameCore::PlayerAvatar::ITakablePlayerAttack>::IsDamageApplicableTarget(targetObject);
    }

    void ApplySpellDamage(GameObject::IGameObject& from,
                          const std::shared_ptr<GameObject::IGameObject>& hitObject,
                          const GameCore::Damage::PhysicsPower power)
    {
        // 手足のコライダーに当たっても本体にダメージが入るようにする
        const auto owner = Physics::FindBodyOwner(hitObject);
        if (!owner || !IsSpellApplicableTarget(*owner))
            return;

        for (const auto& weakTarget : owner->Components().Catches<GameCore::PlayerAvatar::ITakablePlayerAttack>())
        {
            if (const auto target = weakTarget.lock())
                target->OnTakeDamage(std::make_unique<GameCore::Damage::Physics>(from, *owner, power));
        }
    }

    void ShowSpellDamageText(const std::weak_ptr<GameObject::IGameObject>& caster,
                             const std::shared_ptr<GameObject::IGameObject>& hitObject,
                             const GameCore::Damage::PhysicsPower power,
                             const glm::vec3& position)
    {
        const auto owner = FindHitEnemyForLocalCaster(caster, hitObject);
        if (!owner)
            return;

        const auto magicCaster = caster.lock()->Components().Catch<GameCore::Magic::IMagicCaster>().lock();
        const auto prefab = magicCaster ? magicCaster->DealDamageTextPrefab() : nullptr;
        if (!prefab)
            return;

        Ui::SpawnDealDamageText(*prefab, position, power.Value());
    }

    bool ShakeOnSpellHit(const std::weak_ptr<GameObject::IGameObject>& caster,
                         const std::shared_ptr<GameObject::IGameObject>& hitObject,
                         const float intensity,
                         const float duration_secs)
    {
        if (intensity <= 0.0f || duration_secs <= 0.0f)
            return false;
        if (!FindHitEnemyForLocalCaster(caster, hitObject))
            return false;

        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(intensity, duration_secs);
        return true;
    }

    glm::vec3 HitPartPosition(GameObject::IGameObject& part)
    {
        const auto collider = part.Components().Catch<Physics::ICollider>().lock();
        return collider
            ? collider->CenterOfMassPosition().value_or(part.Transform().GetWorldPos())
            : part.Transform().GetWorldPos();
    }
}
