#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../Engine_Physics_Constraints.h"
#include "../../../../Component/ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"

namespace NanamiEngine::Module::Component
{
    class SphereCollider final : public ColliderBase
    {
    public:
        [[nodiscard]] const JPH::BodyID& BodyId() const override { return bodyId_; }

    private:
        void OnDrawGui       () override;
        void OnDebugDraw() const override;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;

        float radius_ = 5.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(emotionType_));
            archive(CEREAL_NVP(layer_));
            archive(CEREAL_NVP(constraints_));
            archive(CEREAL_NVP(isSensor_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(emotionType_));
            archive(CEREAL_NVP(layer_));
            archive(CEREAL_NVP(constraints_));
            archive(CEREAL_NVP(isSensor_));
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::SphereCollider, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::SphereCollider);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::SphereCollider);
#pragma endregion