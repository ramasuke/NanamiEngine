#pragma once
#include "../IVirtualCameraBehaviour.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    class ShakeCameraBehaviour final : public Component::ComponentBase,
                                       public LifeCycleCallback::IAwakable,
                                       public LifeCycleCallback::IUpdatable,
                                       public IVirtualCameraBehaviour
    {
    public:
        void Shake(float intensity, float duration);
        void Shake();

        static void ShakeMainCamera(float intensity, float duration);
        static void ShakeMainCamera();

    private:
        void OnAwake () override;
        void OnDestroy() override;
        void OnUpdate() override;
        void MainCameraCallback() override;

        static ShakeCameraBehaviour* instance_;

        float trauma_   = 0.0f;
        float duration_ = 0.4f;

        glm::vec3 posAmplitude_   = glm::vec3(0.4f, 0.4f, 0.25f); // 位置揺れ幅
        glm::vec3 angleAmplitude_ = glm::vec3(2.0f, 2.0f, 3.0f);  // 回転揺れ幅
        float     frequency_      = 22.0f;                        // 揺れの速さ
        float     defaultIntensity_ = 0.6f;
        float     defaultDuration_  = 0.4f;
        glm::vec3 seed_ = glm::vec3(13.37f, 71.13f, 42.42f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version <= 0) archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            if (version <= 0) archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
            archive(cereal::base_class<IVirtualCameraBehaviour>(this));
            archive(CEREAL_NVP(posAmplitude_));
            archive(CEREAL_NVP(angleAmplitude_));
            archive(CEREAL_NVP(frequency_));
            archive(CEREAL_NVP(defaultIntensity_));
            archive(CEREAL_NVP(defaultDuration_));
            archive(CEREAL_NVP(seed_));
            FIELD(GameObject::IGameObject) cameraBrain_;
            if (version <= 0) archive(CEREAL_NVP(cameraBrain_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version <= 0) archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            if (version <= 0) archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
            archive(cereal::base_class<IVirtualCameraBehaviour>(this));
            if (version >= 0) archive(CEREAL_NVP(posAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(angleAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(frequency_));
            if (version >= 0) archive(CEREAL_NVP(defaultIntensity_));
            if (version >= 0) archive(CEREAL_NVP(defaultDuration_));
            if (version >= 0) archive(CEREAL_NVP(seed_));
            FIELD(GameObject::IGameObject) cameraBrain_;
            if (version <= 0) archive(CEREAL_NVP(cameraBrain_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(CineMachine::Behaviour::ShakeCameraBehaviour, 1)
CEREAL_REGISTER_POLYMORPHIC_RELATION(CineMachine::IVirtualCameraBehaviour, CineMachine::Behaviour::ShakeCameraBehaviour);
