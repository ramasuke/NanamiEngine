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

    /** @brief 島が雲の下からせり上がり、階段が架かる演出(ReturningIsland::PlayReturnAsync)の尺と距離 */
    struct IslandReturnShot
    {
        float skipGrace_secs    = 1.5f;
        /** 読み込みが明けてからせり上がり始めるまでの時間 */
        float delay_secs        = 0.6f;
        float rise_secs         = 6.0f;
        float riseDepth         = 900.0f;
        float riseTiltDegrees   = 7.0f;
        /** 島が上がりきってから階段が架かり始めるまでの時間 */
        float stairsDelay_secs    = 0.8f;
        float stairsStep_secs     = 0.7f;
        float stairsInterval_secs = 0.45f;
        float stairsStepDrop      = 40.0f;
        float hold_secs           = 1.8f;

        void OnDrawGui();

        template<class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(skipGrace_secs));
            archive(CEREAL_NVP(delay_secs));
            archive(CEREAL_NVP(rise_secs));
            archive(CEREAL_NVP(riseDepth));
            archive(CEREAL_NVP(riseTiltDegrees));
            archive(CEREAL_NVP(stairsDelay_secs));
            archive(CEREAL_NVP(stairsStep_secs));
            archive(CEREAL_NVP(stairsInterval_secs));
            archive(CEREAL_NVP(stairsStepDrop));
            archive(CEREAL_NVP(hold_secs));
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Prop::DepartShot, 0);
CEREAL_CLASS_VERSION(GamePlay::Prop::ReturnShot, 0);
CEREAL_CLASS_VERSION(GamePlay::Prop::IslandReturnShot, 0);
#pragma endregion
