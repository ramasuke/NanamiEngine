#include "GrassRenderer.h"

#include <cmath>
#include <limits>

#include "DxLib.h"
#include "glm.hpp"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../Weather/WindZone.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    namespace
    {
        constexpr int           GRASS_VERTICES_PER_BLADE = 5;
        constexpr int           GRASS_INDICES_PER_BLADE  = 9;
        constexpr std::uint32_t GRASS_BLADE_INDICES[GRASS_INDICES_PER_BLADE] = {0, 1, 2, 1, 3, 2, 2, 3, 4};
        constexpr float         GRASS_MID_HEIGHT_RATIO   = 0.55f;
        constexpr float         GRASS_MID_WIDTH_RATIO    = 0.7f;

        // spos.xyz に根元の位置、w に揺れの位相を入れて、葉全体が同じ位相で揺れるようにする。
        // u は根元からの高さ比率、v は葉の高さ(揺れで伸びて見えない補正に使う)
        VERTEX3DSHADER MakeGrassVertex(const glm::vec3& position, const glm::vec3& root, const glm::vec3& normal,
                                       const COLOR_U8& color, const float heightRatio, const float bladeHeight,
                                       const float phase01)
        {
            VERTEX3DSHADER vertex{};
            vertex.pos    = VGet(position.x, position.y, position.z);
            vertex.spos.x = root.x;
            vertex.spos.y = root.y;
            vertex.spos.z = root.z;
            vertex.spos.w = phase01;
            vertex.norm   = VGet(normal.x, normal.y, normal.z);
            vertex.tan    = VGet(0.0f, 0.0f, 0.0f);
            vertex.binorm = VGet(0.0f, 0.0f, 0.0f);
            vertex.dif    = color;
            vertex.spc    = color;
            vertex.u      = heightRatio;
            vertex.v      = bladeHeight;
            vertex.su     = 0.0f;
            vertex.sv     = 0.0f;
            return vertex;
        }

        void SetGrassFloat4(float* destination, const float x, const float y, const float z, const float w)
        {
            destination[0] = x;
            destination[1] = y;
            destination[2] = z;
            destination[3] = w;
        }
    }

    GrassRenderer::~GrassRenderer()
    {
        ReleaseBuffers();
        ReleaseConstantBuffer();
    }

    bool GrassRenderer::HasCustomShader() const
    {
        return vsFile_ && psFile_
            && vsFile_->GetVsHandle() != -1
            && psFile_->GetPsHandle() != -1;
    }

    int GrassRenderer::GetOrCreateShaderConstantBufferHandle()
    {
        if (!HasCustomShader())
            return -1;

        if (cbHandle_ == -1)
        {
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            cbHandle_ = CreateShaderConstantBuffer(Component::CUSTOM_SHADER_CB_SIZE);
            SetUseASyncLoadFlag(useASyncLoad);
        }

        return cbHandle_;
    }

    void GrassRenderer::ReleaseBuffers()
    {
        for (const auto& buffer : chunkBuffers_)
        {
            if (buffer.vertexBuffer != -1)
                DeleteVertexBuffer(buffer.vertexBuffer);
            if (buffer.indexBuffer != -1)
                DeleteIndexBuffer(buffer.indexBuffer);
        }
        chunkBuffers_.clear();
    }

    void GrassRenderer::ReleaseConstantBuffer()
    {
        if (cbHandle_ == -1)
            return;

        DeleteShaderConstantBuffer(cbHandle_);
        cbHandle_ = -1;
    }

    void GrassRenderer::RebuildBuffers(const Asset::GrassField& field)
    {
        ReleaseBuffers();

        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        // 読み込み中のハンドルになると Set*BufferData で完了待ちに入るので同期で作る
        const int useASyncLoad = GetUseASyncLoadFlag();
        SetUseASyncLoadFlag(FALSE);

        std::vector<VERTEX3DSHADER> vertices;
        std::vector<std::uint32_t>  indices;
        for (const auto& [key, chunk] : field.Chunks())
        {
            if (chunk.blades.empty())
                continue;

            vertices.clear();
            indices.clear();
            vertices.reserve(chunk.blades.size() * GRASS_VERTICES_PER_BLADE);
            indices .reserve(chunk.blades.size() * GRASS_INDICES_PER_BLADE);

            glm::vec3 rootMin(std::numeric_limits<float>::max());
            glm::vec3 rootMax(std::numeric_limits<float>::lowest());

            for (const auto& blade : chunk.blades)
            {
                const glm::vec3 root      = field.DecodeBlade(key, chunk, blade);
                const auto      variation = Asset::GrassField::Variation(key, blade);

                const float height = std::lerp(field.HeightMin(), field.HeightMax(), variation.height01);
                const float width  = std::lerp(field.WidthMin(),  field.WidthMax(),  variation.width01);
                const float bend   = height * field.BendAmount();

                const glm::vec3 forward(std::sin(variation.yaw), 0.0f, std::cos(variation.yaw));
                const glm::vec3 right  (forward.z, 0.0f, -forward.x);
                const glm::vec3 normal = glm::normalize(forward + up * 0.6f);

                const auto     shade = static_cast<unsigned char>(255.0f * (1.0f - field.ColorVariation() * variation.color01));
                const COLOR_U8 color = GetColorU8(shade, shade, shade, 255);

                const float     halfWidth    = width * 0.5f;
                const float     midHalfWidth = halfWidth * GRASS_MID_WIDTH_RATIO;
                const glm::vec3 mid = root + up * (height * GRASS_MID_HEIGHT_RATIO)
                                           + forward * (bend * GRASS_MID_HEIGHT_RATIO * GRASS_MID_HEIGHT_RATIO);
                const glm::vec3 tip = root + up * height + forward * bend;

                const auto baseIndex = static_cast<std::uint32_t>(vertices.size());
                vertices.push_back(MakeGrassVertex(root - right * halfWidth,   root, normal, color, 0.0f,                   height, variation.phase01));
                vertices.push_back(MakeGrassVertex(root + right * halfWidth,   root, normal, color, 0.0f,                   height, variation.phase01));
                vertices.push_back(MakeGrassVertex(mid  - right * midHalfWidth, root, normal, color, GRASS_MID_HEIGHT_RATIO, height, variation.phase01));
                vertices.push_back(MakeGrassVertex(mid  + right * midHalfWidth, root, normal, color, GRASS_MID_HEIGHT_RATIO, height, variation.phase01));
                vertices.push_back(MakeGrassVertex(tip,                         root, normal, color, 1.0f,                   height, variation.phase01));
                for (const auto index : GRASS_BLADE_INDICES)
                    indices.push_back(baseIndex + index);

                rootMin = glm::min(rootMin, root);
                rootMax = glm::max(rootMax, root);
            }

            ChunkBuffer buffer;
            buffer.vertexBuffer = CreateVertexBuffer(static_cast<int>(vertices.size()), DX_VERTEX_TYPE_SHADER_3D);
            buffer.indexBuffer  = CreateIndexBuffer (static_cast<int>(indices.size()),  DX_INDEX_TYPE_32BIT);
            buffer.rootMin      = rootMin;
            buffer.rootMax      = rootMax;
            chunkBuffers_.push_back(buffer);

            if (buffer.vertexBuffer == -1 || buffer.indexBuffer == -1)
            {
                Module::LogError("GrassRenderer: 頂点バッファの作成に失敗しました");
                continue;
            }
            SetVertexBufferData(0, vertices.data(), static_cast<int>(vertices.size()), buffer.vertexBuffer);
            SetIndexBufferData (0, indices.data(),  static_cast<int>(indices.size()),  buffer.indexBuffer);
        }

        SetUseASyncLoadFlag(useASyncLoad);
    }

    void GrassRenderer::WriteConstantBuffer(const Asset::GrassField& field, const int cbHandle) const
    {
        auto* cb = static_cast<GrassCB*>(GetBufferShaderConstantBuffer(cbHandle));
        if (!cb)
            return;

        const glm::vec2 windDirection  = Weather::WindZone::GetDirection();
        const glm::vec3 baseColor      = field.BaseColor();
        const glm::vec3 tipColor       = field.TipColor();
        const VECTOR    lightDirection = GetLightDirection();
        const COLOR_F   lightColor     = GetLightDifColor();

        SetGrassFloat4(cb->wind,           Time::CurrentTime(),
                                           field.WindStrength() * Weather::WindZone::GetStrength01(),
                                           Weather::WindZone::GetSpeed(),
                                           Weather::WindZone::GetFrequency());
        SetGrassFloat4(cb->windDirection,  windDirection.x, 0.0f, windDirection.y, 0.0f);
        SetGrassFloat4(cb->baseColor,      baseColor.r, baseColor.g, baseColor.b, 1.0f);
        SetGrassFloat4(cb->tipColor,       tipColor.r,  tipColor.g,  tipColor.b,  1.0f);
        SetGrassFloat4(cb->lightDirection, lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
        SetGrassFloat4(cb->lightColor,     lightColor.r, lightColor.g, lightColor.b, field.Ambient());

        UpdateShaderConstantBuffer(cbHandle);
    }

    void GrassRenderer::OnRender()
    {
        if (!IsEnable() || !grassField_ || !HasCustomShader())
            return;

        const auto field = grassField_.get();
        if (!field)
            return;

        if (field.get() != builtField_ || field->Revision() != builtRevision_)
        {
            RebuildBuffers(*field);
            builtField_    = field.get();
            builtRevision_ = field->Revision();
        }

        drawnChunkCount_ = 0;
        if (chunkBuffers_.empty())
            return;

        const int cbHandle = GetOrCreateShaderConstantBufferHandle();
        if (cbHandle == -1)
            return;

        // 編集モードでは OnUpdate が回らないので、描画のたびに書き込んで編集中も揺らす
        WriteConstantBuffer(*field, cbHandle);

        const MATRIX identity = MGetIdent();
        SetTransformToWorld(&identity);

        SetUseVertexShader(vsFile_->GetVsHandle());
        SetUsePixelShader (psFile_->GetPsHandle());
        SetShaderConstantBuffer(cbHandle, DX_SHADERTYPE_VERTEX, Component::CUSTOM_SHADER_CB_SLOT);
        SetShaderConstantBuffer(cbHandle, DX_SHADERTYPE_PIXEL,  Component::CUSTOM_SHADER_CB_SLOT);

        const int backCulling = GetUseBackCulling();
        SetUseBackCulling(FALSE);
        SetDrawBlendMode (DX_BLENDMODE_NOBLEND, 0);
        SetUseZBuffer3D  (TRUE);
        SetWriteZBuffer3D(TRUE);

        const VECTOR    cameraPosition = GetCameraPosition();
        const glm::vec3 camera(cameraPosition.x, cameraPosition.y, cameraPosition.z);
        const float     padding     = field->HeightMax() * (1.0f + field->BendAmount()) + field->WindStrength() * 1.25f;
        const float     maxDistance = field->MaxDrawDistance();

        for (const auto& buffer : chunkBuffers_)
        {
            if (buffer.vertexBuffer == -1 || buffer.indexBuffer == -1)
                continue;

            const glm::vec3 boundsMin = buffer.rootMin - glm::vec3(padding);
            const glm::vec3 boundsMax = buffer.rootMax + glm::vec3(padding);
            if (maxDistance > 0.0f && glm::distance(camera, glm::clamp(camera, boundsMin, boundsMax)) > maxDistance)
                continue;
            if (CheckCameraViewClip_Box(VGet(boundsMin.x, boundsMin.y, boundsMin.z),
                                        VGet(boundsMax.x, boundsMax.y, boundsMax.z)) == TRUE)
                continue;

            DrawPrimitiveIndexed3DToShader_UseVertexBuffer(buffer.vertexBuffer, buffer.indexBuffer, DX_PRIMTYPE_TRIANGLELIST);
            ++drawnChunkCount_;
        }

        SetUseBackCulling(backCulling);
        SetUseVertexShader(-1);
        SetUsePixelShader (-1);
    }

    void GrassRenderer::OnDestroy()
    {
        ReleaseBuffers();
        ReleaseConstantBuffer();
    }

    void GrassRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("grassField_", grassField_);
        ImGuiHelper::OnDrawInputField("vsFile_",     vsFile_);
        ImGuiHelper::OnDrawInputField("psFile_",     psFile_);
        ImGui::Text("Chunks: %d / %d drawn", drawnChunkCount_, static_cast<int>(chunkBuffers_.size()));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::GrassRenderer);
#pragma endregion
