#pragma once
#include "../IDestructibleObject.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "../../../../Data/Drop/Data_DropTable.h"
#include "../../../Core/Game/StatusParameter/Health/Health.h"

namespace GamePlay::Prop
{
    class DestructibleObject final : public Component::ComponentBase,
                                     public IDestructibleObject
    {
    public:
        
        
    private:
        void OnTakeDamage(std::unique_ptr<GameCore::IDamage> context) override;
        void OnDestroy() override;
        [[nodiscard]] glm::vec3 BreakPosition();

        const static GameCore::StatusParameter::Health MIN_HEALTH;
        [[serialize(0)]] GameCore::StatusParameter::Health currentHealth_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) onDamageParticle_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) destroyParticle_;
        [[serialize(1)]] FIELD(GameObject::IGameObject) particlePos_;
        [[serialize(4)]] FIELD(Asset::DropTable) dropTable_;
        // 破棄はフレーム末なので、同じフレームの2発目で二重に壊れないようにする
        bool isBroken_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(currentHealth_));
            archive(CEREAL_NVP(onDamageParticle_));
            archive(CEREAL_NVP(destroyParticle_));
            archive(CEREAL_NVP(particlePos_));
            archive(CEREAL_NVP(dropTable_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(currentHealth_));
            if (version >= 0) archive(CEREAL_NVP(onDamageParticle_));
            if (version >= 0) archive(CEREAL_NVP(destroyParticle_));
            if (version >= 1) archive(CEREAL_NVP(particlePos_));
            if (version >= 4) archive(CEREAL_NVP(dropTable_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::DestructibleObject, 4)