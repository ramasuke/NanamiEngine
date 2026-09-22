#pragma once
#include <string>

#include "cereal/types/string.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Hlsl/HlslVsFile.h"
#include "Engine/Module/Asset/Hlsl/HlslPsFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/Shader/IShaderConstantBufferHost.h"
#include "Engine/Module/Component/Shader/IModelMaterialShaderPolicy.h"
#include "Engine/Module/Component/Shader/ShaderConstantBufferSlot.h"

namespace GamePlay::Prop
{
    // 同じ GameObject の ModelRenderer に対して、葉のマテリアルだけ風で揺れるシェーダーを供給する。
    // 揺れの重みと葉カードごとの位相は .mv1 の第2UV(TEXCOORD1)にベイク済み。
    class TreeLeafSway final : public Component::ComponentBase,
                               public Component::IShaderConstantBufferHost,
                               public Component::IModelMaterialShaderPolicy
    {
    public:
        [[nodiscard]] int  GetOrCreateShaderConstantBufferHandle() override;
        [[nodiscard]] bool TryGetMaterialShaderPass(const std::string& materialName, Component::MaterialShaderPass& outPass) override;
        [[nodiscard]] bool ShouldDrawShadow        (const std::string& materialName) override;

    private:
        // Tree_VS.hlsl / Tree_PS.hlsl の TreeWindBuffer と同じ並び
        struct TreeWindCB
        {
            float wind[4];
            float windDirection[4];
            float lightDirection[4];
            float lightColor[4];
        };

        void OnDestroy() override;
        void WriteConstantBuffer(int cbHandle) const;
        [[nodiscard]] bool IsLeafMaterial(const std::string& materialName) const;

        FIELD(Asset::HlslVsFile) vsFile_;
        FIELD(Asset::HlslPsFile) psFile_;

        std::string leafMaterialSuffix_ = "_Leaf"; // .mv1 の材質名は <樹種>_Leaf / <樹種>_Bark
        float       swayAmplitude_      = 8.0f;    // ワールド単位(1ユニット=1cm)での揺れ幅
        float       ambient_            = 0.35f;

        int cbHandle_ = -1;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(vsFile_));
            archive(CEREAL_NVP(psFile_));
            archive(CEREAL_NVP(leafMaterialSuffix_));
            archive(CEREAL_NVP(swayAmplitude_));
            archive(CEREAL_NVP(ambient_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(vsFile_));
            if (version >= 0) archive(CEREAL_NVP(psFile_));
            if (version >= 0) archive(CEREAL_NVP(leafMaterialSuffix_));
            if (version >= 0) archive(CEREAL_NVP(swayAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(ambient_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::TreeLeafSway, 0)
