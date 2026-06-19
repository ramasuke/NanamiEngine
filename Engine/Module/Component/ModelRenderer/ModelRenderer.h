#pragma once
#include <DxLib.h>
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
        int modelDxLibHandle_ = -1;
        int cbHandle_         = -1;

    private:
        void InitRenderer    () override;
        void OnShadowRender  () override;
        void OnRender        () override;
        void OnDestroy       () override;
        void OnPreFixedUpdate() override;
        void OnUpdatedPhysics() override;

        [[nodiscard]] MATRIX GetRenderMatrix() const;
        [[nodiscard]] bool   HasCustomShader () const;
        void ApplyCustomShader  ();
        void RestoreCustomShader();

        FIELD(Asset::Mv1File)    mv1File_;
        FIELD(Asset::HlslVsFile) vsFile_;
        FIELD(Asset::HlslPsFile) psFile_;
        bool useFixedInterpolation_ = false;

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
