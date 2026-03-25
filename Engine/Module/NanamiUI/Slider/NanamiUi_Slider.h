#pragma once
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../Component/ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"

namespace NanamiEngine::Module::NanamiUi
{
    class Slider final : public Component::ComponentBase,
                         public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        [[nodiscard]] float GetValue() const { return value_; }
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }
        void SetValue(float value);

    private:
        void OnUserInterfaceRender() override;
        void OnDrawGui() override;

        [[serialize(0)]] FIELD(Asset::SpriteFile) gaugeSprite_;
        [[serialize(0)]] glm::vec2 drawPosition_  = glm::vec2(0.0f, 0.0f);
        [[serialize(0)]] glm::vec2 drawSize_ = glm::vec2(100.0f, 20.0f);
        [[serialize(0)]] float value_ = 1.0f;
        [[serialize(0)]] int renderOrder_ = 0;

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(gaugeSprite_));
            archive(CEREAL_NVP(drawPosition_));
            archive(CEREAL_NVP(drawSize_));
            archive(CEREAL_NVP(value_));
            archive(CEREAL_NVP(renderOrder_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(gaugeSprite_));
            if (version >= 0) archive(CEREAL_NVP(drawPosition_));
            if (version >= 0) archive(CEREAL_NVP(drawSize_));
            if (version >= 0) archive(CEREAL_NVP(value_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiUi::Slider, 0);
CEREAL_REGISTER_TYPE(NanamiUi::Slider);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiUi::Slider);
#pragma endregion