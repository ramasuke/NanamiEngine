#pragma once
#include "vec3.hpp"
#include "../../Core/Game/StatusParameter/Money/Money.h"
#include "GamePlay_PickupItemBase.h"

namespace GamePlay::Pickup
{
    /**
     * @brief 落ちているお金。プレイヤーの PickupArea に拾われると、拾った人の財布へ入る
     */
    class MoneyPickup final : public PickupItemBase
    {
    public:
        /** @brief 生成直後に呼ぶ。額を決め、sideDirection 側へ跳ね上げる */
        void Drop(GameCore::StatusParameter::Money amount, const glm::vec3& sideDirection);

    private:
        void Receive(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus) override;

        [[serialize(0)]] GameCore::StatusParameter::Money amount_ = GameCore::StatusParameter::Money(1);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<PickupItemBase>(this));
            archive(CEREAL_NVP(amount_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<PickupItemBase>(this));
            if (version >= 0) archive(CEREAL_NVP(amount_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Pickup::MoneyPickup, 0);
