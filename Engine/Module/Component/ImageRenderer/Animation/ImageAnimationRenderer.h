#pragma once
#include "../../ComponentBase.h"
#include "../../../../Core/Object/Field/Field.h"
#include "../../../Asset/Sprite/AnimationFile/SpriteAnimationFile.h"
#include "../../../LifeCycleCallback/InitRenderable/IInitRenderable.h"
#include "../../../LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable.h"

namespace NanamiEngine::Module::NanamiUi
{
    class ImageAnimationRenderer final : public Component::ComponentBase,
                                         public LifeCycleCallback::IInitRenderable,
                                         public LifeCycleCallback::IUserInterfaceRenderable
    {
    private:
        void InitRenderer() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override;

        [[serialize(0)]] FIELD(Asset::SpriteAnimationFile) animationFile_;
        [[serialize(0)]] bool isLoop_ = true;
        [[serialize(0)]] int currentFrame_ = 0;
        [[serialize(0)]] float animationDuration_secs_ = 0.1f;
        [[serialize(0)]] float timer_ = 0.0f;
        [[serialize(0)]] int renderPriority_ = 0;

#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(CEREAL_NVP(animationFile_));
    archive(CEREAL_NVP(renderPriority_));
    archive(CEREAL_NVP(animationDuration_secs_));
    archive(CEREAL_NVP(isLoop_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    if (version >= 0) archive(CEREAL_NVP(animationFile_));
    if (version >= 0) archive(CEREAL_NVP(renderPriority_));
    if (version >= 0) archive(CEREAL_NVP(animationDuration_secs_));
    if (version >= 0) archive(CEREAL_NVP(isLoop_));
}
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::ImageAnimationRenderer, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::NanamiUi::ImageAnimationRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::NanamiUi::ImageAnimationRenderer);
#pragma endregion
