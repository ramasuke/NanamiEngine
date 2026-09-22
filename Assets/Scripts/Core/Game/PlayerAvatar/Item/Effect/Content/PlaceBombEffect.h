#pragma once
#include "../IItemEffect.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Damage/Physics/Game_Damage_PhysicsPower.h"

namespace GameCore::PlayerAvatar::Item
{
    // 使い手の正面の地面に爆弾を置く。起爆と範囲ダメージは置いたプレハブの MagicBlast が受け持つ
    class PlaceBombEffect final : public IItemEffect
    {
    public:
        void Apply(IItemEffectTarget& target, const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& user) const override;

    private:
        /** @brief センサーの SphereCollider と MagicBlast を持つプレハブ */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) bombPrefab_;
        [[serialize(0)]] Damage::PhysicsPower power_ = Damage::PhysicsPower(80);
        /** @brief 置いてから起爆するまで */
        [[serialize(0)]] float fuse_secs_ = 3.0f;
        /** @brief 使い手からどれだけ前に置くか */
        [[serialize(0)]] float distance_ = 8.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IItemEffect>(this));
            archive(CEREAL_NVP(bombPrefab_));
            archive(CEREAL_NVP(power_));
            archive(CEREAL_NVP(fuse_secs_));
            archive(CEREAL_NVP(distance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IItemEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(bombPrefab_));
            if (version >= 0) archive(CEREAL_NVP(power_));
            if (version >= 0) archive(CEREAL_NVP(fuse_secs_));
            if (version >= 0) archive(CEREAL_NVP(distance_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Item::PlaceBombEffect, 0);
