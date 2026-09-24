#pragma once
#include <optional>
#include <string>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/Physics/ContactCallback/ICollisionEnterable/Engine_Physics_ICollisionEnterable.h"
#include "../../../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"

namespace NanamiEngine::Module::Component
{
    class Animator;
    class LookAtBone;
    class RigidBody;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::PlayerAvatar
{
    class PlayerHitShakeReceiver;
}

namespace GamePlay::Npc::Friendly
{
    class FriendlyNpc;

    /**
     * @brief FriendlyNpc が近くのプレイヤーを目で追い、攻撃されたりぶつかられたりしたら驚いて振り向く
     * @note  リアクション中は FriendlyNpc が BehaviourTree を止める
     */
    class FriendlyNpcReaction final : public Component::ComponentBase,
                                      public LifeCycleCallback::IUpdatable,
                                      public Physics::Callback::ICollisionEnterable,
                                      public GameCore::PlayerAvatar::ITakablePlayerAttack
    {
    public:
        [[nodiscard]] bool IsReacting() const { return reaction_.has_value(); }

    private:
        enum class ReactionKind
        {
            Hit,
            Bump,
        };

        struct Reaction
        {
            ReactionKind kind;
            float        elapsed_secs = 0.0f;
        };

        void OnUpdate() override;
        void OnCollisionEnter(const Physics::Manifold& maniFold, const std::shared_ptr<GameObject::IGameObject>& other) override;
        void OnTakeDamage(std::unique_ptr<GameCore::IDamage> damage) override;

        void StartReaction(ReactionKind kind, const glm::vec3& shakeDirection);
        void UpdateReaction(const GameCore::IPlayerAvatar* player);
        void EndReaction();
        void UpdateIdle(const GameCore::IPlayerAvatar* player);
        void TurnBodyTowards(const glm::vec3& targetPos, float turnSpeed_radPerSec) const;
        [[nodiscard]] bool IsMoving() const;
        [[nodiscard]] bool IsChatting() const;
        [[nodiscard]] glm::vec3 PlayerLookPosition(const GameCore::IPlayerAvatar& player);
        [[nodiscard]] std::shared_ptr<Component::Animator> ReadyAnimator() const;

        // 座っている NPC は体を回さず頭だけで追う
        [[serialize(0)]] bool  canTurnBody_               = true;
        [[serialize(0)]] float noticeRadius_              = 60.0f;
        [[serialize(0)]] float chatTurnSpeed_radPerSec_   = 3.0f;
        [[serialize(0)]] float reactTurnSpeed_radPerSec_  = 6.0f;
        // プレイヤーがこれ以上の速さでぶつかってきたら反応する(歩き 24 / 走り 70)
        [[serialize(0)]] float bumpSpeedThreshold_        = 45.0f;
        [[serialize(0)]] float reactionCooldown_secs_     = 0.6f;
        // Animator の State に入れる値。-1 ならアニメーションを変えない
        [[serialize(0)]] int   hitAnimatorState_          = 2;
        [[serialize(0)]] int   bumpAnimatorState_         = 3;
        [[serialize(0)]] std::string hitClipName_         = "Hit";
        [[serialize(0)]] std::string bumpClipName_        = "Bump";
        // クリップが見つからない・終わらない時の上限
        [[serialize(0)]] float hitMaxDuration_secs_       = 3.0f;
        [[serialize(0)]] float bumpMaxDuration_secs_      = 2.5f;
        [[serialize(0)]] float hitShakeAmplitude_         = 0.6f;
        [[serialize(0)]] float bumpShakeAmplitude_        = 0.3f;
        [[serialize(0)]] float shakeDuration_secs_        = 0.2f;
        // これ以上の水平速度で動いている間は歩いているとみなす
        [[serialize(1)]] float movingSpeedThreshold_      = 3.0f;
        // NOTE: クリップの終わり際で戻すと遷移のブレンドで最後まで見える
        [[serialize(1)]] float clipEndNormalizedTime_     = 0.9f;
        [[serialize(1)]] float minReaction_secs_          = 0.3f;

        std::optional<Reaction> reaction_;
        std::optional<int>      savedAnimatorState_;
        float                   cooldown_secs_ = 0.0f;
        int                     playerHeadBoneIndex_       = -1;
        int                     playerHeadBoneModelHandle_ = -1;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(canTurnBody_));
            archive(CEREAL_NVP(noticeRadius_));
            archive(CEREAL_NVP(chatTurnSpeed_radPerSec_));
            archive(CEREAL_NVP(reactTurnSpeed_radPerSec_));
            archive(CEREAL_NVP(bumpSpeedThreshold_));
            archive(CEREAL_NVP(reactionCooldown_secs_));
            archive(CEREAL_NVP(hitAnimatorState_));
            archive(CEREAL_NVP(bumpAnimatorState_));
            archive(CEREAL_NVP(hitClipName_));
            archive(CEREAL_NVP(bumpClipName_));
            archive(CEREAL_NVP(hitMaxDuration_secs_));
            archive(CEREAL_NVP(bumpMaxDuration_secs_));
            archive(CEREAL_NVP(hitShakeAmplitude_));
            archive(CEREAL_NVP(bumpShakeAmplitude_));
            archive(CEREAL_NVP(shakeDuration_secs_));
            archive(CEREAL_NVP(movingSpeedThreshold_));
            archive(CEREAL_NVP(clipEndNormalizedTime_));
            archive(CEREAL_NVP(minReaction_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(canTurnBody_));
            archive(CEREAL_NVP(noticeRadius_));
            archive(CEREAL_NVP(chatTurnSpeed_radPerSec_));
            archive(CEREAL_NVP(reactTurnSpeed_radPerSec_));
            archive(CEREAL_NVP(bumpSpeedThreshold_));
            archive(CEREAL_NVP(reactionCooldown_secs_));
            archive(CEREAL_NVP(hitAnimatorState_));
            archive(CEREAL_NVP(bumpAnimatorState_));
            archive(CEREAL_NVP(hitClipName_));
            archive(CEREAL_NVP(bumpClipName_));
            archive(CEREAL_NVP(hitMaxDuration_secs_));
            archive(CEREAL_NVP(bumpMaxDuration_secs_));
            archive(CEREAL_NVP(hitShakeAmplitude_));
            archive(CEREAL_NVP(bumpShakeAmplitude_));
            archive(CEREAL_NVP(shakeDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(movingSpeedThreshold_));
            if (version >= 1) archive(CEREAL_NVP(clipEndNormalizedTime_));
            if (version >= 1) archive(CEREAL_NVP(minReaction_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Npc::Friendly::FriendlyNpcReaction, 1);
