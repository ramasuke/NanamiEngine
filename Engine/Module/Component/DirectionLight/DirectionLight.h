#pragma once
#include "../ComponentBase.h"
#include "../../LifeCycleCallback/InitRenderable/IInitRenderable.h"

namespace NanamiEngine::Module::Component
{
    class DirectionLight final : public ComponentBase,
                                 public LifeCycleCallback::IInitRenderable,
                                 public LifeCycleCallback::IRenderable
    {
    private:
        void InitRenderer() override;
        void OnRender    () override;
    
#pragma region Serialization Function
public:
void OnDrawGui() {
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 1) archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::DirectionLight, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::DirectionLight);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::DirectionLight);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IRenderable, NanamiEngine::Module::Component::DirectionLight);
#pragma endregion
