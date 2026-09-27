#pragma once
#include <memory>
#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"

namespace GamePlay::Prop
{
    /**
     * @brief 骸竜が呼んだ砂嵐の間だけ叩ける光の心臓。requiredHits_ 回叩くと揺らぎ、骸竜が気絶して砂嵐が止む
     * @note 同じ GameObject に Sensor でない Collider と Static な RigidBody が要る。
     *       砂嵐の外では当たり判定ごと地面の下へ退ける (無効にしたコライダーも当たるため)
     */
    class StormHeart final : public Component::ComponentBase,
                             public LifeCycleCallback::IAwakable,
                             public LifeCycleCallback::IUpdatable,
                             public GameCore::PlayerAvatar::ITakablePlayerAttack
    {
    public:
        /** @brief 揺らいでいたら true を返し、揺らぎを消す。骸竜の BT (ホスト) が読む */
        [[nodiscard]] static bool ConsumeShaken();
        /** @brief 他のピアで揺らいだ心臓を、このピア (ホスト) でも揺らいだことにする */
        static void ShakeByRemote();
        /** @brief 揺らいで砂嵐が止んだときの演出。全ピアで流す */
        static void PlayShakenBurst();

    private:
        void OnAwake  () override;
        void OnUpdate () override;
        void OnDestroy() override;
        void OnTakeDamage(std::unique_ptr<GameCore::IDamage> damage) override;

        void Shake();
        void SetHitBoxActive(bool isActive);
        [[nodiscard]] glm::vec3 EffectPosition() const;

        static StormHeart* instance_;

        [[serialize(0)]] int   requiredHits_  = 3;
        // 1回の振りで何度も数えないための間隔
        [[serialize(0)]] float hitInterval_secs_ = 0.3f;
        [[serialize(0)]] glm::vec3 effectOffset_ = glm::vec3(0.0f, 9.0f, 0.0f);
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) hitParticle_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) shakenParticle_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hitSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) shakenSound_;

        glm::vec3 homeLocalPos_ = glm::vec3(0.0f);
        bool  isHitBoxActive_ = true;
        bool  isShaken_       = false;
        int   hitCount_       = 0;
        float hitCooldown_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(requiredHits_));
            archive(CEREAL_NVP(hitInterval_secs_));
            archive(CEREAL_NVP(effectOffset_));
            archive(CEREAL_NVP(hitParticle_));
            archive(CEREAL_NVP(shakenParticle_));
            archive(CEREAL_NVP(hitSound_));
            archive(CEREAL_NVP(shakenSound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(requiredHits_));
            if (version >= 0) archive(CEREAL_NVP(hitInterval_secs_));
            if (version >= 0) archive(CEREAL_NVP(effectOffset_));
            if (version >= 0) archive(CEREAL_NVP(hitParticle_));
            if (version >= 0) archive(CEREAL_NVP(shakenParticle_));
            if (version >= 0) archive(CEREAL_NVP(hitSound_));
            if (version >= 0) archive(CEREAL_NVP(shakenSound_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::StormHeart, 0);
