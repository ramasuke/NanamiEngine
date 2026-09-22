#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "../Type/EnemyKind.h"

namespace GameCore::Npc::Enemy
{
    /** ステージの敵の湧き地点。この GameObject の位置と向きで kind_ の敵を湧かせる */
    class EnemySpawnPoint final : public Component::ComponentBase
    {
    public:
        [[nodiscard]] EnemyKind Kind() const { return kind_; }

    private:
        [[serialize(0)]] EnemyKind kind_ = EnemyKind::Hyena;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(kind_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(kind_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GameCore::Npc::Enemy::EnemySpawnPoint, 0)
