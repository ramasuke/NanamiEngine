#pragma once
#include <DxLib.h>
#include <utility>
#include <vector>
#include <../../Libs/glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <../../Libs/glm/gtx/quaternion.hpp>

#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/MV1/MV1File.h"
#include "../../Asset/Hlsl/HlslVsFile.h"
#include "../../Asset/Hlsl/HlslPsFile.h"
#include "../ComponentBase.h"
#include "../../../Core/Coroutine/Task/Task.h"
#include "../../LifeCycleCallback/InitRenderable/IInitRenderable.h"
#include "../../LifeCycleCallback/PreFixedUpdate/IPreFixedUpdate.h"
#include "../../LifeCycleCallback/UpdatedPhysics/IEndPhysics.h"

namespace NanamiEngine::Module::Component
{
    class ModelRenderer final : public ComponentBase,
                                public LifeCycleCallback::IInitRenderable,
                                public LifeCycleCallback::IShadowRenderable,
                                public LifeCycleCallback::IRenderable,
                                public LifeCycleCallback::IPreFixedUpdate,
                                public LifeCycleCallback::IEndPhysics
    {
    public:
        // DxLib(Direct3D 11) は定数バッファスロット b0～b3 を内部で使用しているため、
        // カスタムシェーダー用の定数バッファは b4 に割り当てる。
        // (HLSL 側も register(b4) で受ける必要がある)
        static constexpr int CUSTOM_SHADER_CB_SLOT = 4;
        static constexpr int CUSTOM_SHADER_CB_SIZE = 256;

        int modelDxLibHandle_ = -1;

        // カスタムシェーダー用の定数バッファハンドルを返す(未生成なら生成する)。
        // vsFile_ / psFile_ が有効でない場合は -1。
        [[nodiscard]] int GetOrCreateShaderConstantBufferHandle();

    private:
        void InitRenderer    () override;
        void OnShadowRender  () override;
        void OnRender        () override;
        void OnDestroy       () override;
        void OnPreFixedUpdate() override;
        void OnUpdatedPhysics() override;

        [[nodiscard]] MATRIX GetRenderMatrix() const;
        [[nodiscard]] bool   HasCustomShader () const;
        void RefreshTriangleListInfo();
        void ApplyCustomModelState  ();
        void RestoreDefaultModelState();
        void DrawWithCustomShader   ();

        FIELD(Asset::Mv1File)    mv1File_;
        FIELD(Asset::HlslVsFile) vsFile_;
        FIELD(Asset::HlslPsFile) psFile_;
        bool useFixedInterpolation_ = false;

        int  cbHandle_           = -1;
        bool customStateApplied_ = false;

        // トライアングルリストごとに「剛体用頂点シェーダーで描画できるか」
        // (4/8 ボーンのスキンメッシュは DxLib 標準シェーダーにフォールバックする)
        std::vector<bool> rigidTriangleList_;
        bool              allRigid_ = true;

        // カスタムシェーダー適用前のマテリアルのブレンド設定 (mode, param) の退避
        std::vector<std::pair<int, int>> originalMaterialBlend_;

        glm::vec3 prevWorldPos_   {};
        glm::quat prevWorldRot_   {};
        glm::vec3 currWorldPos_   {};
        glm::quat currWorldRot_   {};
        bool      hasPrevCapture_ = false;
        bool      hasCurrCapture_ = false;

#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IShadowRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
    archive(CEREAL_NVP(mv1File_));
    archive(CEREAL_NVP(useFixedInterpolation_));
    archive(CEREAL_NVP(vsFile_));
    archive(CEREAL_NVP(psFile_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IShadowRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    if (version >= 3) archive(cereal::base_class<LifeCycleCallback::IEndPhysics>(this));
    if (version >= 0) archive(CEREAL_NVP(mv1File_));
    if (version >= 2) archive(CEREAL_NVP(useFixedInterpolation_));
    if (version >= 4) archive(CEREAL_NVP(vsFile_));
    if (version >= 4) archive(CEREAL_NVP(psFile_));
}
#pragma endregion
};
}
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::ModelRenderer, 4)
