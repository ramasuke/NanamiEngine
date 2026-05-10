#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/ICollisionEnterable/Engine_Physics_ICollisionEnterable.h"
#include "../../../Core/Game/DamageContext/Physics/PhysicsPower.h"

namespace GamePlay::PlayerAvatar::Bullet
{
    class CannonBullet final : public Component::ComponentBase,
                               public LifeCycleCallback::IStartable,
                               public Physics::Callback::ICollisionEnterable 
    {
    private:
        void OnStart() override;
        void OnCollisionEnter(
            const Physics::Manifold& maniFold,
            const std::shared_ptr<GameObject::IGameObject>& other) override;
        
        Coroutine::Task<void> ExplosionCountDownAsync();
        void Explosion();

        [[serialize(0)]] float explositionDuration_secs_ = 0.0f;
        [[serialize(0)]] GameCore::Damage::PhysicsPower physicsDamage_; 
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) explosionParticle_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(explositionDuration_secs_));
            archive(CEREAL_NVP(physicsDamage_));
            archive(CEREAL_NVP(explosionParticle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(explositionDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(physicsDamage_));
            if (version >= 0) archive(CEREAL_NVP(explosionParticle_));
        }
#pragma endregion
    };

    
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::Bullet::CannonBullet, 0);
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::Bullet::CannonBullet);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::PlayerAvatar::Bullet::CannonBullet);
#pragma endregion
