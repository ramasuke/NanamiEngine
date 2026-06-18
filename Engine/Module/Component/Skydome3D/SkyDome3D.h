#pragma once
#include "../ComponentBase.h"
#include "../../../../Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/MV1/MV1File.h"

namespace NanamiEngine::Module::Component
{
    class SkyDome3D final : public ComponentBase,
                            public LifeCycleCallback::IInitRenderable,
                            public LifeCycleCallback::IRenderable,
                            public LifeCycleCallback::IDebugRenderable,
                            public LifeCycleCallback::IUpdatable
    {
    private:
        void InitRenderer () override;
        void OnUpdate     () override;
        void OnRender     () override;
        void OnDebugRender() override;
        void OnDestroy() override;

        FIELD(Asset::Mv1File) skyDomeModel_;
        int skyDomeModelDxLibHandle_ = -1;
        FIELD(CineMachine::CinemachineCameraBrain) mainCamera_;
        
    
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IDebugRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    archive(CEREAL_NVP(skyDomeModel_));
    if (version <= 2) archive(CEREAL_NVP(skyDomeModelDxLibHandle_));
    archive(CEREAL_NVP(mainCamera_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IDebugRenderable>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    if (version >= 0) archive(CEREAL_NVP(skyDomeModel_));
    if (version <= 2) archive(CEREAL_NVP(skyDomeModelDxLibHandle_));
    if (version >= 2) archive(CEREAL_NVP(mainCamera_));
}
#pragma endregion
};
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::SkyDome3D, 3)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IInitRenderable, NanamiEngine::Module::Component::SkyDome3D);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IRenderable, NanamiEngine::Module::Component::SkyDome3D);
