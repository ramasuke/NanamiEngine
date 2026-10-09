#pragma once
#include <memory>
#include <optional>
#include <string>

#include "vec3.hpp"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    // NOTE: プレイヤーに示す次にすること
    struct NavigationObjective
    {
        std::string stepId;
        std::string title;
        
        std::string label;
        std::string targetId;
        
        std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> target;
        float markerHeight = 2.0f;

        // NOTE: 目的地の足元
        [[nodiscard]] std::optional<glm::vec3> TargetPosition() const
        {
            const auto object = target.lock();
            if (!object)
                return std::nullopt;
            return object->Transform().GetWorldPos();
        }

        // NOTE: 目印を出す位置
        [[nodiscard]] std::optional<glm::vec3> MarkerPosition() const
        {
            auto position = TargetPosition();
            if (position)
                position->y += markerHeight;
            return position;
        }
    };
}
