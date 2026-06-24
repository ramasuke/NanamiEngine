#pragma once
#include "../IVirtualCameraBehaviour.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    // ThirdPersonCameraBehaviour と同じ Virtual Camera 上で併用する想定のカメラシェイク。
    // Shake() / ShakeMainCamera() を呼ぶと、trauma を加算してカメラが揺れる。
    // 揺れは brain がメインカメラを確定させた直後の MainCameraCallback() で上書き適用するため、
    // brain の補間(スムージング)に打ち消されずキレのある揺れになる。
    class ShakeCameraBehaviour final
        : public Component::ComponentBase
        , public LifeCycleCallback::IAwakable
        , public LifeCycleCallback::IUpdatable
        , public IVirtualCameraBehaviour
    {
    public:
        // intensity: 加算する trauma 量(0〜1で正規化)。duration: 減衰しきるまでの秒数。
        void Shake(float intensity, float duration);
        // GUI で設定したデフォルト値で発動する。
        void Shake();

        // どこからでも 1 行で現在のメインカメラを揺らす。
        static void ShakeMainCamera(float intensity, float duration);
        static void ShakeMainCamera();

    private:
        void OnAwake () override;
        void OnDestroy() override;
        void OnUpdate() override;
        // brain がカメラを確定させた直後に呼ばれる。ここで揺れを上書き適用する。
        void MainCameraCallback() override;

        static ShakeCameraBehaviour* instance_;

        // 実行時状態(シリアライズしない)
        float trauma_   = 0.0f;
        float duration_ = 0.4f;

        // 編集可能パラメータ
        glm::vec3 posAmplitude_   = glm::vec3(0.4f, 0.4f, 0.25f); // 位置揺れ幅(カメラローカル, ワールド単位)
        glm::vec3 angleAmplitude_ = glm::vec3(2.0f, 2.0f, 3.0f);  // 回転揺れ幅(yaw, pitch, roll / 度)
        float     frequency_      = 22.0f;                        // 揺れの速さ(ノイズの周波数)
        float     defaultIntensity_ = 0.6f;
        float     defaultDuration_  = 0.4f;
        glm::vec3 seed_ = glm::vec3(13.37f, 71.13f, 42.42f);      // 軸ごとのノイズ位相シード

        FIELD(GameObject::IGameObject) cameraBrain_; // brain GameObject(確定後の最終 Transform を読む)

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
            archive(cereal::base_class<IVirtualCameraBehaviour>(this));
            archive(CEREAL_NVP(posAmplitude_));
            archive(CEREAL_NVP(angleAmplitude_));
            archive(CEREAL_NVP(frequency_));
            archive(CEREAL_NVP(defaultIntensity_));
            archive(CEREAL_NVP(defaultDuration_));
            archive(CEREAL_NVP(seed_));
            archive(CEREAL_NVP(cameraBrain_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
            archive(cereal::base_class<IVirtualCameraBehaviour>(this));
            if (version >= 0) archive(CEREAL_NVP(posAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(angleAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(frequency_));
            if (version >= 0) archive(CEREAL_NVP(defaultIntensity_));
            if (version >= 0) archive(CEREAL_NVP(defaultDuration_));
            if (version >= 0) archive(CEREAL_NVP(seed_));
            if (version >= 0) archive(CEREAL_NVP(cameraBrain_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(CineMachine::Behaviour::ShakeCameraBehaviour, 0)
CEREAL_REGISTER_POLYMORPHIC_RELATION(LifeCycleCallback::IAwakable, CineMachine::Behaviour::ShakeCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(LifeCycleCallback::IUpdatable, CineMachine::Behaviour::ShakeCameraBehaviour);
CEREAL_REGISTER_POLYMORPHIC_RELATION(CineMachine::IVirtualCameraBehaviour, CineMachine::Behaviour::ShakeCameraBehaviour);
