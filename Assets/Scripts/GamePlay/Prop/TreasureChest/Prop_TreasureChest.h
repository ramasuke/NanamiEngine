#pragma once
#include <optional>

#include "gtc/quaternion.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../../../Data/Drop/Data_DropTable.h"
#include "../../../Core/Game/PlayerAvatar/Interactable/IPlayerInteractable.h"
#include "../../Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"

namespace GamePlay::Prop
{
    /** @brief 調べると箱が揺れてからフタが開き、開き切ったら dropTable_ を散らす宝箱。開けたことは保存しない */
    class TreasureChest final : public Component::ComponentBase,
                                public LifeCycleCallback::IStartable,
                                public LifeCycleCallback::IUpdatable,
                                public GameCore::PlayerAvatar::IPlayerInteractable
    {
    private:
        void OnStart           () override;
        void OnUpdate          () override;
        void OnInteractable    () override;
        void OnExitInteractable() override;
        void OnInteract        () override;
        [[nodiscard]] bool CanInteract() const override { return !isOpened_; }
        [[nodiscard]] const GameObject::Transform& InteractableTransform() const override;
        [[nodiscard]] glm::vec3 DropPosition() const;
        void SetLidAngle(float angle_deg);
        void ApplyShake(float elapsed_secs);
        void StartOpenLid();
        void SpillLoot();

        [[serialize(0)]] FIELD(Asset::DropTable) dropTable_;
        /** ヒンジ位置が原点のフタ。ローカル X 軸回りに回す */
        [[serialize(0)]] FIELD(GameObject::IGameObject) lid_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) dropPoint_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) openParticle_;
        [[serialize(0)]] FIELD(Asset::SoundFile) openSound_;
        [[serialize(0)]] FIELD(Ui::BillBoardNpcChatIcon) chatIcon_;
        /** 未開封の間だけ流す光。開けたら止める */
        [[serialize(1)]] FIELD(Component::ParticleSystem) idleParticle_;
        [[serialize(0)]] float openAngle_deg_     = -105.0f;
        [[serialize(0)]] float openDuration_secs_ = 0.45f;
        /** 開ける前に lid_ と一緒に揺らす箱の本体 */
        [[serialize(2)]] FIELD(GameObject::IGameObject) body_;
        [[serialize(2)]] float shakeDuration_secs_ = 0.55f;
        [[serialize(2)]] float shakeAngle_deg_     = 5.0f;
        [[serialize(2)]] float shakeFrequency_hz_  = 9.0f;

        struct ClosedPose
        {
            glm::vec3 pos;
            glm::quat rot;
        };
        std::optional<ClosedPose> bodyClosedPose_;
        std::optional<ClosedPose> lidClosedPose_;
        LibCore::Tween::TweenPlayer<float> lidTween_;
        float shakeElapsed_secs_ = 0.0f;
        bool  isOpened_         = false;
        bool  isLidOpening_     = false;
        bool  isSpilled_        = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(dropTable_));
            archive(CEREAL_NVP(lid_));
            archive(CEREAL_NVP(dropPoint_));
            archive(CEREAL_NVP(openParticle_));
            archive(CEREAL_NVP(openSound_));
            archive(CEREAL_NVP(chatIcon_));
            archive(CEREAL_NVP(openAngle_deg_));
            archive(CEREAL_NVP(openDuration_secs_));
            archive(CEREAL_NVP(idleParticle_));
            archive(CEREAL_NVP(body_));
            archive(CEREAL_NVP(shakeDuration_secs_));
            archive(CEREAL_NVP(shakeAngle_deg_));
            archive(CEREAL_NVP(shakeFrequency_hz_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(dropTable_));
            if (version >= 0) archive(CEREAL_NVP(lid_));
            if (version >= 0) archive(CEREAL_NVP(dropPoint_));
            if (version >= 0) archive(CEREAL_NVP(openParticle_));
            if (version >= 0) archive(CEREAL_NVP(openSound_));
            if (version >= 0) archive(CEREAL_NVP(chatIcon_));
            if (version >= 0) archive(CEREAL_NVP(openAngle_deg_));
            if (version >= 0) archive(CEREAL_NVP(openDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(idleParticle_));
            if (version >= 2) archive(CEREAL_NVP(body_));
            if (version >= 2) archive(CEREAL_NVP(shakeDuration_secs_));
            if (version >= 2) archive(CEREAL_NVP(shakeAngle_deg_));
            if (version >= 2) archive(CEREAL_NVP(shakeFrequency_hz_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::TreasureChest, 2);
