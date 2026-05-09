#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::Module::Component
{
    class ColliderBase;
}

namespace GamePlay::Prop
{
    class Canon final : public Component::ComponentBase
    {
    public:
        void Shoot();
        void RightRotate();
        void LeftRotate();

    private:
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) bulletPrefab_;
        [[serialize(0)]] float bulletForceSpeed_ = 10.0f;
        [[serialize(0)]] FIELD(Asset::SoundFile) shootSound_;
        [[serialize(1)]] float addRotateTorque_ = 3.0f;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(bulletPrefab_));
            archive(CEREAL_NVP(bulletForceSpeed_));
            archive(CEREAL_NVP(shootSound_));
            archive(CEREAL_NVP(addRotateTorque_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bulletPrefab_));
            if (version >= 0) archive(CEREAL_NVP(bulletForceSpeed_));
            if (version >= 0) archive(CEREAL_NVP(shootSound_));
            if (version >= 1) archive(CEREAL_NVP(addRotateTorque_));
        }
#pragma endregion
    };

    
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::Canon, 1);
CEREAL_REGISTER_TYPE(GamePlay::Prop::Canon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Prop::Canon);
#pragma endregion
