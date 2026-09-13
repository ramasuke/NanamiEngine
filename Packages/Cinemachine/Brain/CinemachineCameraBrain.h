#pragma once
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Libs/glm/gtc/quaternion.hpp"
#include "../VirtualCamera/CineMachineVirtualCamera.h"

namespace NanamiEngine::CineMachine
{
    class CinemachineCameraBrain final : public Component::ComponentBase,
                                         public LifeCycleCallback::IAwakable,
                                         public LifeCycleCallback::IStartable,
                                         public LifeCycleCallback::IUpdatable,
                                         public LifeCycleCallback::IDebugRenderable
    {
    public:
        void OnDrawGui() override;
        void ApplyVirtualCameraMatrix() const;
        void ApplyVirtualCameraMatrix(const CineMachineVirtualCamera& virtualCamera) const;
        static CinemachineCameraBrain* Instance() { return cameraBrain_; }
        static void SubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera);
        static void UnSubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera);

        [[nodiscard]] float GetFov()  const { return fov_; }
        [[nodiscard]] float GetNear() const { return cameraNear_; }
        [[nodiscard]] float GetFar()  const { return cameraFar_; }
        
    private:
        void OnAwake      () override;
        void OnStart      () override;
        void OnUpdate     () override;
        void OnDestroy    () override;
        void OnDebugRender() override;
        void OnDebugCameraFovRender() const;
        // カメラ周囲の空き距離から、Near平面の四隅が障害物にめり込まないNearクリップ距離を求める
        [[nodiscard]] float CalculateSafeNear(const glm::vec3& cameraPos) const;

        std::vector<FIELD(CineMachineVirtualCamera)> virtualCameras_;
        FIELD(CineMachineVirtualCamera) currentVirtualCamera_;
        
        float positionLerpSpeed_secs_  = 5.0f;
        float rotationSlerpSpeed_secs_ = 5.0f;
        float fov_                     = 100.0f;
        float cameraNear_              = 0.1f;
        float cameraFar_               = 100.0f;
        // 障害物が近いときに動的に縮めるNearクリップの下限。小さすぎると遠景のZ精度が落ちる
        float minCameraNear_           = 0.5f;
        // コライダーと描画メッシュのズレを吸収するため、空き距離に掛ける安全係数(0～1)
        float nearClipMargin_          = 0.9f;
        // 実際にSetCameraNearFarへ渡したNear(確認用、非シリアライズ)
        float appliedNear_             = 0.1f;
        static CinemachineCameraBrain* cameraBrain_;

        // Shake/Noiseなどのオフセットを含まない、補完だけの姿勢。
        // 揺れた後のTransformを次フレームの補完開始点にすると揺れが蓄積・増幅するため分離して保持する。
        glm::vec3 smoothedPos_ = glm::vec3(0.0f);
        glm::quat smoothedRot_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        bool hasSmoothedPose_ = false;
        const CineMachineVirtualCamera* liveCamera_ = nullptr;

#pragma region Serialization Function
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Module::Component::ComponentBase>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::IUpdatable>(this));
            archive(CEREAL_NVP(currentVirtualCamera_));

            archive(cereal::make_nvp("virtualCameraCount", static_cast<uint32_t>(virtualCameras_.size())));

            for (size_t i = 0; i < virtualCameras_.size(); ++i)
            {
                archive(cereal::make_nvp("virtualCamera_" + std::to_string(i), virtualCameras_[i]));
            }
            archive(CEREAL_NVP(positionLerpSpeed_secs_));
            archive(CEREAL_NVP(rotationSlerpSpeed_secs_));
            archive(CEREAL_NVP(fov_));
            archive(CEREAL_NVP(cameraNear_));
            archive(CEREAL_NVP(cameraFar_));
            archive(CEREAL_NVP(minCameraNear_));
            archive(CEREAL_NVP(nearClipMargin_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Module::Component::ComponentBase>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::IUpdatable>(this));
            archive(CEREAL_NVP(currentVirtualCamera_));
            uint32_t count = 0;
            archive(cereal::make_nvp("virtualCameraCount", count));
            virtualCameras_.resize(count);

            for (size_t i = 0; i < count; ++i)
            {
                archive(cereal::make_nvp("virtualCamera_" + std::to_string(i), virtualCameras_[i]));
            }
            if (version >= 2)
            {
            archive(CEREAL_NVP(positionLerpSpeed_secs_));
            archive(CEREAL_NVP(rotationSlerpSpeed_secs_));
            archive(CEREAL_NVP(fov_));
            archive(CEREAL_NVP(cameraNear_));
            archive(CEREAL_NVP(cameraFar_));
            }
            if (version >= 3)
            {
            archive(CEREAL_NVP(minCameraNear_));
            archive(CEREAL_NVP(nearClipMargin_));
            }
            cameraBrain_ = this;
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::CinemachineCameraBrain, 3)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::CineMachine::CinemachineCameraBrain);
