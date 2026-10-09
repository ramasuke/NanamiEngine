#pragma once
#include <cstdint>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GamePlay::Title
{
    // NOTE: タイトル画面のカメラ。shotsRoot_ の子の VirtualCamera を順に映してループする
    // NOTE: 各ショットはカメラからその先頭の子の VirtualCamera へ Brain の補間で動く
    class TitleCameraDirector final : public Component::ComponentBase,
                                      public LifeCycleCallback::IStartable,
                                      public LifeCycleCallback::IUpdatable
    {
    private:
        struct Shot
        {
            std::weak_ptr<CineMachine::CineMachineVirtualCamera> camera;
            std::weak_ptr<CineMachine::CineMachineVirtualCamera> endCamera;
            float duration_secs = 0.0f;
        };

        void OnStart () override;
        void OnUpdate() override;

        void BeginShot(std::size_t index);
        void ApplyDip(float rate) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) shotsRoot_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) dipMask_;
        [[serialize(0)]] std::vector<float> shotDurations_secs_;
        [[serialize(0)]] float defaultShotDuration_secs_ = 9.0f;
        // NOTE: 暗転の全長 (半分で暗くなり、半分で明ける)
        [[serialize(0)]] float dipDuration_secs_ = 0.8f;
        [[serialize(0)]] int shotPriority_ = 100;

        std::vector<Shot> shots_;
        std::size_t currentIndex_ = 0;
        float elapsed_secs_ = 0.0f;
        bool isStarted_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(shotsRoot_));
            archive(CEREAL_NVP(dipMask_));
            archive(CEREAL_NVP(shotDurations_secs_));
            archive(CEREAL_NVP(defaultShotDuration_secs_));
            archive(CEREAL_NVP(dipDuration_secs_));
            archive(CEREAL_NVP(shotPriority_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(shotsRoot_));
            if (version >= 0) archive(CEREAL_NVP(dipMask_));
            if (version >= 0) archive(CEREAL_NVP(shotDurations_secs_));
            if (version >= 0) archive(CEREAL_NVP(defaultShotDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(dipDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(shotPriority_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Title::TitleCameraDirector, 0);
