#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>

#include "vec3.hpp"
#include "vec4.hpp"
#include "mat4x4.hpp"
#include "../../../Module/Color/Color32.h"

// カスタムシェーダー描画の DxLib を出さない入口: 定数バッファ、頂点 / インデックスバッファ、描画ステート。
// ハンドルはすべて DxLib の int。ShaderVertex3D は DxLib の VERTEX3DSHADER と同じ並び (.cpp で static_assert)
namespace NanamiEngine::Platform::Render
{
    enum class ShaderStage : int
    {
        Vertex = 0,
        Pixel  = 1,
    };

    /** 頂点色 (DxLib の COLOR_U8 と同じ b, g, r, a の順) */
    struct NANAMI_API VertexColor8
    {
        std::uint8_t b = 255, g = 255, r = 255, a = 255;

        [[nodiscard]] static VertexColor8 FromColor32(const Color32& color, std::uint8_t alpha = 255) { return { color.B(), color.G(), color.R(), alpha }; }
        [[nodiscard]] static VertexColor8 Gray(const std::uint8_t value, const std::uint8_t alpha = 255) { return { value, value, value, alpha }; }
    };

    /** シェーダー用 3D 頂点 (DxLib の VERTEX3DSHADER) */
    struct NANAMI_API ShaderVertex3D
    {
        glm::vec3    position{};
        glm::vec4    shaderPosition{}; // spos: シェーダーが自由に使う 4 成分
        glm::vec3    normal{};
        glm::vec3    tangent{};
        glm::vec3    binormal{};
        VertexColor8 diffuse{};
        VertexColor8 specular{};
        float        u = 0.0f, v = 0.0f;
        float        su = 0.0f, sv = 0.0f;
    };

    namespace ConstantBuffer
    {
        /** @brief 定数バッファを同期で作る (非同期読み込みが有効でも完了待ちにならない)。失敗で -1 */
        [[nodiscard]] NANAMI_API int   Create(int sizeInBytes);
        /** @brief CPU 側の書き込み先。書いたら Update を呼ぶ */
        [[nodiscard]] NANAMI_API void* Map(int handle);
        NANAMI_API void                Update(int handle);
        NANAMI_API void                Delete(int handle);
        NANAMI_API void                Bind(int handle, ShaderStage stage, int slot);
    }

    namespace VertexBuffer
    {
        /** @brief ShaderVertex3D 用の頂点バッファ。失敗で -1 */
        [[nodiscard]] NANAMI_API int Create(int vertexCount);
        NANAMI_API bool              SetData(int handle, const ShaderVertex3D* vertices, int count, int offset = 0);
        NANAMI_API void              Delete(int handle);
    }

    namespace IndexBuffer
    {
        /** @brief 32bit インデックスバッファ。失敗で -1 */
        [[nodiscard]] NANAMI_API int Create(int indexCount);
        NANAMI_API bool              SetData(int handle, const std::uint32_t* indices, int count, int offset = 0);
        NANAMI_API void              Delete(int handle);
    }

    /** @brief -1 で標準シェーダーに戻す */
    NANAMI_API void SetVertexShader(int handle);
    NANAMI_API void SetPixelShader(int handle);
    /** @brief 設定中の頂点 / ピクセルシェーダーで三角形リストを描く */
    NANAMI_API void DrawIndexedTriangles(int vertexBufferHandle, int indexBufferHandle);

    namespace RenderState
    {
        [[nodiscard]] NANAMI_API bool GetBackCulling();
        NANAMI_API void SetBackCulling(bool enabled);
        NANAMI_API void SetZBufferEnabled(bool enabled);
        NANAMI_API void SetZBufferWrite(bool enabled);
        NANAMI_API void SetWorldTransform(const glm::mat4& matrix);
        NANAMI_API void ResetWorldTransform();
    }
}
