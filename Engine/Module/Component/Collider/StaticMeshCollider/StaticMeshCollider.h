#pragma once
#include "../ColliderBase.h"
#include "../../ComponentBase.h"
#include "../../../Physics/Layer/PhysicsLayer.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyID.h"

namespace NanamiEngine::Module::Component
{
    class StaticMeshCollider final : public ColliderBase,
                                     public LifeCycleCallback::IStartable
    {
    public:
        glm::vec3 offset_   = {0, 0, 0};
        glm::vec3 rotation_ = {0, 0, 0};
        glm::vec3 scale_    = {1.0f, 1.0f, 1.0f};
        
    private:
        void OnStart         () override;
        void OnDebugDraw     () const override;
        [[nodiscard]] const JPH::BodyID& BodyId() const override { return bodyId_; }
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;

        JPH::BodyID bodyId_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(layer_));
            archive(CEREAL_NVP(scale_));
            archive(CEREAL_NVP(rotation_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(offset_);
            if (version >= 0) archive(layer_);
            if (version >= 1) archive(scale_);
            if (version >= 2) archive(rotation_);
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::StaticMeshCollider, 2);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::StaticMeshCollider);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiEngine::Module::Component::StaticMeshCollider);
#pragma endregion