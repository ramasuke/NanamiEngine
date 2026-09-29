#pragma once
#include <cstdint>
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace GamePlay::Prop
{
    /** @brief ステージで石が飛び去る演出(FloatingStone::PlayDepartAsync)の尺と距離 */
    struct DepartShot
    {
        // NOTE: ボスを倒した直後は攻撃ボタンを連打しているので、始まってしばらくはスキップを受け付けない
        float skipGrace_secs    = 1.5f;
        /** 石のモデルの結晶の中ほど(モデルの単位)。LookAt と光の尾はここに合わせる */
        float stoneCenterHeight = 8.5f;
        /** ボスが倒れきるのを待つ時間 */
        float delay_secs        = 2.5f;
        float shake_secs        = 1.6f;
        float rise_secs         = 2.6f;
        float fly_secs          = 2.4f;
        float hold_secs         = 0.8f;
        float shakeWidth        = 1.2f;
        float riseHeight        = 70.0f;
        float riseTurnDegrees   = 120.0f;
        float flyTurnDegrees    = 540.0f;
        /** 浮き上がった所から飛び去る先。拠点の島の方角(空の高いところ)へ向ける */
        glm::vec3 flyOffset     = glm::vec3(-500.0f, 900.0f, -700.0f);

        void OnDrawGui();

        template<class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(skipGrace_secs));
            archive(CEREAL_NVP(stoneCenterHeight));
            archive(CEREAL_NVP(delay_secs));
            archive(CEREAL_NVP(shake_secs));
            archive(CEREAL_NVP(rise_secs));
            archive(CEREAL_NVP(fly_secs));
            archive(CEREAL_NVP(hold_secs));
            archive(CEREAL_NVP(shakeWidth));
            archive(CEREAL_NVP(riseHeight));
            archive(CEREAL_NVP(riseTurnDegrees));
            archive(CEREAL_NVP(flyTurnDegrees));
            archive(CEREAL_NVP(flyOffset));
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::DepartShot, 0);
#pragma endregion
