#pragma once
#include "../../../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"

namespace GamePlay::Ui
{
    /**
     * @brief 力尽きたプレイヤーを見下ろすカメラ。GameOverDeathCamera.prefab の根に付ける。
     *
     * 仮想カメラは Brain があるシーンにしか登録できないので、常駐の GameOverScene には置かず、
     * ゲームオーバーのたびにメインシーンへ生成する（メインシーンと一緒に消える）。
     * 今映っている位置から始めて、背中側へ回り込みながら上へ引いていく
     */
    class GameOverDeathCamera final : public Component::ComponentBase,
                                      public LifeCycleCallback::IUpdatable
    {
    public:
        void Begin(const std::shared_ptr<GameObject::IGameObject>& target);

    private:
        void OnUpdate() override;
        void ApplyShot(float rate) const;

        [[serialize(0)]] int priority_ = 500;
        [[serialize(0)]] float shotSecs_ = 7.0f;
        [[serialize(0)]] float orbitDeg_ = 40.0f;
        [[serialize(0)]] float endPitchDeg_ = 55.0f;
        [[serialize(0)]] float endDistance_ = 46.0f;
        [[serialize(0)]] glm::vec3 endLookAtOffset_ = glm::vec3(0.0f, 1.5f, 0.0f);

        std::weak_ptr<GameObject::IGameObject> target_;
        std::weak_ptr<CineMachine::Behaviour::VirtualCameraFollowBehaviour> follow_;
        std::weak_ptr<CineMachine::Behaviour::VirtualCameraLookAtBehaviour> lookAt_;
        float startYaw_rad_ = 0.0f;
        float startPitch_rad_ = 0.0f;
        float startDistance_ = 0.0f;
        glm::vec3 startLookAtOffset_ = glm::vec3(0.0f);
        float elapsedSecs_ = 0.0f;
        bool isPlaying_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(priority_));
            archive(CEREAL_NVP(shotSecs_));
            archive(CEREAL_NVP(orbitDeg_));
            archive(CEREAL_NVP(endPitchDeg_));
            archive(CEREAL_NVP(endDistance_));
            archive(CEREAL_NVP(endLookAtOffset_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(priority_));
            if (version >= 0) archive(CEREAL_NVP(shotSecs_));
            if (version >= 0) archive(CEREAL_NVP(orbitDeg_));
            if (version >= 0) archive(CEREAL_NVP(endPitchDeg_));
            if (version >= 0) archive(CEREAL_NVP(endDistance_));
            if (version >= 0) archive(CEREAL_NVP(endLookAtOffset_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameOverDeathCamera, 0)
