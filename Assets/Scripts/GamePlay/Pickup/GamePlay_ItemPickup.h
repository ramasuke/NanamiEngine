#pragma once
#include <memory>
#include <optional>

#include "vec3.hpp"
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../Data/Item/Data_ItemData.h"
#include "../../Core/Game/PlayerAvatar/Pickable/IPlayerPickable.h"

namespace GamePlay::Pickup
{
    /**
     * @brief 落ちているアイテム。プレイヤーの PickupArea に拾われると、拾った人のポーチへ入る。
     *        ポーチに入りきらない間は地面に残る
     */
    class ItemPickup final : public Component::ComponentBase,
                             public LifeCycleCallback::IUpdatable,
                             public GameCore::PlayerAvatar::IPlayerPickable
    {
    public:
        /** @brief 生成直後に呼ぶ。中身を決め、sideDirection 側へ跳ね上げる */
        void Drop(const std::shared_ptr<Asset::ItemData>& item, int count, const glm::vec3& sideDirection);

    private:
        void OnUpdate() override;
        [[nodiscard]] bool CanPickUp(const GameCore::PlayerAvatar::IPlayerAvatarStatus& picker) const override;
        void OnPickUp(GameCore::PlayerAvatar::IPlayerAvatarStatus& pickerStatus) override;
        [[nodiscard]] bool IsPickupDelayOver() const;
        void AnimateModel();
        void Remove();

        [[serialize(0)]] FIELD(Asset::ItemData) item_;
        [[serialize(0)]] int   count_              = 1;
        [[serialize(0)]] float launchUpSpeed_      = 70.0f;
        [[serialize(0)]] float launchSideSpeedMin_ = 8.0f;
        [[serialize(0)]] float launchSideSpeedMax_ = 20.0f;
        [[serialize(0)]] float pickupDelay_secs_   = 0.6f;
        [[serialize(0)]] FIELD(Asset::SoundFile) pickupSound_;
        /** 回して上下させる見た目の子。ルートは回転を止めてあるので、足元の光は水平のまま */
        [[serialize(0)]] FIELD(GameObject::IGameObject) model_;
        [[serialize(0)]] float spinSpeed_degPerSec_ = 45.0f;
        [[serialize(0)]] float bobHeight_           = 0.32f;
        [[serialize(0)]] float bobPeriod_secs_      = 2.0f;

        std::optional<float>     originHeight_;
        std::optional<glm::vec3> modelBasePos_;
        float elapsed_secs_ = 0.0f;
        bool  isRemoved_    = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(item_));
            archive(CEREAL_NVP(count_));
            archive(CEREAL_NVP(launchUpSpeed_));
            archive(CEREAL_NVP(launchSideSpeedMin_));
            archive(CEREAL_NVP(launchSideSpeedMax_));
            archive(CEREAL_NVP(pickupDelay_secs_));
            archive(CEREAL_NVP(pickupSound_));
            archive(CEREAL_NVP(model_));
            archive(CEREAL_NVP(spinSpeed_degPerSec_));
            archive(CEREAL_NVP(bobHeight_));
            archive(CEREAL_NVP(bobPeriod_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(item_));
            if (version >= 0) archive(CEREAL_NVP(count_));
            if (version >= 0) archive(CEREAL_NVP(launchUpSpeed_));
            if (version >= 0) archive(CEREAL_NVP(launchSideSpeedMin_));
            if (version >= 0) archive(CEREAL_NVP(launchSideSpeedMax_));
            if (version >= 0) archive(CEREAL_NVP(pickupDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(pickupSound_));
            if (version >= 0) archive(CEREAL_NVP(model_));
            if (version >= 0) archive(CEREAL_NVP(spinSpeed_degPerSec_));
            if (version >= 0) archive(CEREAL_NVP(bobHeight_));
            if (version >= 0) archive(CEREAL_NVP(bobPeriod_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Pickup::ItemPickup, 0)
