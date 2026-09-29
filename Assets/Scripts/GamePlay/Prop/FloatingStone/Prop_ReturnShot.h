#pragma once
#include <cstdint>
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace GamePlay::Prop
{
    /** @brief 拠点の島へ石が戻ってはまる演出(FloatingStone::PlayReturnAsync)の尺と距離 */
    struct ReturnShot
    {
        float skipGrace_secs    = 1.5f;
        /** 石のモデルの結晶の中ほど(モデルの単位)。LookAt と光の尾はここに合わせる */
        float stoneCenterHeight = 8.5f;
        /** 読み込みが明けてから飛んでくるまでの時間 */
        float delay_secs        = 1.0f;
        float fly_secs          = 4.0f;
        float settle_secs       = 1.4f;
        float hold_secs         = 2.2f;
        float flyTurnDegrees    = 540.0f;
        /** はまる位置から見た飛び始めの位置 */
        glm::vec3 startOffset    = glm::vec3(900.0f, -350.0f, 900.0f);
        /** はまる位置から見た、減速し始める位置(底の真下) */
        glm::vec3 approachOffset = glm::vec3(0.0f, -90.0f, 0.0f);

        void OnDrawGui();

        template<class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(skipGrace_secs));
            archive(CEREAL_NVP(stoneCenterHeight));
            archive(CEREAL_NVP(delay_secs));
            archive(CEREAL_NVP(fly_secs));
            archive(CEREAL_NVP(settle_secs));
            archive(CEREAL_NVP(hold_secs));
            archive(CEREAL_NVP(flyTurnDegrees));
            archive(CEREAL_NVP(startOffset));
            archive(CEREAL_NVP(approachOffset));
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::ReturnShot, 0);
#pragma endregion
