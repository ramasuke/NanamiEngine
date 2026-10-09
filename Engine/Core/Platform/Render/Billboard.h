#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "vec3.hpp"

namespace NanamiEngine::Platform::Render::Billboard
{
    // NOTE: カメラ正対の板を描く。center はワールド座標、size はワールド単位の横幅、angle は画面上の回転 (ラジアン)
    NANAMI_API void Draw(const glm::vec3& center, float size, float angle, int graphHandle);
}
