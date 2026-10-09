#pragma once
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "../Type/EnemyKind.h"

namespace GameCore::Npc::Enemy
{
    // NOTE: ステージの敵の湧き地点。この GameObject の位置と向きで kind_ の敵を湧かせる
    // NOTE: prefab_ を指定するとその prefab を湧かせる。ボス HP ゲージなどの後処理は kind_ で決まる
    class EnemySpawnPoint final : public Component::ComponentBase
    {
    public:
        [[nodiscard]] EnemyKind Kind() const { return kind_; }
        // NOTE: 指定がなければ nullptr (種類ごとの既定の prefab が使われる)
        [[nodiscard]] std::shared_ptr<Asset::PrefabGameObjectFile> Prefab() const { return prefab_.get(); }

    private:
        [[serialize(0)]] EnemyKind kind_ = EnemyKind::Hyena;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile) prefab_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(kind_));
            archive(CEREAL_NVP(prefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(kind_));
            // NOTE: 旧版のストーリーフラグ。今は使わないので読み捨てる
            if (version == 1)
            {
                int skipIfStoryFlag_ = -1;
                archive(CEREAL_NVP(skipIfStoryFlag_));
            }
            if (version >= 2) archive(CEREAL_NVP(prefab_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::EnemySpawnPoint, 2);
