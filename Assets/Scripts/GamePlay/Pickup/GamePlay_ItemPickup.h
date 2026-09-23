#pragma once
#include <memory>
#include <optional>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../Data/Item/Data_ItemData.h"
#include "GamePlay_PickupItemBase.h"

namespace GamePlay::Pickup
{
    /**
     * @brief 落ちているアイテム。プレイヤーの PickupArea に拾われると、拾った人のポーチへ入る。
     *        ポーチに入りきらない間は地面に残る
     */
    class ItemPickup final : public PickupItemBase
    {
    public:
        /** @brief 生成直後に呼ぶ。中身を決め、sideDirection 側へ跳ね上げる */
        void Drop(const std::shared_ptr<Asset::ItemData>& item, int count, const glm::vec3& sideDirection);

    private:
        void OnPickupUpdate(float elapsed_secs) override;
        [[nodiscard]] bool CanReceive(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const override;
        void Receive(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus) override;

        [[serialize(0)]] FIELD(Asset::ItemData) item_;
        [[serialize(0)]] int count_ = 1;
        /** 回して上下させる見た目の子。ルートは回転を止めてあるので、足元の光は水平のまま */
        [[serialize(0)]] FIELD(GameObject::IGameObject) model_;
        [[serialize(0)]] float spinSpeed_degPerSec_ = 45.0f;
        [[serialize(0)]] float bobHeight_           = 0.32f;
        [[serialize(0)]] float bobPeriod_secs_      = 2.0f;

        std::optional<glm::vec3> modelBasePos_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<PickupItemBase>(this));
            archive(CEREAL_NVP(item_));
            archive(CEREAL_NVP(count_));
            archive(CEREAL_NVP(model_));
            archive(CEREAL_NVP(spinSpeed_degPerSec_));
            archive(CEREAL_NVP(bobHeight_));
            archive(CEREAL_NVP(bobPeriod_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<PickupItemBase>(this));
            if (version >= 0) archive(CEREAL_NVP(item_));
            if (version >= 0) archive(CEREAL_NVP(count_));
            if (version >= 0) archive(CEREAL_NVP(model_));
            if (version >= 0) archive(CEREAL_NVP(spinSpeed_degPerSec_));
            if (version >= 0) archive(CEREAL_NVP(bobHeight_));
            if (version >= 0) archive(CEREAL_NVP(bobPeriod_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Pickup::ItemPickup, 0);
