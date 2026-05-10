#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace NanamiEngine::Module::Component
{
    class ColliderBase;
}

namespace GamePlay::Prop
{
    class Canon final : public Component::ComponentBase
    {
    public:
        void Use() const;
        void Shoot() const;
        void RightRotate();
        void LeftRotate();

    private:
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) bulletPrefab_;
        [[serialize(0)]] float bulletForceSpeed_ = 10.0f;
        [[serialize(0)]] FIELD(Asset::SoundFile) shootSound_;
        [[serialize(1)]] float addRotateTorque_ = 3.0f;
        [[serialize(2)]] FIELD(CineMachine::CineMachineVirtualCamera) shootCamera_;
        [[serialize(2)]] FIELD(GameObject::IGameObject) shootBulletPos_;
        
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
            archive(CEREAL_NVP(shootCamera_));
            archive(CEREAL_NVP(shootBulletPos_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(bulletPrefab_));
            if (version >= 0) archive(CEREAL_NVP(bulletForceSpeed_));
            if (version >= 0) archive(CEREAL_NVP(shootSound_));
            if (version >= 1) archive(CEREAL_NVP(addRotateTorque_));
            if (version >= 2) archive(CEREAL_NVP(shootCamera_));
            if (version >= 2) archive(CEREAL_NVP(shootBulletPos_));
        }
#pragma endregion
    };

    
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::Canon, 2);
CEREAL_REGISTER_TYPE(GamePlay::Prop::Canon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Prop::Canon);
#pragma endregion
