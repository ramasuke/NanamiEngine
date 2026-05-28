#pragma once
#include "../IDestructibleObject.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
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
        
        const static GameCore::StatusParameter::Health MIN_HEALTH;
        [[serialize(0)]] GameCore::StatusParameter::Health currentHealth_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) onDamageParticle_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) destroyParticle_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(currentHealth_));
            archive(CEREAL_NVP(onDamageParticle_));
            archive(CEREAL_NVP(destroyParticle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(currentHealth_));
            if (version >= 0) archive(CEREAL_NVP(onDamageParticle_));
            if (version >= 0) archive(CEREAL_NVP(destroyParticle_));
        }
#pragma endregion        
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::DestructibleObject, 0);
CEREAL_REGISTER_TYPE(GamePlay::Prop::DestructibleObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Prop::DestructibleObject);
#pragma endregion