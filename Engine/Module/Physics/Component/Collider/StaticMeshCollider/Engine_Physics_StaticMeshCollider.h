#pragma once
#include "../Engine_Physics_ColliderBase.h"
#include "../../../../Component/ComponentBase.h"
#include "../JoltPhysics/Jolt/Jolt.h"
#include "../JoltPhysics/Jolt/Physics/Body/BodyID.h"

namespace NanamiEngine::Module::Component
{
    class StaticMeshCollider final : public ColliderBase,
                                     public LifeCycleCallback::IStartable
    {
    public:
        // offset_ と offsetRotation_ は ColliderBase に定義済み
        glm::vec3 scale_ = {1.0f, 1.0f, 1.0f};

    private:
        void OnAwake    () override;
        void OnStart    () override;
        void OnDebugDraw() const override;
        [[nodiscard]] JPH::RefConst<JPH::Shape> CreateColliderShape() const override;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ColliderBase>(this));
            archive(CEREAL_NVP(scale_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version >= 3) {
                archive(cereal::base_class<ColliderBase>(this));
                archive(CEREAL_NVP(scale_));
            } else {
                // v2 以前はフィールドを直接保存していたため移行 (NVP なしの位置引数)
                archive(cereal::base_class<ComponentBase>(this));
                glm::vec3 tmpOffset;
                archive(tmpOffset);
                offset_ = tmpOffset;
                archive(layer_);
                if (version >= 1) archive(scale_);
                if (version >= 2) {
                    glm::vec3 tmpRotation;
                    archive(tmpRotation);
                    offsetRotation_ = tmpRotation;
                }
            }
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::StaticMeshCollider, 3)
