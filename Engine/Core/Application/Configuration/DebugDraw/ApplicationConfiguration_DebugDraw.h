#pragma once
#include <array>

#include "../../../../Module/Physics/Component/Collider/Engine_Physics_ColliderShapeKind.h"
#include "../../../../Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Core::Application::Configuration
{
    /** @brief エディタでシーン全体のコライダーを描画するかの設定。StaticMesh 等は重いので種類・レイヤー・Trigger で絞り込める */
    class DebugDrawConfiguration final
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        static void Load();
        static void Save();

        [[nodiscard]] static bool ShouldDrawCollider(Module::Physics::ColliderShapeKind kind, Module::Physics::Layer layer, bool isSensor);
        [[nodiscard]] static bool ShouldDrawMainCameraFrustum();
        [[nodiscard]] static bool ShouldDrawVirtualCameraFrustum();

        static void DrawConfigGUI();

    private:
        using ColliderKindFlags  = std::array<bool, static_cast<size_t>(Module::Physics::ColliderShapeKind::Count)>;
        using ColliderLayerFlags = std::array<bool, Module::Physics::MAX_LAYER_COUNT>;

        static bool               showAllColliders_;
        static ColliderKindFlags  showColliderKinds_;
        static ColliderLayerFlags showColliderLayers_;
        static bool               showTriggerColliders_;
        static bool               showMainCameraFrustum_;
        static bool               showVirtualCameraFrustums_;
    };
}
