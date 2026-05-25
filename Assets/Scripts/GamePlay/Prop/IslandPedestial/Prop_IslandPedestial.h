#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Physics/Component/Listener/Collision/Engine_Physics_CollisionListener.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"

namespace GamePlay::Prop
{
    class IslandPedestial final : public Component::ComponentBase,
                                  public LifeCycleCallback::IAwakable,
                                  public LifeCycleCallback::IStartable
    {
    public:
        
        
    private:
        void OnAwake() override;
        void OnStart() override;
        
        
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) stageSelectUiPrefab_;
        [[serialize(1)]] FIELD(Component::CollisionListener) collisionListener_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(stageSelectUiPrefab_));
            archive(CEREAL_NVP(collisionListener_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stageSelectUiPrefab_));
            if (version >= 1) archive(CEREAL_NVP(collisionListener_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::IslandPedestial, 1);
CEREAL_REGISTER_TYPE(GamePlay::Prop::IslandPedestial);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Prop::IslandPedestial);
#pragma endregion