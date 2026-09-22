#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Hlsl/HlslVsFile.h"
#include "Engine/Module/Asset/Hlsl/HlslPsFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/Shader/IShaderConstantBufferHost.h"
#include "Engine/Module/Component/Shader/IModelMaterialShaderPolicy.h"
#include "Engine/Module/Component/Shader/ShaderConstantBufferSlot.h"

namespace GamePlay::Prop
{
    class ProximityReveal final : public Component::ComponentBase,
                                  public Component::IShaderConstantBufferHost,
                                  public Component::IModelMaterialShaderPolicy
    {
    public:
        [[nodiscard]] int  GetOrCreateShaderConstantBufferHandle() override;
        [[nodiscard]] bool TryGetMaterialShaderPass(const std::string& materialName, Component::MaterialShaderPass& outPass) override;
        [[nodiscard]] bool ShouldDrawShadow        (const std::string& materialName) override;

    private:
        struct ProximityCB
        {
            float playerPos[3];
            float revealRadius;
            float transitionWidth;
            float pad[3];
        };

        void OnDestroy() override;
        void WriteConstantBuffer(int cbHandle) const;

        float revealRadius_    = 5.0f;
        float transitionWidth_ = 3.0f;

        FIELD(Asset::HlslVsFile) vsFile_;
        FIELD(Asset::HlslPsFile) psFile_;

        int cbHandle_ = -1;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(revealRadius_));
            archive(CEREAL_NVP(transitionWidth_));
            archive(CEREAL_NVP(vsFile_));
            archive(CEREAL_NVP(psFile_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(revealRadius_));
            if (version >= 0) archive(CEREAL_NVP(transitionWidth_));
            if (version >= 1) archive(CEREAL_NVP(vsFile_));
            if (version >= 1) archive(CEREAL_NVP(psFile_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ProximityReveal, 1)
