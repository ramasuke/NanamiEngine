#include "ApplicationConfiguration_DebugDraw.h"

#include <string>

#include "../ApplicationConfiguration.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto DEFAULT_SHOW_ALL_COLLIDERS     = false;
    constexpr auto DEFAULT_SHOW_COLLIDER_LAYER    = true;
    constexpr auto DEFAULT_SHOW_TRIGGER_COLLIDERS = true;
    constexpr auto DEFAULT_SHOW_MAIN_CAMERA_FRUSTUM     = true;
    constexpr auto DEFAULT_SHOW_VIRTUAL_CAMERA_FRUSTUMS = true;
    // Box, Sphere, Capsule, Cylinder, StaticMesh の順。StaticMesh は描画が重いのでデフォルト OFF
    constexpr std::array<bool, static_cast<size_t>(Module::Physics::ColliderShapeKind::Count)> DEFAULT_SHOW_COLLIDER_KINDS = { true, true, true, true, false };

    bool DebugDrawConfiguration::showAllColliders_     = DEFAULT_SHOW_ALL_COLLIDERS;
    bool DebugDrawConfiguration::showTriggerColliders_ = DEFAULT_SHOW_TRIGGER_COLLIDERS;
    bool DebugDrawConfiguration::showMainCameraFrustum_     = DEFAULT_SHOW_MAIN_CAMERA_FRUSTUM;
    bool DebugDrawConfiguration::showVirtualCameraFrustums_ = DEFAULT_SHOW_VIRTUAL_CAMERA_FRUSTUMS;
    DebugDrawConfiguration::ColliderKindFlags  DebugDrawConfiguration::showColliderKinds_  = DEFAULT_SHOW_COLLIDER_KINDS;
    DebugDrawConfiguration::ColliderLayerFlags DebugDrawConfiguration::showColliderLayers_ = []
    {
        std::array<bool, Module::Physics::MAX_LAYER_COUNT> flags;
        flags.fill(DEFAULT_SHOW_COLLIDER_LAYER);
        return flags;
    }();

    constexpr auto DEBUG_DRAW_CONFIG_PATH                = "DebugDraw/";
    constexpr auto DEBUG_DRAW_SHOW_ALL_COLLIDERS_KEY     = "ShowAllColliders";
    constexpr auto DEBUG_DRAW_SHOW_COLLIDER_KEY_PREFIX   = "ShowCollider_";
    constexpr auto DEBUG_DRAW_SHOW_LAYER_KEY_PREFIX      = "ShowColliderLayer_";
    constexpr auto DEBUG_DRAW_SHOW_TRIGGER_COLLIDERS_KEY = "ShowTriggerColliders";
    constexpr auto DEBUG_DRAW_SHOW_MAIN_CAMERA_FRUSTUM_KEY     = "ShowMainCameraFrustum";
    constexpr auto DEBUG_DRAW_SHOW_VIRTUAL_CAMERA_FRUSTUMS_KEY = "ShowVirtualCameraFrustums";

    void DebugDrawConfiguration::Load()
    {
        showAllColliders_ = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_ALL_COLLIDERS_KEY, DEFAULT_SHOW_ALL_COLLIDERS);

        for (size_t i = 0; i < showColliderKinds_.size(); ++i)
        {
            const std::string key = std::string(DEBUG_DRAW_SHOW_COLLIDER_KEY_PREFIX) + Module::Physics::COLLIDER_SHAPE_KIND_NAMES[i];
            showColliderKinds_[i] = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, key, DEFAULT_SHOW_COLLIDER_KINDS[i]);
        }

        for (size_t i = 0; i < static_cast<size_t>(Module::Physics::LayerCount()); ++i)
        {
            const std::string key = std::string(DEBUG_DRAW_SHOW_LAYER_KEY_PREFIX) + Module::Physics::LayerNames()[i];
            showColliderLayers_[i] = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, key, DEFAULT_SHOW_COLLIDER_LAYER);
        }

        showTriggerColliders_ = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_TRIGGER_COLLIDERS_KEY, DEFAULT_SHOW_TRIGGER_COLLIDERS);

        showMainCameraFrustum_     = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_MAIN_CAMERA_FRUSTUM_KEY, DEFAULT_SHOW_MAIN_CAMERA_FRUSTUM);
        showVirtualCameraFrustums_ = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_VIRTUAL_CAMERA_FRUSTUMS_KEY, DEFAULT_SHOW_VIRTUAL_CAMERA_FRUSTUMS);
    }

    void DebugDrawConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_ALL_COLLIDERS_KEY, showAllColliders_);

        for (size_t i = 0; i < showColliderKinds_.size(); ++i)
        {
            const std::string key = std::string(DEBUG_DRAW_SHOW_COLLIDER_KEY_PREFIX) + Module::Physics::COLLIDER_SHAPE_KIND_NAMES[i];
            Module::ProjectConfig::SaveWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, key, showColliderKinds_[i]);
        }

        for (size_t i = 0; i < static_cast<size_t>(Module::Physics::LayerCount()); ++i)
        {
            const std::string key = std::string(DEBUG_DRAW_SHOW_LAYER_KEY_PREFIX) + Module::Physics::LayerNames()[i];
            Module::ProjectConfig::SaveWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, key, showColliderLayers_[i]);
        }

        Module::ProjectConfig::SaveWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_TRIGGER_COLLIDERS_KEY, showTriggerColliders_);

        Module::ProjectConfig::SaveWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_MAIN_CAMERA_FRUSTUM_KEY, showMainCameraFrustum_);
        Module::ProjectConfig::SaveWithPath<bool>(DEBUG_DRAW_CONFIG_PATH, DEBUG_DRAW_SHOW_VIRTUAL_CAMERA_FRUSTUMS_KEY, showVirtualCameraFrustums_);
    }

    bool DebugDrawConfiguration::ShouldDrawCollider(
        const Module::Physics::ColliderShapeKind kind,
        const Module::Physics::Layer layer,
        const bool isSensor)
    {
        // ProjectConfig に ON が保存されたままでも、ゲームビルドでは描画しない
        if (APPLICATION_MODE != ApplicationMode::Editor)
            return false;

        if (!showAllColliders_)
            return false;

        const auto kindIndex = static_cast<size_t>(kind);
        if (kindIndex >= showColliderKinds_.size() || !showColliderKinds_[kindIndex])
            return false;

        const auto layerIndex = static_cast<size_t>(layer);
        if (layerIndex >= showColliderLayers_.size() || !showColliderLayers_[layerIndex])
            return false;

        return !isSensor || showTriggerColliders_;
    }

    bool DebugDrawConfiguration::ShouldDrawMainCameraFrustum()
    {
        return APPLICATION_MODE == ApplicationMode::Editor && showMainCameraFrustum_;
    }

    bool DebugDrawConfiguration::ShouldDrawVirtualCameraFrustum()
    {
        return APPLICATION_MODE == ApplicationMode::Editor && showVirtualCameraFrustums_;
    }

    void DebugDrawConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Collider");
        ImGui::Separator();

        bool changed = false;

        if (ImGui::Checkbox("Show All Colliders", &showAllColliders_))
            changed = true;

        ImGui::BeginDisabled(!showAllColliders_);

        ImGui::Spacing();
        ImGui::Text("Shape");
        ImGui::PushID("ColliderShape");
        for (size_t i = 0; i < showColliderKinds_.size(); ++i)
        {
            if (ImGui::Checkbox(Module::Physics::COLLIDER_SHAPE_KIND_NAMES[i], &showColliderKinds_[i]))
                changed = true;

            if (static_cast<Module::Physics::ColliderShapeKind>(i) == Module::Physics::ColliderShapeKind::StaticMesh)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(heavy)");
            }
        }
        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Text("Layer");
        ImGui::PushID("ColliderLayer");
        for (size_t i = 0; i < static_cast<size_t>(Module::Physics::LayerCount()); ++i)
        {
            if (ImGui::Checkbox(Module::Physics::LayerNames()[i], &showColliderLayers_[i]))
                changed = true;
        }
        ImGui::PopID();

        ImGui::Spacing();
        if (ImGui::Checkbox("Show Trigger (Sensor)", &showTriggerColliders_))
            changed = true;

        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::TextDisabled("* Trigger colliders are drawn in light blue");
        ImGui::TextDisabled("* The object selected in Inspector always draws its collider");

        ImGui::Spacing();
        ImGui::Text("Camera");
        ImGui::Separator();

        if (ImGui::Checkbox("Show Main Camera Frustum", &showMainCameraFrustum_))
            changed = true;

        if (ImGui::Checkbox("Show Virtual Camera Frustums", &showVirtualCameraFrustums_))
            changed = true;

        if (changed)
            Save();
    }
}
