#include "Main_SceneContextBase.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene
{
    void SceneContextBase::Init()
    {
        playerSpawnPoint_   .Init();
        navigationGrid_     .Init();
        for (auto& target : navigationTargets_)
            target.object_.Init();
    }

    glm::vec3 SceneContextBase::PlayerSpawnPoint() const
    {
        return playerSpawnPoint_->Transform().GetWorldPos();
    }

    glm::quat SceneContextBase::PlayerSpawnRotation() const
    {
        return playerSpawnPoint_->Transform().GetWorldRot();
    }

    std::optional<Navigation::NavigationTargetRef> SceneContextBase::FindNavigationTarget(const std::string_view id) const
    {
        for (const auto& target : navigationTargets_)
        {
            if (target.id_ != id)
                continue;

            auto object = target.object_.get();
            if (!object)
                return std::nullopt;

            return Navigation::NavigationTargetRef{ std::move(object), target.markerHeight_ };
        }
        return std::nullopt;
    }

    void SceneContextBase::SetNavigationObjective(const std::string& id, const bool active)
    {
        const bool changed = active ? navigationObjectives_.insert(id).second
                                    : navigationObjectives_.erase(id) > 0;
        if (changed)
            onNavigationObjectivesChanged_.OnNext(NanamiEngine::R4::Unit{});
    }

    void SceneContextBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("loadSceneFile_", loadSceneFile_);
        ImGuiHelper::OnDrawInputField("playerSpawnPoint_", playerSpawnPoint_);
        ImGuiHelper::OnDrawInputField("playerAvatarFactory_", playerAvatarFactory_);
        ImGuiHelper::OnDrawInputField("navigationGrid_", navigationGrid_);
        if (ImGui::TreeNode("navigationTargets_"))
        {
            for (std::size_t i = 0; i < navigationTargets_.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                const bool remove = navigationTargets_[i].OnDrawGui();
                ImGui::Separator();
                ImGui::PopID();
                if (remove)
                {
                    navigationTargets_.erase(navigationTargets_.begin() + static_cast<std::ptrdiff_t>(i));
                    break;
                }
            }
            if (ImGui::Button("Add"))
                navigationTargets_.emplace_back();
            ImGui::TreePop();
        }
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Scene::SceneContextBase);
#pragma endregion
