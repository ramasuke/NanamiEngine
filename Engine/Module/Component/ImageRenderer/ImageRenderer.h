#pragma once
#include "../ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../LifeCycleCallback/InitRenderable/IInitRenderable.h"
#include "../../LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable.h"

namespace NanamiEngine::Module::Component
{
    class ImageRenderer final : public ComponentBase,
                                public LifeCycleCallback::IInitRenderable,
                                public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        [[nodiscard]] const std::weak_ptr<Asset::SpriteFile>& GetSprite() const { return spriteFile_.get(); }
        void SetSprite(const std::weak_ptr<Asset::SpriteFile>& sprite);
        
    private:
        void InitRenderer         ()       override;
        void OnUserInterfaceRender()       override;
        [[nodiscard]] int GetRenderOrder() const override { return renderPriority_; }
        
        [[serialize(0)]] FIELD(Asset::SpriteFile) spriteFile_;
        [[serialize(2)]] int renderPriority_ = 0;
#pragma region Serialization Function
public:
void OnDrawGui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
    archive(CEREAL_NVP(spriteFile_));
    archive(CEREAL_NVP(renderPriority_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
    archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
    if (version >= 0) archive(CEREAL_NVP(spriteFile_));
    if (version >= 2) archive(CEREAL_NVP(renderPriority_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ImageRenderer, 2);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::ImageRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::ImageRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(LifeCycleCallback::IUserInterfaceRenderable, NanamiEngine::Module::Component::ImageRenderer);
#pragma endregion
