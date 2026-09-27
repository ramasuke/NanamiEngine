#pragma once
#include <optional>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../Core/Game/PlayerAvatar/Pickable/IPlayerPickable.h"

namespace GamePlay::Pickup
{
    /**
     * @brief 地面に落ちている拾い物の共通部分。跳ね上げて出し、少し待ってから拾えるようにし、
     *        拾えるようになったらこの PC のプレイヤーへ飛んでいって渡し、SE とパーティクルを出して消える。
     *        持ち物に入りきらない間は地面に残り、島の外へ落ちたら消える
     */
    class PickupItemBase : public Component::ComponentBase,
                           public LifeCycleCallback::IUpdatable,
                           public GameCore::PlayerAvatar::IPlayerPickable
    {
    protected:
        /** @brief 生成直後に呼ぶ。sideDirection 側へ、横速度をランダムにして跳ね上げる */
        void Launch(const glm::vec3& sideDirection);
        [[nodiscard]] bool IsPickable() const;

        virtual void OnPickupUpdate(float elapsed_secs) {}
        /** @brief picker の持ち物に入りきらない間は false。その間は地面に残る */
        [[nodiscard]] virtual bool CanReceive(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const { return true; }
        /** @brief 中身を拾った人へ渡す */
        virtual void Receive(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus) = 0;

    private:
        void OnUpdate() final;
        [[nodiscard]] bool CanPickUp(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const final;
        void OnPickUp(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus) final;
        void UpdateHoming();
        void StartHoming();
        void StopHoming();
        void PlayPickupFeedback();
        void Remove();

        [[serialize(0)]] float launchUpSpeed_      = 70.0f;
        [[serialize(0)]] float launchSideSpeedMin_ = 8.0f;
        [[serialize(0)]] float launchSideSpeedMax_ = 20.0f;
        [[serialize(0)]] float pickupDelay_secs_   = 0.6f;
        [[serialize(0)]] FIELD(Asset::SoundFile) pickupSound_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) pickupParticle_;
        // 島の外へ落ちた拾い物を落とし続けないよう、出た高さからこれだけ落ちたら消す
        [[serialize(1)]] float fallOutDepth_ = 500.0f;
        // 隙間や崖下へ落ちても取りに行かずに済むよう、地形をすり抜けてプレイヤーへ飛ばす
        [[serialize(2)]] float homingSpeed_          = 80.0f;
        [[serialize(2)]] float homingAcceleration_   = 600.0f;
        [[serialize(2)]] float homingMaxSpeed_       = 700.0f;
        // NOTE: プレイヤーのカプセルに押し当てないよう、触れる前に渡す
        [[serialize(2)]] float homingArriveDistance_ = 25.0f;
        [[serialize(2)]] float homingTargetHeight_   = 20.0f;

        std::optional<float> originHeight_;
        float elapsed_secs_   = 0.0f;
        float homingSpeedNow_ = 0.0f;
        bool  isHoming_       = false;
        bool  isRemoved_      = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(launchUpSpeed_));
            archive(CEREAL_NVP(launchSideSpeedMin_));
            archive(CEREAL_NVP(launchSideSpeedMax_));
            archive(CEREAL_NVP(pickupDelay_secs_));
            archive(CEREAL_NVP(pickupSound_));
            archive(CEREAL_NVP(pickupParticle_));
            archive(CEREAL_NVP(fallOutDepth_));
            archive(CEREAL_NVP(homingSpeed_));
            archive(CEREAL_NVP(homingAcceleration_));
            archive(CEREAL_NVP(homingMaxSpeed_));
            archive(CEREAL_NVP(homingArriveDistance_));
            archive(CEREAL_NVP(homingTargetHeight_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(launchUpSpeed_));
            if (version >= 0) archive(CEREAL_NVP(launchSideSpeedMin_));
            if (version >= 0) archive(CEREAL_NVP(launchSideSpeedMax_));
            if (version >= 0) archive(CEREAL_NVP(pickupDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(pickupSound_));
            if (version >= 0) archive(CEREAL_NVP(pickupParticle_));
            if (version >= 1) archive(CEREAL_NVP(fallOutDepth_));
            if (version >= 2) archive(CEREAL_NVP(homingSpeed_));
            if (version >= 2) archive(CEREAL_NVP(homingAcceleration_));
            if (version >= 2) archive(CEREAL_NVP(homingMaxSpeed_));
            if (version >= 2) archive(CEREAL_NVP(homingArriveDistance_));
            if (version >= 2) archive(CEREAL_NVP(homingTargetHeight_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Pickup::PickupItemBase, 2);
