#pragma once
#include "ICollider.h"
#include "../ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Core/Reference.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Constraints.h"
#include "../../Physics/Layer/PhysicsLayer.h"

namespace JPH
{
    class Shape;
}

namespace NanamiEngine::Module::Component
{
    class ColliderBase : public ComponentBase,
                         public LifeCycleCallback::IAwakable,
                         public LifeCycleCallback::IBeginPhysics,
                         public LifeCycleCallback::IEndPhysics,
                         public Physics::ICollider
    {
    public:
        virtual ~ColliderBase() override;
        void ChangeEmotionType(const JPH::EMotionType& type) override;
        void ChangeIsGravity(bool isGravity);

        [[nodiscard]] virtual void OnDebugDraw() const = 0; 
        [[nodiscard]] bool IsGravity() const { return isGravity_; }
        [[nodiscard]] const JPH::BodyID& BodyId() const override { return bodyId_; }

    protected:
        [[nodiscard]] virtual JPH::RefConst<JPH::Shape> CreateColliderShape() const = 0;
        

        glm::vec3 offset_ = glm::vec3( 0,  0,  0);
        JPH::EMotionType emotionType_ = JPH::EMotionType::Static;
        bool isSensor_ = false;
        Physics::Constraints constraints_ = Physics::Constraints::None;
        Physics::Layer       layer_       = Physics::Layer::Default;
        JPH::BodyID bodyId_;
        
    private:
        void OnAwake         () override;
        void OnBeginPhysics  () override;
        void OnUpdatedPhysics() override;
        void BasedOnDrawgui  () override;
        void OnDestroy       () override;
        
        [[serialize(0)]] bool isGravity_ = true;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            if (version == 0) archive(CEREAL_NVP(isGravity_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version < 1) archive(CEREAL_NVP(isGravity_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ColliderBase, 1);
#pragma endregion