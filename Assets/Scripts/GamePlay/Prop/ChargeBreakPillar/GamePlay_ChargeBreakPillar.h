#pragma once
#include <memory>
#include <optional>

#include "gtc/quaternion.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace GamePlay::Prop
{
    /**
     * @brief 突進してきた敵の頭が刺さると折れて倒れる柱。ChargeStuckObstacle と同じ GameObject に付ける
     * @note 倒れた後は ChargeStuckObstacle を無効にし、standingCollider_ を破棄して通れるようにする
     */
    class ChargeBreakPillar final : public Component::ComponentBase,
                                    public LifeCycleCallback::IUpdatable
    {
    public:
        // NOTE: コライダーは子に付いているので、当たった GameObject から親をさかのぼって探す
        [[nodiscard]] static std::shared_ptr<ChargeBreakPillar> FindFrom(GameObject::IGameObject& hitObject);
        /** @brief position に一番近い柱。RPC の受信側が同じ柱を特定するのに使う */
        [[nodiscard]] static std::shared_ptr<ChargeBreakPillar> FindNear(const glm::vec3& position);

        /**
         * @brief fallDirection へ倒す。2回目以降は何もしない
         * @return 今回倒れたなら true
         */
        bool Collapse(const glm::vec3& fallDirection);
        [[nodiscard]] bool IsCollapsed() const { return isCollapsed_; }
        [[nodiscard]] int  CollapseDamage() const { return collapseDamage_; }

    private:
        void OnUpdate() override;
        [[nodiscard]] glm::vec3 DustPosition() const;

        /** 折れる位置が原点の上半分。倒れる向きに回す */
        [[serialize(0)]] FIELD(GameObject::IGameObject) top_;
        /** 立っている間だけの当たり。倒れたら破棄する */
        [[serialize(0)]] FIELD(GameObject::IGameObject) standingCollider_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) dustPoint_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) breakParticle_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) landParticle_;
        [[serialize(0)]] FIELD(Asset::SoundFile) breakSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) landSound_;
        /** 刺さった敵に入れるダメージ。0 なら入れない */
        [[serialize(0)]] int   collapseDamage_    = 30;
        [[serialize(0)]] float fallAngle_deg_     = 84.0f;
        [[serialize(0)]] float fallDuration_secs_ = 1.1f;

        std::optional<glm::quat> topStandingRot_;
        glm::vec3 fallAxis_          = glm::vec3(1.0f, 0.0f, 0.0f);
        float     fallElapsed_secs_  = 0.0f;
        bool      isCollapsed_       = false;
        bool      isLanded_          = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(top_));
            archive(CEREAL_NVP(standingCollider_));
            archive(CEREAL_NVP(dustPoint_));
            archive(CEREAL_NVP(breakParticle_));
            archive(CEREAL_NVP(landParticle_));
            archive(CEREAL_NVP(breakSound_));
            archive(CEREAL_NVP(landSound_));
            archive(CEREAL_NVP(collapseDamage_));
            archive(CEREAL_NVP(fallAngle_deg_));
            archive(CEREAL_NVP(fallDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(top_));
            if (version >= 0) archive(CEREAL_NVP(standingCollider_));
            if (version >= 0) archive(CEREAL_NVP(dustPoint_));
            if (version >= 0) archive(CEREAL_NVP(breakParticle_));
            if (version >= 0) archive(CEREAL_NVP(landParticle_));
            if (version >= 0) archive(CEREAL_NVP(breakSound_));
            if (version >= 0) archive(CEREAL_NVP(landSound_));
            if (version >= 0) archive(CEREAL_NVP(collapseDamage_));
            if (version >= 0) archive(CEREAL_NVP(fallAngle_deg_));
            if (version >= 0) archive(CEREAL_NVP(fallDuration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::ChargeBreakPillar, 0);
