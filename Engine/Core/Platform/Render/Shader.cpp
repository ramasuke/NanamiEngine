#include "Shader.h"

#include <cstddef>

#include "DxLib.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"
#include "../AsyncLoad/AsyncLoad.h"

namespace NanamiEngine::Platform::Render
{
    namespace
    {
        static_assert(static_cast<int>(ShaderStage::Vertex) == DX_SHADERTYPE_VERTEX);
        static_assert(static_cast<int>(ShaderStage::Pixel)  == DX_SHADERTYPE_PIXEL);

        // ShaderVertex3D を VERTEX3DSHADER としてそのまま渡すので、並びが同じであること
        static_assert(sizeof(VertexColor8)   == sizeof(COLOR_U8));
        static_assert(sizeof(ShaderVertex3D) == sizeof(VERTEX3DSHADER));
        static_assert(offsetof(ShaderVertex3D, position)       == offsetof(VERTEX3DSHADER, pos));
        static_assert(offsetof(ShaderVertex3D, shaderPosition) == offsetof(VERTEX3DSHADER, spos));
        static_assert(offsetof(ShaderVertex3D, normal)         == offsetof(VERTEX3DSHADER, norm));
        static_assert(offsetof(ShaderVertex3D, tangent)        == offsetof(VERTEX3DSHADER, tan));
        static_assert(offsetof(ShaderVertex3D, binormal)       == offsetof(VERTEX3DSHADER, binorm));
        static_assert(offsetof(ShaderVertex3D, diffuse)        == offsetof(VERTEX3DSHADER, dif));
        static_assert(offsetof(ShaderVertex3D, specular)       == offsetof(VERTEX3DSHADER, spc));
        static_assert(offsetof(ShaderVertex3D, u)              == offsetof(VERTEX3DSHADER, u));
        static_assert(offsetof(ShaderVertex3D, v)              == offsetof(VERTEX3DSHADER, v));
        static_assert(offsetof(ShaderVertex3D, su)             == offsetof(VERTEX3DSHADER, su));
        static_assert(offsetof(ShaderVertex3D, sv)             == offsetof(VERTEX3DSHADER, sv));
        static_assert(offsetof(VertexColor8, b) == offsetof(COLOR_U8, b));
        static_assert(offsetof(VertexColor8, g) == offsetof(COLOR_U8, g));
        static_assert(offsetof(VertexColor8, r) == offsetof(COLOR_U8, r));
        static_assert(offsetof(VertexColor8, a) == offsetof(COLOR_U8, a));
    }

    int ConstantBuffer::Create(const int sizeInBytes)
    {
        // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、Map / Update で完了待ちに入って固まる
        const AsyncLoad::SyncLoadScope sync;
        return CreateShaderConstantBuffer(sizeInBytes);
    }

    void* ConstantBuffer::Map(const int handle)
    {
        return GetBufferShaderConstantBuffer(handle);
    }

    void ConstantBuffer::Update(const int handle)
    {
        UpdateShaderConstantBuffer(handle);
    }

    void ConstantBuffer::Delete(const int handle)
    {
        DeleteShaderConstantBuffer(handle);
    }

    void ConstantBuffer::Bind(const int handle, const ShaderStage stage, const int slot)
    {
        SetShaderConstantBuffer(handle, static_cast<int>(stage), slot);
    }

    int VertexBuffer::Create(const int vertexCount)
    {
        const AsyncLoad::SyncLoadScope sync;
        return CreateVertexBuffer(vertexCount, DX_VERTEX_TYPE_SHADER_3D);
    }

    bool VertexBuffer::SetData(const int handle, const ShaderVertex3D* vertices, const int count, const int offset)
    {
        return SetVertexBufferData(offset, vertices, count, handle) == 0;
    }

    void VertexBuffer::Delete(const int handle)
    {
        DeleteVertexBuffer(handle);
    }

    int IndexBuffer::Create(const int indexCount)
    {
        const AsyncLoad::SyncLoadScope sync;
        return CreateIndexBuffer(indexCount, DX_INDEX_TYPE_32BIT);
    }

    bool IndexBuffer::SetData(const int handle, const std::uint32_t* indices, const int count, const int offset)
    {
        return SetIndexBufferData(offset, indices, count, handle) == 0;
    }

    void IndexBuffer::Delete(const int handle)
    {
        DeleteIndexBuffer(handle);
    }

    void SetVertexShader(const int handle)
    {
        SetUseVertexShader(handle);
    }

    void SetPixelShader(const int handle)
    {
        SetUsePixelShader(handle);
    }

    void DrawIndexedTriangles(const int vertexBufferHandle, const int indexBufferHandle)
    {
        DrawPrimitiveIndexed3DToShader_UseVertexBuffer(vertexBufferHandle, indexBufferHandle, DX_PRIMTYPE_TRIANGLELIST);
    }

    bool RenderState::GetBackCulling()
    {
        return GetUseBackCulling() != FALSE;
    }

    void RenderState::SetBackCulling(const bool enabled)
    {
        SetUseBackCulling(enabled ? TRUE : FALSE);
    }

    void RenderState::SetZBufferEnabled(const bool enabled)
    {
        SetUseZBuffer3D(enabled ? TRUE : FALSE);
    }

    void RenderState::SetZBufferWrite(const bool enabled)
    {
        SetWriteZBuffer3D(enabled ? TRUE : FALSE);
    }

    void RenderState::SetWorldTransform(const glm::mat4& matrix)
    {
        const MATRIX dxMatrix = LibCore::Dxlib::ToDxMatrix(matrix);
        SetTransformToWorld(&dxMatrix);
    }

    void RenderState::ResetWorldTransform()
    {
        const MATRIX identity = MGetIdent();
        SetTransformToWorld(&identity);
    }
}
