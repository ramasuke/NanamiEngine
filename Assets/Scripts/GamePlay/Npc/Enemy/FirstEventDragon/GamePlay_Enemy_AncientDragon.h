#pragma once
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"

namespace GamePlay::Npc::Enemy
{
    // NOTE: 古竜。序章のドラゴンと同じ姿で、巣 (DragonNestScene) で戦う終章のボス
    class AncientDragon final : public GameCore::Npc::BossEnemyBase
    {
    private:
        void DoUpdate() override;
        [[nodiscard]] std::optional<GameCore::Npc::Enemy::EnemyKind> RecordKind() const override { return GameCore::Npc::Enemy::EnemyKind::AncientDragon; }

        // NOTE: 巣の外へ落ちたら戻す位置 (心臓の山の上空)
        [[serialize(0)]] glm::vec3 respawnPosition_ = glm::vec3(750.0f, 300.0f, 720.0f);
        // NOTE: これより下へ落ちたら戻す
        [[serialize(0)]] float fallLimitY_ = 0.0f;

#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<BossEnemyBase>(this));
            archive(CEREAL_NVP(respawnPosition_));
            archive(CEREAL_NVP(fallLimitY_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<BossEnemyBase>(this));
            if (version >= 0) archive(CEREAL_NVP(respawnPosition_));
            if (version >= 0) archive(CEREAL_NVP(fallLimitY_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::AncientDragon, 0);
#pragma endregion
