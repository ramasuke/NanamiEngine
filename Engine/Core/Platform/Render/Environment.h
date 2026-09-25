#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "vec3.hpp"
#include "../../../Module/Color/Color32.h"

// フォグと標準ライトの設定 (DxLib の SetFog* / SetLightDifColor などの DxLib を出さない入口)
namespace NanamiEngine::Platform::Render::Environment
{
    NANAMI_API void SetFogEnabled(bool enabled);
    NANAMI_API void SetFogColor(const Color32& color);
    NANAMI_API void SetFogStartEnd(float start, float end);

    /** @param rgb01 各成分 0..1 */
    NANAMI_API void      SetLightDiffuseColor(const glm::vec3& rgb01);
    [[nodiscard]] NANAMI_API glm::vec3 GetLightDiffuseColor();
    [[nodiscard]] NANAMI_API glm::vec3 GetLightDirection();
}
