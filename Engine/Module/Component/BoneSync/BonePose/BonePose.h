#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../../../Libs/glm/glm.hpp"
#include "../../../../../Libs/glm/gtc/quaternion.hpp"

namespace NanamiEngine::Module::Bone
{
    //NOTE: 値オブジェクト
    struct NANAMI_API BonePose final
    {
        BonePose(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
        /** @brief TRS 行列を位置・回転・スケールに分解する。回転はスケールを除いてから抽出する */
        [[nodiscard]] static BonePose FromMatrix(const glm::mat4& matrix);

        [[nodiscard]] const glm::vec3& Position() const { return position_; }
        [[nodiscard]] const glm::quat& Rotation() const { return rotation_; }
        [[nodiscard]] const glm::vec3& Scale   () const { return scale_;    }

        bool operator==(const BonePose& other) const;
        bool operator!=(const BonePose& other) const { return !(*this == other); }

    private:
        glm::vec3 position_;
        glm::quat rotation_;
        glm::vec3 scale_;
    };
}
