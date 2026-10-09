#pragma once
#include "vec2.hpp"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/InitRenderable/IInitRenderable.h"

namespace GamePlay::Weather
{
    // NOTE: シーンに 1 つ置く風の供給元。振幅は使う側が持つ
    class WindZone final : public Component::ComponentBase,
                           public LifeCycleCallback::IInitRenderable
    {
    public:
        // NOTE: シーンに未配置でも使う側が壊れないよう、インスタンスが無ければ既定値を返す
        [[nodiscard]] static glm::vec2 GetDirection ();
        [[nodiscard]] static float     GetStrength01();
        [[nodiscard]] static float     GetSpeed     ();
        [[nodiscard]] static float     GetFrequency ();

    private:
        // NOTE: エディタでは OnAwake が呼ばれないため、両方で走る InitRenderer で登録する
        void InitRenderer() override;
        void OnDestroy   () override;

        static WindZone* instance_;

        float windDirectionDeg_ = 30.0f;
        float strength01_       = 1.0f;
        float speed_            = 1.6f;
        float frequency_        = 0.0015f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(windDirectionDeg_));
            archive(CEREAL_NVP(strength01_));
            archive(CEREAL_NVP(speed_));
            archive(CEREAL_NVP(frequency_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(windDirectionDeg_));
            if (version >= 0) archive(CEREAL_NVP(strength01_));
            if (version >= 0) archive(CEREAL_NVP(speed_));
            if (version >= 0) archive(CEREAL_NVP(frequency_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Weather::WindZone, 0);
