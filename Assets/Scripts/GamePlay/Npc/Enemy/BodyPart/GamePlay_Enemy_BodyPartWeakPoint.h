#pragma once
#include <memory>
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../Core/Game/PlayerAvatar/LockOnTarget/ILockOnTarget.h"

namespace GamePlay::Npc::Enemy
{
    /**
     * ボスの部位グループ。配下のコライダーが受けたダメージを溜め、閾値を超えたら一度だけ「破壊」する。
     * 部位コライダー自身ではなく、それらを束ねる親オブジェクトに付ける。
     */
    class BodyPartWeakPoint final : public Component::ComponentBase,
                                    public LifeCycleCallback::IUpdatable,
                                    public GameCore::PlayerAvatar::ILockOnPart
    {
    public:
        /** @brief 配下のコライダー中心の平均。グループ自体は動かず、肢のコライダーがアニメに追従する */
        [[nodiscard]] glm::vec3 LockOnPosition() override;
        /** @brief 破壊が成立した瞬間だけ true。破壊済みの部位は蓄積を続けるだけで二度と成立しない */
        bool AccumulateDamage(int damageValue);
        [[nodiscard]] bool IsBroken() const { return isBroken_; }
        /** @brief 大技の予備動作中など、狙わせたい間だけ弱点として露出させる */
        void OpenWeakWindow(float duration_secs);
        [[nodiscard]] bool IsWeakWindowOpen() const { return weakWindowRemaining_secs_ > 0.0f; }
        /** @brief 露出中の弱点に溜め攻撃が入ったか。スタンの判定とダメージ表記の赤文字で共有する */
        [[nodiscard]] bool IsChargeCounter(const bool isChargedAttack) const { return isChargedAttack && IsWeakWindowOpen(); }

        /** @brief 当たったコライダーから親を遡り、最初に見つかった部位グループを返す。stopAt(本体)まで見つからなければ nullptr */
        [[nodiscard]] static std::shared_ptr<BodyPartWeakPoint> FindFrom(
            const std::shared_ptr<GameObject::IGameObject>& hitPart,
            const GameObject::IGameObject&                  stopAt);

    private:
        void OnUpdate() override;
        void SetHintPlaying(bool isPlaying);

        [[serialize(0)]] int durability_ = 300;
        [[serialize(1)]] FIELD(Component::ParticleSystem) weakWindowHint_;
        // Manual 再生のパーティクルはループしないので、露出中はこの間隔で鳴らし直す
        [[serialize(0)]] float hintRetrigger_secs_ = 1.0f;

        int   accumulatedDamage_        = 0;
        bool  isBroken_                 = false;
        float weakWindowRemaining_secs_ = 0.0f;
        float hintRetriggerDuring_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(durability_));
            archive(CEREAL_NVP(weakWindowHint_));
            archive(CEREAL_NVP(hintRetrigger_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(durability_));
            if (version >= 1) archive(CEREAL_NVP(weakWindowHint_));
            if (version >= 0) archive(CEREAL_NVP(hintRetrigger_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Npc::Enemy::BodyPartWeakPoint, 1)
