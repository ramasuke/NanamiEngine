#pragma once
#include <cstdint>
#include <vector>

#include "vec3.hpp"
#include "../../../../../Assets/Data/GrassField/Data_GrassField.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Hlsl/HlslVsFile.h"
#include "Engine/Module/Asset/Hlsl/HlslPsFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/Shader/IShaderConstantBufferHost.h"
#include "Engine/Module/Component/Shader/ShaderConstantBufferSlot.h"

namespace GamePlay::Prop
{
    // GrassField の草をチャンクごとの頂点バッファにまとめ、風の揺れは頂点シェーダーで付けて描画する
    class GrassRenderer final : public Component::ComponentBase,
                                public LifeCycleCallback::IRenderable,
                                public Component::IShaderConstantBufferHost
    {
    public:
        ~GrassRenderer() override;
        [[nodiscard]] int GetOrCreateShaderConstantBufferHandle() override;

    private:
        // Grass_VS.hlsl / Grass_PS.hlsl の GrassBuffer と同じ並び
        struct GrassCB
        {
            float wind[4];           
            float windDirection[4];  // xz=向き
            float baseColor[4];
            float tipColor[4];
            float lightDirection[4];
            float lightColor[4];     // w=ambient
        };

        struct ChunkBuffer
        {
            int       vertexBuffer = -1;
            int       indexBuffer  = -1;
            glm::vec3 rootMin{0.0f};
            glm::vec3 rootMax{0.0f};
        };

        void OnRender () override;
        void OnDestroy() override;

        [[nodiscard]] bool HasCustomShader() const;
        void RebuildBuffers(const Asset::GrassField& field);
        void ReleaseBuffers();
        void ReleaseConstantBuffer();
        void WriteConstantBuffer(const Asset::GrassField& field, int cbHandle) const;

        FIELD(Asset::GrassField) grassField_;
        FIELD(Asset::HlslVsFile) vsFile_;
        FIELD(Asset::HlslPsFile) psFile_;

        int                      cbHandle_       = -1;
        std::vector<ChunkBuffer> chunkBuffers_;
        const Asset::GrassField* builtField_     = nullptr;
        std::uint64_t            builtRevision_  = 0;
        int                      drawnChunkCount_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
            archive(CEREAL_NVP(grassField_));
            archive(CEREAL_NVP(vsFile_));
            archive(CEREAL_NVP(psFile_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(grassField_));
            if (version >= 0) archive(CEREAL_NVP(vsFile_));
            if (version >= 0) archive(CEREAL_NVP(psFile_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::GrassRenderer, 0);
