#pragma once
#include <cstdint>
#include <vector>

#include "vec3.hpp"
#include "gtc/quaternion.hpp"
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
    /**
     * @brief タイトル画面のカメラ。shotsRoot_ の子の VirtualCamera を上から順に映し、最後まで行ったら最初に戻る。
     *
     * 各カメラは子 (先頭) の位置と向きへ、ショットの長さをかけてゆっくり動く (序章の OpeningShots と同じ置き方)。
     * ショットの切り替えは dipMask_ (全面の黒) を一瞬下ろして繋ぐ。dipDuration_secs_ が 0 ならそのまま切る
     */
    class TitleCameraDirector final : public Component::ComponentBase,
                                      public LifeCycleCallback::IStartable,
                                      public LifeCycleCallback::IUpdatable
    {
    private:
        struct Shot
        {
            std::weak_ptr<CineMachine::CineMachineVirtualCamera> camera;
            glm::vec3 fromPos = glm::vec3(0.0f);
            glm::quat fromRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 toPos   = glm::vec3(0.0f);
            glm::quat toRot   = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            float duration_secs = 0.0f;
        };

        void OnStart () override;
        void OnUpdate() override;

        void BeginShot(std::size_t index);
        void ApplyShotPose(const Shot& shot, float rate) const;
        void ApplyDip(float rate) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) shotsRoot_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) dipMask_;
        [[serialize(0)]] std::vector<float> shotDurations_secs_;
        [[serialize(0)]] float defaultShotDuration_secs_ = 9.0f;
        // 暗転の全長 (半分で暗くなり、半分で明ける)
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
