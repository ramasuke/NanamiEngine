#include "Story_IslandHeartDeparture.h"

namespace GameCore::Story
{
    namespace
    {
        std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> s_heart;
        glm::vec3 s_position{};
    }

    void IslandHeartDeparture::Notify(const std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject>& heart, const glm::vec3& position)
    {
        s_heart    = heart;
        s_position = position;
    }

    std::optional<glm::vec3> IslandHeartDeparture::DepartedFrom()
    {
        if (s_heart.expired())
            return std::nullopt;
        return s_position;
    }
}
