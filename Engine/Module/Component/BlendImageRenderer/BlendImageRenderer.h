#pragma once
#include "../ComponentBase.h"
#include "../../../../Libs/LibCore/DxLib/BlendMode.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"

namespace NanamiEngine::Module::NanamiUi
{
    class BlendImageRenderer final : public Component::ComponentBase,
                                     public LifeCycleCallback::IInitRenderable,
                                     public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetBlendRate(int blendRate);
        [[nodiscard]] int GetBlendRate() const { return blendRate_; }

    private:
        void InitRenderer         () override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[serialize(0)]] Dxlib::BlendMode blendMode_ = Dxlib::BlendMode::Alpha;
        [[serialize(0)]] int blendRate_; 
        [[serialize(0)]] FIELD(Asset::SpriteFile) spriteFile_;
        [[serialize(0)]] int renderOrder_ = 0;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(blendMode_));
            archive(CEREAL_NVP(blendRate_));
            archive(CEREAL_NVP(spriteFile_));
            archive(CEREAL_NVP(renderOrder_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(blendMode_));
            if (version >= 0) archive(CEREAL_NVP(blendRate_));
            if (version >= 0) archive(CEREAL_NVP(spriteFile_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::BlendImageRenderer, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::NanamiUi::BlendImageRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiEngine::Module::NanamiUi::BlendImageRenderer);
#pragma endregion