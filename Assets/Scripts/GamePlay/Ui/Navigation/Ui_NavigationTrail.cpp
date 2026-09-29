#include "Ui_NavigationTrail.h"

#include <algorithm>

#include "glm.hpp"
#include "Ui_NavigationMemory.h"
#include "Ui_NavigationPresenter.h"
#include "Engine/Core/Application/Time/Time.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/PathFinding/PathFinding_GridDirections.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        float HorizontalDistance(const glm::vec3& a, const glm::vec3& b)
        {
            return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
        }
    }

    void NavigationTrail::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();

        const auto& current = NavigationMemory::Instance().Current();
        const auto targetPosition = current ? current->TargetPosition() : std::nullopt;
        const auto player = GameCore::PlayerAvatar::Owner();
        if (!targetPosition || !player)
        {
            Hide();
            return;
        }

        const glm::vec3 playerPosition = player->PlayerTransform().GetWorldPos();
        const bool isShown = !NavigationPresenter::IsQuiet() && glm::length(*targetPosition - playerPosition) > hideDistance_;
        const float step = fade_secs_ > 0.0f ? deltaTime / fade_secs_ : 1.0f;
        visibility_ = std::clamp(visibility_ + (isShown ? step : -step), 0.0f, 1.0f);

        bool hasGridPath = false;
        const auto context = GameCore::Game::Instance().Scenes().CurrentContext();
        if (const auto grid = context ? context->NavigationGrid() : nullptr)
        {
            if (!pathFinder_)
                pathFinder_ = std::make_shared<GameCore::PathFinding::HeightGridAstar>();

            // 目的地が変わったら、前の目的地への道は捨てて探し直す
            const glm::vec2 cellSize = grid->CellSize();
            if (HorizontalDistance(searchedGoal_, *targetPosition) > std::max(cellSize.x, cellSize.y))
            {
                pathFinder_->ClearPath();
                searchedGoal_ = *targetPosition;
            }

            pathFinder_->Tick(grid, playerPosition, { *targetPosition }, GameCore::PathFinding::EIGHT_DIRECTIONS,
                maxCellRange_, maxClimbAngle_deg_, searchInterval_secs_);

            // NOTE: 探索は searchInterval_secs_ ごとなので、その間に通り過ぎた点は前から詰める
            auto& path = pathFinder_->Path();
            const float reach = std::max(cellSize.x, cellSize.y) * 1.5f;
            while (path.size() > 1 && HorizontalDistance(path.front(), playerPosition) < reach)
                path.erase(path.begin());

            hasGridPath = pathFinder_->HasPath() && !path.empty();
        }

        RebuildPolyline(playerPosition, *targetPosition, hasGridPath);

        if (renderer_)
        {
            renderer_->SetPath(polyline_);
            renderer_->SetVisibility(visibility_);
        }
    }

    void NavigationTrail::Hide()
    {
        visibility_ = 0.0f;
        polyline_.clear();
        if (renderer_)
        {
            renderer_->ClearPath();
            renderer_->SetVisibility(0.0f);
        }
    }

    void NavigationTrail::RebuildPolyline(const glm::vec3& playerPosition, const glm::vec3& targetPosition, const bool hasGridPath)
    {
        polyline_.clear();
        polyline_.push_back(playerPosition);
        if (hasGridPath)
        {
            const auto& path = pathFinder_->Path();
            polyline_.insert(polyline_.end(), path.begin(), path.end());
        }
        polyline_.push_back(targetPosition);
    }

    void NavigationTrail::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("renderer_", renderer_);
        ImGuiHelper::OnDrawInputField("maxCellRange_", maxCellRange_);
        ImGuiHelper::OnDrawInputField("maxClimbAngle_deg_", maxClimbAngle_deg_);
        ImGuiHelper::OnDrawInputField("searchInterval_secs_", searchInterval_secs_);
        ImGuiHelper::OnDrawInputField("hideDistance_", hideDistance_);
        ImGuiHelper::OnDrawInputField("fade_secs_", fade_secs_);

        ImGui::Text("path points: %zu, visibility: %.2f", polyline_.size(), visibility_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::NavigationTrail);
#pragma endregion
