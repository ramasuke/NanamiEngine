#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../Engine_Physics_Constraints.h"
#include "../../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Component
{
    class CapsuleCollider final : public ColliderBase
    {
    public:
        [[nodiscard]] const JPH::BodyID& BodyId() const override { return bodyId_; }

    private:
        void OnDrawGui  () override;
        void OnDebugDraw() const override;
        [[nodiscard]] const glm::vec3& CalcColliderWorldPos() const;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;

        float radius_ = 5.0f;
        float height_ = 10.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(height_));
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
            archive(CEREAL_NVP(height_));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(emotionType_));
            archive(CEREAL_NVP(layer_));
            if (version >= 2) archive(CEREAL_NVP(constraints_));
            if (version >= 2) archive(CEREAL_NVP(isSensor_));
        }
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::CapsuleCollider, 2)