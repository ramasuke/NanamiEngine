#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"

namespace GamePlay::Ui
{
    class InjuredMaskUI final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IUpdatable
    {
    public:
        void OnAwake() override;
        void OnUpdate() override;
        void SetActive(bool isActive);

    private:
        [[serialize(0)]] int   minBlendRate_       = 0;
        [[serialize(0)]] int   maxBlendRate_        = 200;
        [[serialize(0)]] float cycleDuration_secs_ = 1.5f;

        bool isActive_ = false;
        FIELD(NanamiUi::BlendImageRenderer) blendRenderer_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(minBlendRate_));
            archive(CEREAL_NVP(maxBlendRate_));
            archive(CEREAL_NVP(cycleDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(minBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(maxBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(cycleDuration_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::InjuredMaskUI, 0)
