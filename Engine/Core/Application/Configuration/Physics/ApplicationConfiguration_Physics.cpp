#include "ApplicationConfiguration_Physics.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto DEFAULT_GRAVITY_SCALE           = -360.8f;
    constexpr auto DEFAULT_COLLISION_STEPS         = 1;
    constexpr auto DEFAULT_TEMP_ALLOCATOR_SIZE_MB  = 10;
    constexpr auto DEFAULT_MAX_BODIES              = 1024;
    constexpr auto DEFAULT_MAX_BODY_PAIRS          = 1024;
    constexpr auto DEFAULT_MAX_CONTACT_CONSTRAINTS = 1024;

    float PhysicsConfiguration::gravityScale_          = DEFAULT_GRAVITY_SCALE;
    int   PhysicsConfiguration::collisionSteps_        = DEFAULT_COLLISION_STEPS;
    int   PhysicsConfiguration::tempAllocatorSizeMB_   = DEFAULT_TEMP_ALLOCATOR_SIZE_MB;
    int   PhysicsConfiguration::maxBodies_             = DEFAULT_MAX_BODIES;
    int   PhysicsConfiguration::maxBodyPairs_          = DEFAULT_MAX_BODY_PAIRS;
    int   PhysicsConfiguration::maxContactConstraints_ = DEFAULT_MAX_CONTACT_CONSTRAINTS;

    constexpr auto PHYSICS_CONFIG_PATH                 = "Physics/";
    constexpr auto PHYSICS_GRAVITY_SCALE_KEY           = "GravityScale";
    constexpr auto PHYSICS_COLLISION_STEPS_KEY         = "CollisionSteps";
    constexpr auto PHYSICS_TEMP_ALLOCATOR_SIZE_MB_KEY  = "TempAllocatorSizeMB";
    constexpr auto PHYSICS_MAX_BODIES_KEY              = "MaxBodies";
    constexpr auto PHYSICS_MAX_BODY_PAIRS_KEY          = "MaxBodyPairs";
    constexpr auto PHYSICS_MAX_CONTACT_CONSTRAINTS_KEY = "MaxContactConstraints";

    void PhysicsConfiguration::Load()
    {
        gravityScale_          = Module::ProjectConfig::LoadOrDefaultWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_GRAVITY_SCALE_KEY,           DEFAULT_GRAVITY_SCALE);
        collisionSteps_        = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_COLLISION_STEPS_KEY,         DEFAULT_COLLISION_STEPS);
        tempAllocatorSizeMB_   = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_TEMP_ALLOCATOR_SIZE_MB_KEY,  DEFAULT_TEMP_ALLOCATOR_SIZE_MB);
        maxBodies_             = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODIES_KEY,              DEFAULT_MAX_BODIES);
        maxBodyPairs_          = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODY_PAIRS_KEY,          DEFAULT_MAX_BODY_PAIRS);
        maxContactConstraints_ = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_CONTACT_CONSTRAINTS_KEY, DEFAULT_MAX_CONTACT_CONSTRAINTS);
    }

    void PhysicsConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<float>(PHYSICS_CONFIG_PATH, PHYSICS_GRAVITY_SCALE_KEY,           gravityScale_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_COLLISION_STEPS_KEY,         collisionSteps_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_TEMP_ALLOCATOR_SIZE_MB_KEY,  tempAllocatorSizeMB_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODIES_KEY,              maxBodies_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_BODY_PAIRS_KEY,          maxBodyPairs_);
        Module::ProjectConfig::SaveWithPath<int>  (PHYSICS_CONFIG_PATH, PHYSICS_MAX_CONTACT_CONSTRAINTS_KEY, maxContactConstraints_);
    }

    float PhysicsConfiguration::GetGravityScale()             { return gravityScale_; }
    void  PhysicsConfiguration::SetGravityScale(float scale)  { gravityScale_ = scale; }

    int  PhysicsConfiguration::GetCollisionSteps()            { return collisionSteps_; }
    void PhysicsConfiguration::SetCollisionSteps(int steps)   { collisionSteps_ = steps; }

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
