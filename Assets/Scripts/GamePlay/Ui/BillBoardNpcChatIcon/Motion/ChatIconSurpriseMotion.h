#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/NanamiUI/BillBoard3D/BillboardAnimation3D.h"
#include "ChatIconPopMotion.h"

namespace GamePlay::Ui
{
    // NOTE: ビックリマーク。ゆっくり上下し、周期の先頭で枠を光が走り、周期の最後にコトッと傾く
    // NOTE: 光は子オブジェクトなので位置・スケールは親から引き継ぐ。角度と透明度だけ下地に合わせる
    class ChatIconSurpriseMotion final : public Component::ComponentBase,
                                         public LifeCycleCallback::IUpdatable
    {
    private:
        void OnUpdate() override;

        ChatIconPopMotion pop_;

        [[serialize(0)]] FIELD(NanamiUi::BillboardAnimation3D) rimGlow_;
        [[serialize(0)]] float popDuration_secs_   = 0.25f;
        [[serialize(0)]] float floatAmplitude_     = 0.2f;
        [[serialize(0)]] float floatSpeed_         = 2.0f;
        [[serialize(0)]] float cycle_secs_         = 3.0f;
        [[serialize(0)]] float sweepDuration_secs_ = 0.6f;
        [[serialize(0)]] float tiltDuration_secs_  = 0.5f;
        [[serialize(0)]] float tiltAngle_          = 0.2f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(rimGlow_));
            archive(CEREAL_NVP(popDuration_secs_));
            archive(CEREAL_NVP(floatAmplitude_));
            archive(CEREAL_NVP(floatSpeed_));
            archive(CEREAL_NVP(cycle_secs_));
            archive(CEREAL_NVP(sweepDuration_secs_));
            archive(CEREAL_NVP(tiltDuration_secs_));
            archive(CEREAL_NVP(tiltAngle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rimGlow_));
            if (version >= 0) archive(CEREAL_NVP(popDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(floatAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(floatSpeed_));
            if (version >= 0) archive(CEREAL_NVP(cycle_secs_));
            if (version >= 0) archive(CEREAL_NVP(sweepDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(tiltDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(tiltAngle_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ChatIconSurpriseMotion, 0);
