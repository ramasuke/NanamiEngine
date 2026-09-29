#pragma once
#include <cstdint>
#include <string>

#include "cereal/types/string.hpp"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Scene::GrassLand
{
    /** @brief 初めて着いたときの空撮で、島の見どころを1か所映すショット。前のショットからは切り替えで入る */
    struct StageArrivalTourShot
    {
        /** 見どころの名前。見出しなので全角スペースで字間を空ける (「村 の 跡」) */
        std::string title;
        /** 名前の下に出す一言 */
        std::string subtitle;
        /** カメラの始点と終点 (ワールド座標) */
        glm::vec3 cameraStart = glm::vec3(0.0f);
        glm::vec3 cameraEnd   = glm::vec3(0.0f);
        /** 注視する見どころの位置 (ワールド座標) */
        glm::vec3 lookAt      = glm::vec3(0.0f);
        int duration_msecs    = 4500;

        void OnDrawGui();

        template<class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(title));
            archive(CEREAL_NVP(subtitle));
            archive(CEREAL_NVP(cameraStart));
            archive(CEREAL_NVP(cameraEnd));
            archive(CEREAL_NVP(lookAt));
            archive(CEREAL_NVP(duration_msecs));
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::GrassLand::StageArrivalTourShot, 0);
#pragma endregion
