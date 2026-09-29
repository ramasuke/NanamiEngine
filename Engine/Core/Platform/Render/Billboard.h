#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "vec3.hpp"

namespace NanamiEngine::Platform::Render::Billboard
{
    /**
     * @param center      板の中心 (ワールド座標)
     * @param size        板の横幅 (ワールド単位)
     * @param angle       画面上での回転 (ラジアン)
     * @param graphHandle 画像のハンドル (SpriteFile::GetDxLibHandle)
     */
    NANAMI_API void Draw(const glm::vec3& center, float size, float angle, int graphHandle);
}
