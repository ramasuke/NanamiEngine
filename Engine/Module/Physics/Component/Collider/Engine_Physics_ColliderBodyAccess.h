#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine_Physics_ColliderBase.h"

namespace NanamiEngine::Module::Physics
{
    // BodyAssembler だけが Collider の非公開データを読むための Attorney
    class NANAMI_API ColliderBodyAccess final
    {
        friend class BodyAssembler;

        [[nodiscard]] static JPH::RefConst<JPH::Shape> CreateShape(const Component::ColliderBase& collider)
        {
            return collider.CreateColliderShape();
        }

        [[nodiscard]] static std::pair<JPH::Vec3, JPH::Quat> WorldTransform(const Component::ColliderBase& collider)
        {
            return collider.CalcWorldTransformInternal();
        }

        [[nodiscard]] static bool  IsSensor(const Component::ColliderBase& collider) { return collider.isSensor_; }
        [[nodiscard]] static Layer LayerOf (const Component::ColliderBase& collider) { return collider.layer_; }
        [[nodiscard]] static float Friction(const Component::ColliderBase& collider) { return collider.friction_; }
        [[nodiscard]] static MotionType LegacyMotionType(const Component::ColliderBase& collider) { return collider.legacyMotionType_; }
    };
}
