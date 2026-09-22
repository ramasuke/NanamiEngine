#pragma once
#include <optional>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../Core/Game/PlayerAvatar/Pickable/IPlayerPickable.h"
#include "../../Core/Game/StatusParameter/Money/Money.h"

namespace GamePlay::Pickup
{
    /**
     * @brief 落ちているお金。プレイヤーの PickupArea に拾われると、拾った人の財布へ入る
     */
    class MoneyPickup final : public Component::ComponentBase,
                              public LifeCycleCallback::IUpdatable,
                              public GameCore::PlayerAvatar::IPlayerPickable
    {
    public:
        /** @brief 生成直後に呼ぶ。額を決め、sideDirection 側へ跳ね上げる */
        void Drop(GameCore::StatusParameter::Money amount, const glm::vec3& sideDirection);

    private:
        void OnUpdate() override;
        [[nodiscard]] bool CanPickUp(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const override;
        void OnPickUp(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus) override;
        void Remove();

        [[serialize(0)]] GameCore::StatusParameter::Money amount_ = GameCore::StatusParameter::Money(1);
        [[serialize(0)]] float launchUpSpeed_      = 70.0f;
        [[serialize(0)]] float launchSideSpeedMin_ = 12.0f;
        [[serialize(0)]] float launchSideSpeedMax_ = 30.0f;
        [[serialize(0)]] float pickupDelay_secs_   = 0.6f;
        [[serialize(0)]] FIELD(Asset::SoundFile) pickupSound_;

        std::optional<float> originHeight_;
        float elapsed_secs_ = 0.0f;
        bool  isRemoved_    = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(amount_));
            archive(CEREAL_NVP(launchUpSpeed_));
            archive(CEREAL_NVP(launchSideSpeedMin_));
            archive(CEREAL_NVP(launchSideSpeedMax_));
            archive(CEREAL_NVP(pickupDelay_secs_));
            archive(CEREAL_NVP(pickupSound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(amount_));
            if (version >= 0) archive(CEREAL_NVP(launchUpSpeed_));
            if (version >= 0) archive(CEREAL_NVP(launchSideSpeedMin_));
            if (version >= 0) archive(CEREAL_NVP(launchSideSpeedMax_));
            if (version >= 0) archive(CEREAL_NVP(pickupDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(pickupSound_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Pickup::MoneyPickup, 0)
