#pragma once
#include "Engine/Module/Component/ComponentBase.h"

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
        float     amplitude_     = 0.0f;
        float     duration_secs_ = 0.0f;
        float     elapsed_secs_  = 0.0f;
        bool      isPlaying_     = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::PlayerHitShakeReceiver, 0)
