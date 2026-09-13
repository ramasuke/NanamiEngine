#include "QuadRenderer.h"

#include <vector>

#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        VERTEX3D MakeVertex(const VECTOR& pos, const VECTOR& normal, const COLOR_U8& color, const float u, const float v)
        {
            VERTEX3D vert{};
            vert.pos  = pos;
            vert.norm = normal;
            vert.dif  = color;
            vert.spc  = color;
            vert.u    = u;
            vert.v    = v;
            vert.su   = 0.0f;
            vert.sv   = 0.0f;
            return vert;
        }

        // 板ポリゴンの頂点を構築する。裏面は同じ位置の三角形を巻き順だけ反転させて積むことで、
        // DxLib側のカリング方向(CW/CCW)に依存せず両面から見えるようにする。
        std::vector<VERTEX3D> BuildQuadVertices(const float width, const float height, const bool doubleSided)
        {
            const float hw = width  * 0.5f;
            const float hh = height * 0.5f;

            const VECTOR topLeft     = VGet(-hw,  hh, 0.0f);
            const VECTOR topRight    = VGet( hw,  hh, 0.0f);
            const VECTOR bottomLeft  = VGet(-hw, -hh, 0.0f);
            const VECTOR bottomRight = VGet( hw, -hh, 0.0f);

            const COLOR_U8 white = GetColorU8(255, 255, 255, 255);
            const VECTOR   frontNormal = VGet(0.0f, 0.0f, -1.0f);

            std::vector<VERTEX3D> verts;
            verts.reserve(doubleSided ? 12 : 6);

            // 表面
            verts.push_back(MakeVertex(topLeft,     frontNormal, white, 0.0f, 0.0f));
            verts.push_back(MakeVertex(topRight,    frontNormal, white, 1.0f, 0.0f));
            verts.push_back(MakeVertex(bottomLeft,  frontNormal, white, 0.0f, 1.0f));
            verts.push_back(MakeVertex(topRight,    frontNormal, white, 1.0f, 0.0f));
            verts.push_back(MakeVertex(bottomRight, frontNormal, white, 1.0f, 1.0f));
            verts.push_back(MakeVertex(bottomLeft,  frontNormal, white, 0.0f, 1.0f));

            if (doubleSided)
            {
                const VECTOR backNormal = VGet(0.0f, 0.0f, 1.0f);
                // 裏面(巻き順を反転)
                verts.push_back(MakeVertex(topLeft,     backNormal, white, 0.0f, 0.0f));
                verts.push_back(MakeVertex(bottomLeft,  backNormal, white, 0.0f, 1.0f));
                verts.push_back(MakeVertex(topRight,    backNormal, white, 1.0f, 0.0f));
                verts.push_back(MakeVertex(topRight,    backNormal, white, 1.0f, 0.0f));
                verts.push_back(MakeVertex(bottomLeft,  backNormal, white, 0.0f, 1.0f));
                verts.push_back(MakeVertex(bottomRight, backNormal, white, 1.0f, 1.0f));
            }

            return verts;
        }
    }

    bool QuadRenderer::HasCustomShader() const
    {
        return vsFile_ && psFile_
            && vsFile_->GetVsHandle() != -1
            && psFile_->GetPsHandle() != -1;
    }

    int QuadRenderer::GetOrCreateShaderConstantBufferHandle()
    {
        if (!HasCustomShader())
            return -1;

        if (cbHandle_ == -1)
        {
            // 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、GetBuffer/Set で完了待ちに入って固まるので同期で作る
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            cbHandle_ = CreateShaderConstantBuffer(CUSTOM_SHADER_CB_SIZE);
            SetUseASyncLoadFlag(useASyncLoad);
        }

        return cbHandle_;
    }

    void QuadRenderer::OnRender()
    {
        if (!IsEnable() || !HasCustomShader())
            return;

        const MATRIX worldMat = Transform().GetDxWorldMatrix();
        SetTransformToWorld(&worldMat);

        const int cbHandle = GetOrCreateShaderConstantBufferHandle();

        SetUseVertexShader(vsFile_->GetVsHandle());
        SetUsePixelShader (psFile_->GetPsHandle());
        if (cbHandle != -1)
        {
            SetShaderConstantBuffer(cbHandle, DX_SHADERTYPE_VERTEX, CUSTOM_SHADER_CB_SLOT);
            SetShaderConstantBuffer(cbHandle, DX_SHADERTYPE_PIXEL,  CUSTOM_SHADER_CB_SLOT);
        }

        SetDrawBlendMode (DX_BLENDMODE_ALPHA, 255);
        SetUseZBuffer3D  (TRUE);
        SetWriteZBuffer3D(FALSE);

        const std::vector<VERTEX3D> verts = BuildQuadVertices(width_, height_, doubleSided_);
        DrawPrimitive3DToShader2(verts.data(), static_cast<int>(verts.size()), DX_PRIMTYPE_TRIANGLELIST);

        SetWriteZBuffer3D(TRUE);
        SetDrawBlendMode (DX_BLENDMODE_NOBLEND, 0);
        SetUseVertexShader(-1);
        SetUsePixelShader (-1);
    }

    void QuadRenderer::OnDestroy()
    {
        if (cbHandle_ != -1)
            DeleteShaderConstantBuffer(cbHandle_);
    }

    void QuadRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("width_",       width_);
        ImGuiHelper::OnDrawInputField("height_",      height_);
        ImGuiHelper::OnDrawInputField("doubleSided_", doubleSided_);
        ImGuiHelper::OnDrawInputField("vsFile_",      vsFile_);
        ImGuiHelper::OnDrawInputField("psFile_",      psFile_);
        if (ImGui::Button("OnUpdateShaderConstantBuffer"))
        {
            if (cbHandle_ != -1)
            {
                DeleteShaderConstantBuffer(cbHandle_);
                cbHandle_ = -1;
            }
            GetOrCreateShaderConstantBufferHandle();
        }
        ImGui::Text("cbHandle_: %d  (slot b%d)", cbHandle_, CUSTOM_SHADER_CB_SLOT);
    }
}
