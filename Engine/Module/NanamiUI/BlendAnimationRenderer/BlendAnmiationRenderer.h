#pragma once
#include "../../../Core/Object/Field/Field.h"
#include "../../Component/ComponentBase.h"
#include "../../Component/BlendImageRenderer/BlendImageRenderer.h"

namespace NanamiEngine::Module::NanamiUi
{
    class BlendAnimationRenderer final : public Component::ComponentBase,
                                         public LifeCycleCallback::IAwakable,
                                         public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] int GetAddBlendRate_secs() const { return addBlendRate_secs_; }
        void SetAddBlendRate_secs(int value);

    private:
        void OnAwake () override;
        void OnUpdate() override;

        FIELD(BlendImageRenderer) blendImageRenderer_;
        float currentBlendRate_  = 0.0f;
        int   addBlendRate_secs_ = 1;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(addBlendRate_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(addBlendRate_secs_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiUi::BlendAnimationRenderer, 0);
CEREAL_REGISTER_TYPE(NanamiUi::BlendAnimationRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiUi::BlendAnimationRenderer);
#pragma endregion