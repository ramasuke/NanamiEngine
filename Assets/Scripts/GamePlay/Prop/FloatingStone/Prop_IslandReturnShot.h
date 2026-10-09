#pragma once
#include <cstdint>
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace GamePlay::Prop
{
    // NOTE: 島が雲の下からせり上がり、階段が架かる演出の尺と距離
    struct IslandReturnShot
    {
        float skipGrace_secs    = 1.5f;
        // NOTE: 読み込みが明けてからせり上がり始めるまでの時間
        float delay_secs        = 0.6f;
        float rise_secs         = 6.0f;
        float riseDepth         = 900.0f;
        float riseTiltDegrees   = 7.0f;
        // NOTE: 島が上がりきってから階段が架かり始めるまでの時間
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
CEREAL_CLASS_VERSION(GamePlay::Prop::IslandReturnShot, 0);
#pragma endregion
