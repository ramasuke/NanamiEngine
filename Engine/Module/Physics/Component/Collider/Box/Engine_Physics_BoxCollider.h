#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../../../../Component/ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"

namespace NanamiEngine::Module::Component
{
    /// TODO: layer設定は未対応。後で追加予定。
    class BoxCollider final : public ColliderBase
    {
    private:
        void OnDrawGui  () override;
        void OnDebugDraw() const override;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;
        
        glm::vec3 size_   = glm::vec3(10, 10, 10);
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ColliderBase >(this));
            archive(cereal::base_class<IAwakable    >(this));
            archive(cereal::base_class<IBeginPhysics>(this));
            archive(cereal::base_class<IEndPhysics  >(this));
            archive(CEREAL_NVP(size_       ));
            archive(CEREAL_NVP(offset_     ));
            archive(CEREAL_NVP(emotionType_));
            archive(CEREAL_NVP(layer_      ));
            archive(CEREAL_NVP(constraints_));
            archive(CEREAL_NVP(isSensor_   ));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version >= 5) archive(cereal::base_class<ColliderBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IBeginPhysics>(this));
            archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
            if (version >= 0) archive(CEREAL_NVP(size_       ));
            if (version >= 0) archive(CEREAL_NVP(offset_     ));
            if (version >= 0) archive(CEREAL_NVP(emotionType_));
            if (version >= 2) archive(CEREAL_NVP(layer_      ));
            if (version >= 3) archive(CEREAL_NVP(constraints_));
            if (version >= 4) archive(CEREAL_NVP(isSensor_   ));
        }
#pragma endregion
    };
}
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::BoxCollider, 5)