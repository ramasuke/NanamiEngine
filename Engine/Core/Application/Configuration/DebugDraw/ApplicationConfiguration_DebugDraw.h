#pragma once
#include <array>

#include "../../../../Module/Physics/Component/Collider/Engine_Physics_ColliderShapeKind.h"
#include "../../../../Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"

namespace NanamiEngine::Core::Application::Configuration
{
    /** @brief エディタでシーン全体のコライダーを描画するかの設定。StaticMesh 等は重いので種類・レイヤー・Trigger で絞り込める */
    class DebugDrawConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static bool ShouldDrawCollider(Module::Physics::ColliderShapeKind kind, Module::Physics::Layer layer, bool isSensor);

        static void DrawConfigGUI();

    private:
        using ColliderKindFlags  = std::array<bool, static_cast<size_t>(Module::Physics::ColliderShapeKind::Count)>;
        using ColliderLayerFlags = std::array<bool, static_cast<size_t>(Module::Physics::Layer::Count)>;

        static bool               showAllColliders_;
        static ColliderKindFlags  showColliderKinds_;
        static ColliderLayerFlags showColliderLayers_;
        static bool               showTriggerColliders_;
    };
}
