#pragma once
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Engine/Module/LifeCycleCallback/LateUpdate/LateUpdate.h"
#include "../../../Libs/glm/gtc/quaternion.hpp"
#include "../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "../VirtualCamera/CineMachineVirtualCamera.h"

namespace NanamiEngine::CineMachine
{
    class CinemachineCameraBrain final : public Component::ComponentBase,
                                         public LifeCycleCallback::IAwakable,
                                         public LifeCycleCallback::IStartable,
                                         public LifeCycleCallback::ILateUpdatable,
                                         public LifeCycleCallback::IDebugRenderable
    {
    public:
        void OnDrawGui() override;
        void ApplyVirtualCameraMatrix() const;
        void ApplyVirtualCameraMatrix(const CineMachineVirtualCamera& virtualCamera) const;
        /** @brief 補間を挟まずvirtualCameraの位置・回転・FOVへ即座に合わせる。以降の補間もここから始まる */
        void SnapToVirtualCamera(const CineMachineVirtualCamera& virtualCamera);
        static CinemachineCameraBrain* Instance() { return cameraBrain_; }
        static void SubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera);
        static void UnSubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera);

        // 実際にカメラへ適用中のFOV(VirtualCameraによる上書きと補間を反映した値)
        [[nodiscard]] float GetFov()  const { return appliedFov_; }
        // VirtualCameraがFOVを上書きしないときに使う既定FOV
        [[nodiscard]] float DefaultFov() const { return fov_; }
        [[nodiscard]] float GetNear() const { return cameraNear_; }
        [[nodiscard]] float GetFar()  const { return cameraFar_; }
        
    private:
        void OnAwake      () override;
        void OnStart      () override;
        // 全OnUpdateの後に、VirtualCameraのBehaviourを決まった順に回してからカメラを確定させる
        void OnLateUpdate () override;
        void OnDestroy    () override;
        void OnDebugRender() override;
        void OnDebugCameraFovRender() const;
        // Near平面の中心と四隅へ飛ばしたレイから、Near平面が障害物にめり込まないNearクリップ距離を求める
        [[nodiscard]] float CalculateSafeNear(const glm::vec3& cameraPos, const glm::quat& cameraRot) const;

        std::vector<FIELD(CineMachineVirtualCamera)> virtualCameras_;
        FIELD(CineMachineVirtualCamera) currentVirtualCamera_;
        
        float positionLerpSpeed_secs_  = 5.0f;
        float rotationSlerpSpeed_secs_ = 5.0f;
        float fovLerpSpeed_secs_       = 5.0f;
        // VirtualCameraがFOVを上書きしないときに使う既定FOV
        float fov_                     = 100.0f;
        float cameraNear_              = 0.1f;
        float cameraFar_               = 100.0f;
        // 障害物が近いときに動的に縮めるNearクリップの下限。小さすぎると遠景のZ精度が落ちる
        float minCameraNear_           = 0.5f;
        // コライダーと描画メッシュのズレを吸収するため、空き距離に掛ける安全係数(0～1)
        float nearClipMargin_          = 0.9f;
        // Nearクリップを縮める判定に使うレイヤー
        Module::Physics::LayerMask nearClipLayerMask_ = Module::Physics::ToMask(Module::Physics::Layer::Default);
        // 実際にSetCameraNearFarへ渡したNear(確認用、非シリアライズ)
        float appliedNear_             = 0.1f;
        // 実際にSetupCamera_Perspectiveへ渡したFOV(確認用、非シリアライズ)
        float appliedFov_              = 100.0f;
        static CinemachineCameraBrain* cameraBrain_;

        // Shake/Noiseなどのオフセットを含まない、補完だけの姿勢。
        // 揺れた後のTransformを次フレームの補完開始点にすると揺れが蓄積・増幅するため分離して保持する。
        glm::vec3 smoothedPos_ = glm::vec3(0.0f);
        glm::quat smoothedRot_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        float smoothedFov_ = 100.0f;
        bool hasSmoothedPose_ = false;
        const CineMachineVirtualCamera* liveCamera_ = nullptr;

#pragma region Serialization Function
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Module::Component::ComponentBase>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::IAwakable>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::ILateUpdatable>(this));
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
            archive(CEREAL_NVP(fovLerpSpeed_secs_));
            archive(CEREAL_NVP(nearClipLayerMask_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Module::Component::ComponentBase>(this));
            archive(cereal::base_class<Module::LifeCycleCallback::IAwakable>(this));
            if (version <= 4)
                Module::LifeCycleCallback::DiscardUpdatableBase(archive);
            else
                archive(cereal::base_class<Module::LifeCycleCallback::ILateUpdatable>(this));
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
            if (version >= 4)
            {
            archive(CEREAL_NVP(fovLerpSpeed_secs_));
            }
            if (version >= 6)
            {
            archive(CEREAL_NVP(nearClipLayerMask_));
            }
            else
            {
            // NOTE: version 5 までの固定値 (レイヤー 0～2)
            nearClipLayerMask_ = 0b111;
            }
            cameraBrain_ = this;
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::CinemachineCameraBrain, 6)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::ILateUpdatable, NanamiEngine::CineMachine::CinemachineCameraBrain);
