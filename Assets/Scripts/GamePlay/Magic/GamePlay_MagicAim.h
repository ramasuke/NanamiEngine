#pragma once
#include <memory>

#include "vec3.hpp"
#include "gtc/quaternion.hpp"
#include "../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Magic
{
    class IMagicCaster;
}

namespace GamePlay::Magic
{
    /** @brief 撃ち手の水平な正面 */
    [[nodiscard]] glm::vec3 CasterForward(const GameCore::Magic::IMagicCaster& caster);
    /** @brief ロックオン中は対象の重心、していなければ撃ち手の正面 range 先 */
    [[nodiscard]] glm::vec3 AimPoint(const GameCore::Magic::IMagicCaster& caster, float range);
    /** @brief point の少し上から真下へ地面を探す。見つからなければ point のまま */
    [[nodiscard]] glm::vec3 ProjectToGround(const glm::vec3& point);
    /** @brief -Z を from から to へ向ける回転。向きが決まらなければ fallback */
    [[nodiscard]] glm::quat LookRotation(const glm::vec3& from, const glm::vec3& to, const glm::quat& fallback);
    [[nodiscard]] GameCore::Damage::PhysicsPower ScaledPower(GameCore::Damage::PhysicsPower base, float rate);
    /** @brief 対象をこの画面が持っているか。持っていない画面はダメージや回復を入れない */
    [[nodiscard]] bool IsSpellApplicableTarget(GameObject::IGameObject& targetObject);
    /** @brief hitObject の持ち主に魔法のダメージを入れる。持ち主をこの画面が持っていなければ何もしない */
    void ApplySpellDamage(GameObject::IGameObject& from,
                          const std::shared_ptr<GameObject::IGameObject>& hitObject,
                          GameCore::Damage::PhysicsPower power);
    /** @brief 撃ち手の画面でだけ、hitObject の持ち主にダメージ表記を出す。ダメージを入れる画面とは限らない */
    void ShowSpellDamageText(const std::weak_ptr<GameObject::IGameObject>& caster,
                             const std::shared_ptr<GameObject::IGameObject>& hitObject,
                             GameCore::Damage::PhysicsPower power,
                             const glm::vec3& position);
    /** @brief 部位のコライダーの重心。コライダーがなければ Transform の位置 */
    [[nodiscard]] glm::vec3 HitPartPosition(GameObject::IGameObject& part);
}
