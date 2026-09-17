#include "BonePose.h"

namespace NanamiEngine::Module::Bone
{
    BonePose::BonePose(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
        : position_(position)
        , rotation_(rotation)
        , scale_(scale)
    {
    }

    BonePose BonePose::FromMatrix(const glm::mat4& matrix)
    {
        const glm::vec3 scale = {
            glm::length(glm::vec3(matrix[0])),
            glm::length(glm::vec3(matrix[1])),
            glm::length(glm::vec3(matrix[2]))
        };

        glm::mat3 rotationBasis(matrix);
        rotationBasis[0] = scale.x > 1e-8f ? rotationBasis[0] / scale.x : glm::vec3(1.0f, 0.0f, 0.0f);
        rotationBasis[1] = scale.y > 1e-8f ? rotationBasis[1] / scale.y : glm::vec3(0.0f, 1.0f, 0.0f);
        rotationBasis[2] = scale.z > 1e-8f ? rotationBasis[2] / scale.z : glm::vec3(0.0f, 0.0f, 1.0f);

        return BonePose(glm::vec3(matrix[3]), glm::normalize(glm::quat_cast(rotationBasis)), scale);
    }

    bool BonePose::operator==(const BonePose& other) const
    {
        return position_ == other.position_
            && rotation_ == other.rotation_
            && scale_    == other.scale_;
    }
}
