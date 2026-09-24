#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ScreenColorGrade/ScreenColorGradeRenderer.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace GamePlay::Ui
{
    // ローカルプレイヤーのHPに応じて、赤ビネットの鼓動・彩度低下・心音、ダウン中の暗転を出す
    class LowHealthScreenEffect final : public Component::ComponentBase,
                                        public LifeCycleCallback::IAwakable,
                                        public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const GameCore::PlayerAvatar::IPlayerAvatarStatus& model);

    private:
        void OnAwake  () override;
        void OnDestroy() override;
        void OnUpdate () override;

        void OnChangeHealth(const GameCore::StatusParameter::Health& health, bool isDeath);
        void ResetDownedFade(bool isDowned);
        void PlayHeartbeat(float danger) const;
        [[nodiscard]] float CalcDanger(float healthRate) const;
        [[nodiscard]] float CalcPulse (float sinceBeat_secs) const;

        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) vignette_;
        [[serialize(0)]] float startHealthRate_      = 0.35f;
        [[serialize(0)]] float criticalHealthRate_   = 0.10f;
        [[serialize(0)]] float dangerSmooth_secs_    = 0.4f;
        [[serialize(0)]] int   vignetteBlendRate_    = 170;
        [[serialize(0)]] int   pulseAddBlendRate_    = 60;
        [[serialize(0)]] float minBpm_               = 60.0f;
        [[serialize(0)]] float maxBpm_               = 130.0f;
        [[serialize(0)]] float dubDelay_secs_        = 0.22f;
        [[serialize(0)]] int   maxDesaturation_      = 150;
        [[serialize(0)]] int   downedDesaturation_   = 230;
        [[serialize(0)]] int   downedDarken_         = 90;
        [[serialize(0)]] float downedFade_secs_      = 1.0f;
        [[serialize(0)]] FIELD(Asset::SoundFile) heartbeatSound_;
        [[serialize(0)]] int   heartbeatMinVolume_   = 90;
        [[serialize(0)]] int   heartbeatMaxVolume_   = 200;
        [[serialize(2)]] float pulseDecay_secs_     = 0.09f;
        [[serialize(2)]] float dubPulseStrength_     = 0.6f;

        FIELD(NanamiUi::ScreenColorGradeRenderer) colorGrade_;
        NanamiEngine::R4::SerialDisposable                      subscription_;

        GameCore::StatusParameter::Health maxHealth_;
        GameCore::StatusParameter::Health lastHealth_;
        float healthRate_     = 1.0f;
        bool  isDowned_       = false;
        float danger_         = 0.0f;
        LibCore::Tween::TweenPlayer<float> downedFade_;
        float sinceBeat_secs_ = 10.0f;

        float debugOverrideHealthRate_ = -1.0f;
        bool  debugForceDowned_        = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(vignette_));
            archive(CEREAL_NVP(startHealthRate_));
            archive(CEREAL_NVP(criticalHealthRate_));
            archive(CEREAL_NVP(dangerSmooth_secs_));
            archive(CEREAL_NVP(vignetteBlendRate_));
            archive(CEREAL_NVP(pulseAddBlendRate_));
            archive(CEREAL_NVP(minBpm_));
            archive(CEREAL_NVP(maxBpm_));
            archive(CEREAL_NVP(dubDelay_secs_));
            archive(CEREAL_NVP(maxDesaturation_));
            archive(CEREAL_NVP(downedDesaturation_));
            archive(CEREAL_NVP(downedDarken_));
            archive(CEREAL_NVP(downedFade_secs_));
            archive(CEREAL_NVP(heartbeatSound_));
            archive(CEREAL_NVP(heartbeatMinVolume_));
            archive(CEREAL_NVP(heartbeatMaxVolume_));
            archive(CEREAL_NVP(pulseDecay_secs_));
            archive(CEREAL_NVP(dubPulseStrength_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(vignette_));
            if (version >= 0) archive(CEREAL_NVP(startHealthRate_));
            if (version >= 0) archive(CEREAL_NVP(criticalHealthRate_));
            if (version >= 0) archive(CEREAL_NVP(dangerSmooth_secs_));
            if (version >= 0) archive(CEREAL_NVP(vignetteBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(pulseAddBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(minBpm_));
            if (version >= 0) archive(CEREAL_NVP(maxBpm_));
            if (version >= 0) archive(CEREAL_NVP(dubDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(maxDesaturation_));
            if (version >= 0) archive(CEREAL_NVP(downedDesaturation_));
            if (version >= 0) archive(CEREAL_NVP(downedDarken_));
            if (version >= 0) archive(CEREAL_NVP(downedFade_secs_));
            if (version >= 0) archive(CEREAL_NVP(heartbeatSound_));
            if (version >= 0) archive(CEREAL_NVP(heartbeatMinVolume_));
            if (version >= 0) archive(CEREAL_NVP(heartbeatMaxVolume_));
            if (version >= 2) archive(CEREAL_NVP(pulseDecay_secs_));
            if (version >= 2) archive(CEREAL_NVP(dubPulseStrength_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::LowHealthScreenEffect, 2);
