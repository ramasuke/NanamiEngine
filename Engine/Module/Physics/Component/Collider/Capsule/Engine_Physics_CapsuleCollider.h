#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../Engine_Physics_Constraints.h"
#include "../../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Component
{
    class CapsuleCollider final : public ColliderBase
    {
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
            archive(cereal::base_class<ColliderBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(height_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            // v2 以前はベースクラスのフィールドをここで保存していたため移行
            if (version >= 3)
                archive(cereal::base_class<ColliderBase>(this));
            else
                archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(height_));
            if (version < 3) {
                archive(CEREAL_NVP(offset_));
                archive(CEREAL_NVP(emotionType_));
                archive(CEREAL_NVP(layer_));
                if (version >= 2) archive(CEREAL_NVP(constraints_));
                if (version >= 2) archive(CEREAL_NVP(isSensor_));
            }
        }
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::CapsuleCollider, 3)