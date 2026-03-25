#pragma once
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Engine/Core/Object/Field/Field.h"
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
        static void SubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera);
        static void UnSubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera);
        
    private:
        void OnAwake      () override;
        void OnStart      () override;
        void OnUpdate     () override;
        void OnDebugRender() override;
        void OnDebugCameraFovRender() const;

        std::vector<FIELD(CineMachineVirtualCamera)> virtualCameras_;
        FIELD(CineMachineVirtualCamera) currentVirtualCamera_;
        
        float positionLerpSpeed_secs_  = 5.0f;
        float rotationSlerpSpeed_secs_ = 5.0f;
        float fov_                     = 100.0f;
        float cameraNear_              = 0.1f;
        float cameraFar_               = 100.0f;
        static CinemachineCameraBrain* cameraBrain_;

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
            cameraBrain_ = this;
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::CineMachine::CinemachineCameraBrain, 2);
CEREAL_REGISTER_TYPE(NanamiEngine::CineMachine::CinemachineCameraBrain);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase,NanamiEngine::CineMachine::CinemachineCameraBrain);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::CineMachine::CinemachineCameraBrain);
#pragma endregion
