#pragma once
#include <memory>
#include <vector>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/PathSpriteTrailRenderer/PathSpriteTrailRenderer.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../Core/Game/PathFinding/HeightGridAstar/Multithread/PathFinding_HeightGridAstar_Multithread.h"

namespace GamePlay::Ui
{
    /**
     * @brief プレイヤーから目的地への道を求め、renderer_ に渡して蛍を流させる。
     */
    class NavigationTrail final : public Component::ComponentBase,
                                  public LifeCycleCallback::IUpdatable
    {
    private:
        void OnUpdate() override;
        void Hide();

        void RebuildPolyline(const glm::vec3& playerPosition, const glm::vec3& targetPosition, bool hasGridPath);

        [[serialize(1)]] FIELD(Component::PathSpriteTrailRenderer) renderer_;
        [[serialize(0)]] int                      maxCellRange_ = 400;
        [[serialize(0)]] float                    maxClimbAngle_deg_ = 45.0f;
        [[serialize(0)]] float                    searchInterval_secs_ = 1.0f;
        /** @brief 目的地にこれより近いと出さない */
        [[serialize(0)]] float                    hideDistance_ = 45.0f;
        [[serialize(0)]] float                    fade_secs_ = 0.4f;

        /** @brief ワーカースレッドを持つので動かせない。使うときに作る */
        std::shared_ptr<GameCore::PathFinding::HeightGridAstar> pathFinder_;
        std::vector<glm::vec3>                 polyline_;

        glm::vec3                              searchedGoal_ = glm::vec3(0.0f);
        float                                  visibility_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderer_));
            archive(CEREAL_NVP(maxCellRange_));
            archive(CEREAL_NVP(maxClimbAngle_deg_));
            archive(CEREAL_NVP(searchInterval_secs_));
            archive(CEREAL_NVP(hideDistance_));
            archive(CEREAL_NVP(fade_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(renderer_));
            if (version >= 0) archive(CEREAL_NVP(maxCellRange_));
            if (version >= 0) archive(CEREAL_NVP(maxClimbAngle_deg_));
            if (version >= 0) archive(CEREAL_NVP(searchInterval_secs_));
            if (version >= 0) archive(CEREAL_NVP(hideDistance_));
            if (version >= 0) archive(CEREAL_NVP(fade_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::NavigationTrail, 1);
