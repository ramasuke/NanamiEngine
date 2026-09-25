#pragma once
#include "vec3.hpp"
#include "../../../Module/Color/Color32.h"

// フォグと標準ライトの設定 (DxLib の SetFog* / SetLightDifColor などの DxLib を出さない入口)
namespace NanamiEngine::Platform::Render::Environment
{
    void SetFogEnabled(bool enabled);
    void SetFogColor(const Color32& color);
    void SetFogStartEnd(float start, float end);

    /** @param rgb01 各成分 0..1 */
    void      SetLightDiffuseColor(const glm::vec3& rgb01);
    [[nodiscard]] glm::vec3 GetLightDiffuseColor();
    [[nodiscard]] glm::vec3 GetLightDirection();
}
