#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Ui
{
    class DealDamageTextBillBoard final : public Component::ComponentBase,
                                          public LifeCycleCallback::IAwakable,
                                          public LifeCycleCallback::IUpdatable
    {
    public:
        void Play(int value);

    private:
        void OnAwake () override;
        void OnUpdate() override;

        float riseTime_   = 0.5f;
        float fallTime_   = 0.3f;
        float riseAmount_ = 1.0f;
        float fallAmount_ = 0.8f;

        bool      isPlaying_   = false;
        float     elapsedTime_ = 0.0f;
        glm::vec3 startPos_    = {};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(riseTime_));
            archive(CEREAL_NVP(fallTime_));
            archive(CEREAL_NVP(riseAmount_));
            archive(CEREAL_NVP(fallAmount_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(riseTime_));
            if (version >= 0) archive(CEREAL_NVP(fallTime_));
            if (version >= 0) archive(CEREAL_NVP(riseAmount_));
            if (version >= 0) archive(CEREAL_NVP(fallAmount_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::DealDamageTextBillBoard, 0)
