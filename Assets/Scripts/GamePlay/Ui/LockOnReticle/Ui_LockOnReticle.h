#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GameCore::PlayerAvatar
{
    class PlayerAvatarCameraGroupBase;
}

namespace GamePlay::Ui
{
    // ロックオン中の対象に照準を、未ロック時は「今押したら狙う対象」に候補マーカーを画面座標で重ねる。
    // ローカルプレイヤーの CameraGroup プレハブの子に置き、親の CameraGroup の状態を毎フレーム参照する
    class LockOnReticle final : public Component::ComponentBase,
                                public LifeCycleCallback::IInitRenderable,
                                public LifeCycleCallback::IUserInterfaceRenderable,
                                public LifeCycleCallback::IUpdatable
    {
    private:
        enum class Phase
        {
            Hidden,
            Engaging,  // ロック確定の瞬間: 大きい所から縮んでスナップ
            Locked,
            Releasing, // 解除: 広がりながらフェードアウト
        };

        void InitRenderer() override;
        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[nodiscard]] std::shared_ptr<GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase> CatchCameraGroup();
        static void DrawSprite(
            const std::shared_ptr<Asset::SpriteFile>& sprite,
            const glm::vec3& worldPos,
            float scale,
            float angle,
            float alpha);

        std::weak_ptr<GameCore::PlayerAvatar::PlayerAvatarCameraGroupBase> cameraGroup_;

        Phase     phase_           = Phase::Hidden;
        float     phaseTime_secs_  = 0.0f;
        float     elapsed_secs_    = 0.0f;
        float     ringAngle_       = 0.0f;
        bool      wasEngaged_      = false;
        std::weak_ptr<GameObject::IGameObject> lockedTarget_;
        glm::vec3 lockOnPointWorld_ = {};

        float     candidateFade_   = 0.0f;
        std::weak_ptr<GameObject::IGameObject> candidateTarget_;
        glm::vec3 candidatePointWorld_ = {};

        [[serialize(0)]] int   renderOrder_           = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile) ringSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) bracketSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) candidateSprite_;
        [[serialize(0)]] float reticleScale_          = 0.38f;
        [[serialize(0)]] float candidateScale_        = 0.44f;
        [[serialize(0)]] float candidateAlpha_        = 0.6f;
        [[serialize(0)]] float ringRotateSpeed_       = 0.25f;
        [[serialize(0)]] float lockedBreathScale_     = 0.005f;
        [[serialize(0)]] float engageDuration_secs_   = 0.12f;
        [[serialize(0)]] float engageStartScaleRate_  = 2.0f;
        [[serialize(0)]] float releaseDuration_secs_  = 0.22f;
        [[serialize(0)]] float releaseEndScaleRate_   = 1.5f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(ringSprite_));
            archive(CEREAL_NVP(bracketSprite_));
            archive(CEREAL_NVP(candidateSprite_));
            archive(CEREAL_NVP(reticleScale_));
            archive(CEREAL_NVP(candidateScale_));
            archive(CEREAL_NVP(candidateAlpha_));
            archive(CEREAL_NVP(ringRotateSpeed_));
            archive(CEREAL_NVP(lockedBreathScale_));
            archive(CEREAL_NVP(engageDuration_secs_));
            archive(CEREAL_NVP(engageStartScaleRate_));
            archive(CEREAL_NVP(releaseDuration_secs_));
            archive(CEREAL_NVP(releaseEndScaleRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(ringSprite_));
            if (version >= 0) archive(CEREAL_NVP(bracketSprite_));
            if (version >= 0) archive(CEREAL_NVP(candidateSprite_));
            if (version >= 0) archive(CEREAL_NVP(reticleScale_));
            if (version >= 0) archive(CEREAL_NVP(candidateScale_));
            if (version >= 0) archive(CEREAL_NVP(candidateAlpha_));
            if (version >= 0) archive(CEREAL_NVP(ringRotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(lockedBreathScale_));
            if (version >= 0) archive(CEREAL_NVP(engageDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(engageStartScaleRate_));
            if (version >= 0) archive(CEREAL_NVP(releaseDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(releaseEndScaleRate_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LockOnReticle, 0)
