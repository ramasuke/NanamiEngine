#include "ApplicationConfiguration_Physics.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "../../../../Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "ImGuiHelper.h"
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto DEFAULT_FIXED_UPDATE_RATE       = 144;
    constexpr auto DEFAULT_MAX_DELTA_TIME          = 0.3f;
    constexpr auto DEFAULT_MAX_PHYSICS_STEP        = 1;
    constexpr auto DEFAULT_GRAVITY_SCALE           = -360.8f;
    constexpr auto DEFAULT_COLLISION_STEPS         = 1;
    // atan(2.0) ≒ 63度までの斜面で滑り出さない
    constexpr auto DEFAULT_STATIC_FRICTION             = 2.0f;
    constexpr auto DEFAULT_STATIC_FRICTION_SPEED       = 30.0f;
    constexpr auto DEFAULT_STATIC_FRICTION_MAX_SLOPE   = 60.0f;
    constexpr auto DEFAULT_TEMP_ALLOCATOR_SIZE_MB  = 10;
    constexpr auto DEFAULT_MAX_BODIES              = 1024;
    constexpr auto DEFAULT_MAX_BODY_PAIRS          = 65536;
    constexpr auto DEFAULT_MAX_CONTACT_CONSTRAINTS = 10240;

    int   PhysicsConfiguration::fixedUpdateRate_       = DEFAULT_FIXED_UPDATE_RATE;
    float PhysicsConfiguration::maxDeltaTime_          = DEFAULT_MAX_DELTA_TIME;
    int   PhysicsConfiguration::maxPhysicsStep_        = DEFAULT_MAX_PHYSICS_STEP;
    float PhysicsConfiguration::gravityScale_          = DEFAULT_GRAVITY_SCALE;
    int   PhysicsConfiguration::collisionSteps_        = DEFAULT_COLLISION_STEPS;
    float PhysicsConfiguration::staticFriction_            = DEFAULT_STATIC_FRICTION;
    float PhysicsConfiguration::staticFrictionSpeed_       = DEFAULT_STATIC_FRICTION_SPEED;
    float PhysicsConfiguration::staticFrictionMaxSlopeDeg_ = DEFAULT_STATIC_FRICTION_MAX_SLOPE;
    int   PhysicsConfiguration::tempAllocatorSizeMB_   = DEFAULT_TEMP_ALLOCATOR_SIZE_MB;
    int   PhysicsConfiguration::maxBodies_             = DEFAULT_MAX_BODIES;
    int   PhysicsConfiguration::maxBodyPairs_          = DEFAULT_MAX_BODY_PAIRS;
    int   PhysicsConfiguration::maxContactConstraints_ = DEFAULT_MAX_CONTACT_CONSTRAINTS;

    constexpr auto PHYSICS_CONFIG_PATH                 = "Physics/";
    constexpr auto PHYSICS_FIXED_UPDATE_RATE_KEY       = "FixedUpdateRate";
    constexpr auto PHYSICS_MAX_DELTA_TIME_KEY          = "MaxDeltaTime";
    constexpr auto PHYSICS_MAX_PHYSICS_STEP_KEY        = "MaxPhysicsStep";
    constexpr auto PHYSICS_GRAVITY_SCALE_KEY           = "GravityScale";
    constexpr auto PHYSICS_COLLISION_STEPS_KEY         = "CollisionSteps";
    constexpr auto PHYSICS_STATIC_FRICTION_KEY             = "StaticFriction";
    constexpr auto PHYSICS_STATIC_FRICTION_SPEED_KEY       = "StaticFrictionSpeed";
    constexpr auto PHYSICS_STATIC_FRICTION_MAX_SLOPE_KEY   = "StaticFrictionMaxSlopeDeg";
    constexpr auto PHYSICS_TEMP_ALLOCATOR_SIZE_MB_KEY  = "TempAllocatorSizeMB";
    constexpr auto PHYSICS_MAX_BODIES_KEY              = "MaxBodies";
    constexpr auto PHYSICS_MAX_BODY_PAIRS_KEY          = "MaxBodyPairs";
    constexpr auto PHYSICS_MAX_CONTACT_CONSTRAINTS_KEY = "MaxContactConstraints";
    constexpr auto PHYSICS_LAYER_NAMES_KEY             = "LayerNames";
    // NOTE: 要素が足りないレイヤーは全レイヤーと衝突する
    constexpr auto PHYSICS_LAYER_COLLISION_MASKS_KEY   = "LayerCollisionMasks";
    constexpr auto LAYER_NAME_BUFFER_SIZE              = 64;

    namespace
    {
        void LoadLayers()
        {
            namespace Physics = Module::Physics;
            const auto names = Module::ProjectConfig::LoadOrDefaultWithPath<std::vector<std::string>>(
                PHYSICS_CONFIG_PATH, PHYSICS_LAYER_NAMES_KEY, { "Default" });
            Physics::PhysicsLayers::SetNames(names);

            const auto masks = Module::ProjectConfig::LoadOrDefaultWithPath<std::vector<Physics::LayerMask>>(
                PHYSICS_CONFIG_PATH, PHYSICS_LAYER_COLLISION_MASKS_KEY, {});
            for (int i = 0; i < Physics::MAX_LAYER_COUNT; ++i)
            {
                const auto mask = i < static_cast<int>(masks.size()) ? masks[i] : Physics::ALL_LAYERS_MASK;
                Physics::PhysicsLayers::SetCollisionMaskOf(static_cast<Physics::Layer>(i), mask);
            }
        }

        void SaveLayers()
        {
            namespace Physics = Module::Physics;
            std::vector<std::string>        names;
            std::vector<Physics::LayerMask> masks;
            for (int i = 0; i < Physics::PhysicsLayers::Count(); ++i)
            {
                names.emplace_back(Physics::PhysicsLayers::Names()[i]);
                masks.push_back(Physics::PhysicsLayers::CollisionMaskOf(static_cast<Physics::Layer>(i)));
            }
            Module::ProjectConfig::SaveWithPath(PHYSICS_CONFIG_PATH, PHYSICS_LAYER_NAMES_KEY,           names);
            Module::ProjectConfig::SaveWithPath(PHYSICS_CONFIG_PATH, PHYSICS_LAYER_COLLISION_MASKS_KEY, masks);
        }

        // 戻り値：変更されたかどうか
        bool DrawLayersGui()
        {
            namespace Physics = Module::Physics;
            bool changed = false;

            ImGui::Spacing();
            ImGui::Text("Layers");
            ImGui::Separator();
            ImGui::TextDisabled("* Layer は番号で保存されるため末尾のみ追加/削除できる");

            std::vector<std::string> names;
            for (int i = 0; i < Physics::PhysicsLayers::Count(); ++i)
                names.emplace_back(Physics::PhysicsLayers::Names()[i]);

            for (int i = 0; i < static_cast<int>(names.size()); ++i)
            {
                ImGui::PushID(i);
                char buffer[LAYER_NAME_BUFFER_SIZE] = {};
                names[i].copy(buffer, sizeof(buffer) - 1);
                ImGui::SetNextItemWidth(200);
                ImGui::BeginDisabled(i == 0);
                if (ImGui::InputText(("Layer " + std::to_string(i)).c_str(), buffer, sizeof(buffer)) && buffer[0] != '\0')
                {
                    names[i] = buffer;
                    changed = true;
                }
                ImGui::EndDisabled();
                ImGui::PopID();
            }

            if (static_cast<int>(names.size()) < Physics::MAX_LAYER_COUNT && ImGui::Button("Add Layer"))
            {
                names.push_back("Layer" + std::to_string(names.size()));
                changed = true;
            }
            if (names.size() > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Remove Last Layer"))
                {
                    names.pop_back();
                    changed = true;
                }
            }
            if (changed)
                Physics::PhysicsLayers::SetNames(names);

            ImGui::Spacing();
            ImGui::Text("Layer Collision Matrix");
            if (Physics::PhysicsLayers::DrawCollisionMatrixGui())
                changed = true;

            return changed;
        }
    }

    void PhysicsConfiguration::Load()
    {
        fixedUpdateRate_       = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_FIXED_UPDATE_RATE_KEY,       DEFAULT_FIXED_UPDATE_RATE);
        maxDeltaTime_          = Module::ProjectConfig::LoadOrDefaultWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_MAX_DELTA_TIME_KEY,          DEFAULT_MAX_DELTA_TIME);
        maxPhysicsStep_        = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_PHYSICS_STEP_KEY,        DEFAULT_MAX_PHYSICS_STEP);
        gravityScale_          = Module::ProjectConfig::LoadOrDefaultWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_GRAVITY_SCALE_KEY,           DEFAULT_GRAVITY_SCALE);
        collisionSteps_        = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_COLLISION_STEPS_KEY,         DEFAULT_COLLISION_STEPS);
        staticFriction_            = Module::ProjectConfig::LoadOrDefaultWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_STATIC_FRICTION_KEY,           DEFAULT_STATIC_FRICTION);
        staticFrictionSpeed_       = Module::ProjectConfig::LoadOrDefaultWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_STATIC_FRICTION_SPEED_KEY,     DEFAULT_STATIC_FRICTION_SPEED);
        staticFrictionMaxSlopeDeg_ = Module::ProjectConfig::LoadOrDefaultWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_STATIC_FRICTION_MAX_SLOPE_KEY, DEFAULT_STATIC_FRICTION_MAX_SLOPE);
        tempAllocatorSizeMB_   = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_TEMP_ALLOCATOR_SIZE_MB_KEY,  DEFAULT_TEMP_ALLOCATOR_SIZE_MB);
        maxBodies_             = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODIES_KEY,              DEFAULT_MAX_BODIES);
        maxBodyPairs_          = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODY_PAIRS_KEY,          DEFAULT_MAX_BODY_PAIRS);
        maxContactConstraints_ = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_CONTACT_CONSTRAINTS_KEY, DEFAULT_MAX_CONTACT_CONSTRAINTS);
        LoadLayers();
    }

    void PhysicsConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_FIXED_UPDATE_RATE_KEY,       fixedUpdateRate_);
        Module::ProjectConfig::SaveWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_MAX_DELTA_TIME_KEY,          maxDeltaTime_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_PHYSICS_STEP_KEY,        maxPhysicsStep_);
        Module::ProjectConfig::SaveWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_GRAVITY_SCALE_KEY,           gravityScale_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_COLLISION_STEPS_KEY,         collisionSteps_);
        Module::ProjectConfig::SaveWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_STATIC_FRICTION_KEY,           staticFriction_);
        Module::ProjectConfig::SaveWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_STATIC_FRICTION_SPEED_KEY,     staticFrictionSpeed_);
        Module::ProjectConfig::SaveWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_STATIC_FRICTION_MAX_SLOPE_KEY, staticFrictionMaxSlopeDeg_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_TEMP_ALLOCATOR_SIZE_MB_KEY,  tempAllocatorSizeMB_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODIES_KEY,              maxBodies_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODY_PAIRS_KEY,          maxBodyPairs_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_CONTACT_CONSTRAINTS_KEY, maxContactConstraints_);
        SaveLayers();
    }

    int   PhysicsConfiguration::GetFixedUpdateRate()           { return fixedUpdateRate_; }
    void  PhysicsConfiguration::SetFixedUpdateRate(int rate)  { fixedUpdateRate_ = rate; }

    float PhysicsConfiguration::GetMaxDeltaTime()             { return maxDeltaTime_; }
    void  PhysicsConfiguration::SetMaxDeltaTime(float t)      { maxDeltaTime_ = t; }

    int   PhysicsConfiguration::GetMaxPhysicsStep()           { return maxPhysicsStep_; }
    void  PhysicsConfiguration::SetMaxPhysicsStep(int step)   { maxPhysicsStep_ = step; }

    float PhysicsConfiguration::GetGravityScale()             { return gravityScale_; }
    void  PhysicsConfiguration::SetGravityScale(float scale)  { gravityScale_ = scale; }

    int  PhysicsConfiguration::GetCollisionSteps()            { return collisionSteps_; }
    void PhysicsConfiguration::SetCollisionSteps(int steps)   { collisionSteps_ = steps; }

    float PhysicsConfiguration::GetStaticFriction()                 { return staticFriction_; }
    void  PhysicsConfiguration::SetStaticFriction(float friction)   { staticFriction_ = friction; }

    float PhysicsConfiguration::GetStaticFrictionSpeed()            { return staticFrictionSpeed_; }
    void  PhysicsConfiguration::SetStaticFrictionSpeed(float speed) { staticFrictionSpeed_ = speed; }

    float PhysicsConfiguration::GetStaticFrictionMaxSlopeDeg()               { return staticFrictionMaxSlopeDeg_; }
    void  PhysicsConfiguration::SetStaticFrictionMaxSlopeDeg(float degree)   { staticFrictionMaxSlopeDeg_ = degree; }

    int  PhysicsConfiguration::GetTempAllocatorSizeMB()          { return tempAllocatorSizeMB_; }
    void PhysicsConfiguration::SetTempAllocatorSizeMB(int sizeMB) { tempAllocatorSizeMB_ = sizeMB; }

    int  PhysicsConfiguration::GetMaxBodies()              { return maxBodies_; }
    void PhysicsConfiguration::SetMaxBodies(int maxBodies) { maxBodies_ = maxBodies; }

    int  PhysicsConfiguration::GetMaxBodyPairs()                { return maxBodyPairs_; }
    void PhysicsConfiguration::SetMaxBodyPairs(int maxBodyPairs) { maxBodyPairs_ = maxBodyPairs; }

    int  PhysicsConfiguration::GetMaxContactConstraints()               { return maxContactConstraints_; }
    void PhysicsConfiguration::SetMaxContactConstraints(int maxConstraints) { maxContactConstraints_ = maxConstraints; }

    void PhysicsConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Physics System");
        ImGui::Separator();

        int  fixedRate = fixedUpdateRate_;
        float maxDt    = maxDeltaTime_;
        int  maxStep   = maxPhysicsStep_;

        ImGui::SetNextItemWidth(100);
        const bool frChanged    = ImGui::InputInt("Fixed Update Rate (Hz)", &fixedRate);
        ImGui::SetNextItemWidth(100);
        const bool maxDtChanged = ImGui::InputFloat("Max Delta Time",       &maxDt, 0.0f, 0.0f, "%.3f");
        ImGui::SetNextItemWidth(100);
        const bool msChanged    = ImGui::InputInt("Max Physics Steps",      &maxStep);

        if (frChanged || maxDtChanged || msChanged)
        {
            fixedUpdateRate_ = fixedRate;
            maxDeltaTime_    = maxDt;
            maxPhysicsStep_  = maxStep;
            Save();
        }

        ImGui::Spacing();

        float gravityScale = gravityScale_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputFloat("Gravity Scale", &gravityScale, 0.0f, 0.0f, "%.1f"))
        {
            gravityScale_ = gravityScale;
            Save();
        }

        int collisionSteps = collisionSteps_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Collision Steps", &collisionSteps))
        {
            if (collisionSteps < 1) collisionSteps = 1;
            collisionSteps_ = collisionSteps;
            Save();
        }

        ImGui::Spacing();
        ImGui::Text("Static Friction");
        ImGui::Separator();
        ImGui::TextDisabled("* 止まりかけた接触だけ摩擦を上げて、斜面で滑り落ちないようにする");

        float staticFriction = staticFriction_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputFloat("Static Friction", &staticFriction, 0.0f, 0.0f, "%.2f"))
        {
            if (staticFriction < 0.0f) staticFriction = 0.0f;
            staticFriction_ = staticFriction;
            Save();
        }

        float staticFrictionSpeed = staticFrictionSpeed_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputFloat("Static Friction Speed", &staticFrictionSpeed, 0.0f, 0.0f, "%.1f"))
        {
            if (staticFrictionSpeed < 0.0f) staticFrictionSpeed = 0.0f;
            staticFrictionSpeed_ = staticFrictionSpeed;
            Save();
        }

        float staticFrictionMaxSlopeDeg = staticFrictionMaxSlopeDeg_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputFloat("Static Friction Max Slope (deg)", &staticFrictionMaxSlopeDeg, 0.0f, 0.0f, "%.1f"))
        {
            if (staticFrictionMaxSlopeDeg <  0.0f) staticFrictionMaxSlopeDeg =  0.0f;
            if (staticFrictionMaxSlopeDeg > 90.0f) staticFrictionMaxSlopeDeg = 90.0f;
            staticFrictionMaxSlopeDeg_ = staticFrictionMaxSlopeDeg;
            Save();
        }

        if (DrawLayersGui())
            Save();

        ImGui::Spacing();
        ImGui::Text("Requires Restart");
        ImGui::Separator();

        int tempSizeMB = tempAllocatorSizeMB_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Temp Allocator (MB)", &tempSizeMB))
        {
            if (tempSizeMB < 1) tempSizeMB = 1;
            tempAllocatorSizeMB_ = tempSizeMB;
            Save();
        }

        int maxBodies = maxBodies_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Max Bodies", &maxBodies))
        {
            if (maxBodies < 1) maxBodies = 1;
            maxBodies_ = maxBodies;
            Save();
        }

        int maxBodyPairs = maxBodyPairs_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Max Body Pairs", &maxBodyPairs))
        {
            if (maxBodyPairs < 1) maxBodyPairs = 1;
            maxBodyPairs_ = maxBodyPairs;
            Save();
        }

        int maxContactConstraints = maxContactConstraints_;
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Max Contact Constraints", &maxContactConstraints))
        {
            if (maxContactConstraints < 1) maxContactConstraints = 1;
            maxContactConstraints_ = maxContactConstraints;
            Save();
        }

        ImGui::TextDisabled("* Restart required to apply");
        ImGui::Spacing();
    }
}
