#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace NanamiEngine::Module::Component
{
    class ModelRenderer;
}

namespace GamePlay::PlayerAvatar
{
    /**
     * @brief プレイヤーの攻撃を受けたときに、同じGameObjectのModelRendererの描画位置だけを減衰振動させる
     */
    class PlayerHitShakeReceiver final : public Component::ComponentBase,
                                         public LifeCycleCallback::IUpdatable
    {
    public:
        void Play(const glm::vec3& direction, float amplitude, float duration_secs);

    private:
        void OnUpdate() override;

        std::weak_ptr<Component::ModelRenderer> modelRenderer_;
        glm::vec3 direction_     = {};
        float     duration_secs_ = 0.0f;
        // 振幅の減衰 amplitude * (1 - t)^2
        LibCore::Tween::TweenPlayer<float> envelope_;

        [[serialize(1)]] float shakeFrequency_hz_ = 18.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(shakeFrequency_hz_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(shakeFrequency_hz_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::PlayerHitShakeReceiver, 1);
