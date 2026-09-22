#pragma once
#include <DxLib.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <../../Libs/glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <../../Libs/glm/gtx/quaternion.hpp>

#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/MV1/MV1File.h"
#include "../ComponentBase.h"
#include "../../../Core/Coroutine/Task/Task.h"
#include "../../LifeCycleCallback/InitRenderable/IInitRenderable.h"
#include "../../LifeCycleCallback/PreFixedUpdate/IPreFixedUpdate.h"
#include "../../LifeCycleCallback/UpdatedPhysics/IEndPhysics.h"
#include "../Shader/IModelMaterialShaderPolicy.h"
#include "../Shader/ShaderConstantBufferSlot.h"

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
        int modelDxLibHandle_ = -1;

        void SetMv1File(const std::shared_ptr<Asset::Mv1File>& mv1File);
        /** @brief 描画位置だけをワールド空間でずらす(Transform・物理・同期には影響しない) */
        void SetRenderOffset(const glm::vec3& offset) { renderOffset_ = offset; }
        /** @brief 実際に描画しているワールド座標。useFixedInterpolation_なら物理ステップ間を補間した値(renderOffset_は含まない) */
        [[nodiscard]] glm::vec3 RenderWorldPos() const;

    private:
        using PolicyList = std::vector<std::weak_ptr<IModelMaterialShaderPolicy>>;

        void InitRenderer    () override;
        void ReloadModel     ();
        void OnShadowRender  () override;
        void OnRender        () override;
        void OnDestroy       () override;
        void OnPreFixedUpdate() override;
        void OnUpdatedPhysics() override;

        [[nodiscard]] bool   IsInterpolating() const;
        [[nodiscard]] MATRIX GetRenderMatrix() const;
        void RefreshTriangleListInfo();
        void ResolveMaterialPasses      (const PolicyList& policies);
        void RestoreDefaultMaterialState();
        void DrawWithMaterialPolicies   ();
        [[nodiscard]] bool ShouldDrawShadowForMaterial(const PolicyList& policies, const std::string& materialName) const;

        FIELD(Asset::Mv1File) mv1File_;
        bool useFixedInterpolation_ = false;

        std::vector<bool> rigidTriangleList_;
        bool              allRigid_ = true;

        // モデル差し替え時にだけ組み直す静的な対応表
        std::vector<std::string>         materialNames_;
        std::vector<std::pair<int, int>> originalMaterialBlend_;
        std::vector<int>                 triangleListMaterialIndex_;
        std::vector<int>                 meshMaterialIndex_;
        std::vector<int>                 meshOriginalCulling_;
        
        std::vector<MaterialShaderPass> materialPasses_;
        std::vector<bool>               materialPassActive_;
        bool                            materialStateApplied_ = false;

        glm::vec3 prevWorldPos_   {};
        glm::quat prevWorldRot_   {};
        glm::vec3 currWorldPos_   {};
        glm::quat currWorldRot_   {};
        bool      hasPrevCapture_ = false;
        bool      hasCurrCapture_ = false;

        glm::vec3 renderOffset_   {};

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
}
#pragma endregion
};
}
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::ModelRenderer, 5)
