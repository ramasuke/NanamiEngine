#pragma once
#include <DxLib.h>

#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/MV1/MV1File.h"
#include "../ComponentBase.h"
#include "../../../Core/Coroutine/Task/Task.h"
#include "../../LifeCycleCallback/InitRenderable/IInitRenderable.h"

namespace NanamiEngine::Module::Component
{
    class ModelRenderer final : public ComponentBase,
                                public LifeCycleCallback::IInitRenderable,
                                public LifeCycleCallback::IShadowRenderable,
                                public LifeCycleCallback::IRenderable
    {
    public:
        int modelDxLibHandle_ = -1;
        
    private:
        void InitRenderer  () override;
        void OnShadowRender() override;
        void OnRender      () override;
        void OnDestroy     () override;
        
        FIELD(Asset::Mv1File) mv1File_;
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IShadowRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    archive(CEREAL_NVP(mv1File_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IShadowRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    if (version >= 0) archive(CEREAL_NVP(mv1File_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ModelRenderer, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::ModelRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::ModelRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IRenderable, NanamiEngine::Module::Component::ModelRenderer);
#pragma endregion
