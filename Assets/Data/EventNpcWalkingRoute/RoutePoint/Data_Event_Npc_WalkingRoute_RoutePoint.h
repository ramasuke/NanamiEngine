#pragma once
#include "cereal/cereal.hpp"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace Data::EventNpcWalkingRoute
{
    struct RoutePoint final
    {
        explicit RoutePoint(const glm::vec3& pos = glm::vec3(0.0f), float duration_secs = 1.0f);
        [[nodiscard]] const glm::vec3& Position      () const { return position_; }
        [[nodiscard]] int              Duration_msecs() const { return duration_secs_ * 1000.0f; }
        void OnDrawGui();

    private:
        glm::vec3 position_;
        float duration_secs_;
#pragma region Serialization Function
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(position_));
            archive(CEREAL_NVP(duration_secs_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(position_));
            archive(CEREAL_NVP(duration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(Data::EventNpcWalkingRoute::RoutePoint, 0)