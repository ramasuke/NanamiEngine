#pragma once
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    // NOTE: 狩り場初到着の空撮の字幕。島の名前は中央に大きく、見どころの名前は下に小さく出す
    class StageArrivalCaption final : public Component::ComponentBase,
                                      public LifeCycleCallback::IUpdatable
    {
    public:
        void ShowIsland  (const std::string& title, const std::string& subtitle);
        void ShowLandmark(const std::string& title, const std::string& subtitle);
        void Hide();
        // NOTE: 消えきったら GameObject ごと片付ける
        void HideAndDestroy();
        [[nodiscard]] float FadeOut_secs() const { return fadeOut_secs_; }

    private:
        struct Card
        {
            float rate   = 0.0f;
            float target = 0.0f;
        };

        void OnUpdate() override;
        void ApplyIsland  () const;
        void ApplyLandmark() const;
        static void StepCard(Card& card, float deltaSecs, float fadeIn_secs, float fadeOut_secs);
        static int ToBlendRate(float rate, int maxBlendRate);

        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) islandVignette_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) islandRuleTop_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) islandRuleBottom_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer)       islandTitle_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer)       islandSubtitle_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) landmarkBand_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) landmarkRule_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer)       landmarkTitle_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer)       landmarkSubtitle_;
        [[serialize(0)]] float fadeIn_secs_       = 0.8f;
        [[serialize(0)]] float fadeOut_secs_      = 0.6f;
        // NOTE: 周りを暗くする幕と墨の帯は、出しきっても少し透かす
        [[serialize(0)]] int   vignetteBlendRate_ = 200;
        [[serialize(0)]] int   bandBlendRate_     = 210;

        Card island_;
        Card landmark_;
        bool isDestroyRequested_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(islandVignette_));
            archive(CEREAL_NVP(islandRuleTop_));
            archive(CEREAL_NVP(islandRuleBottom_));
            archive(CEREAL_NVP(islandTitle_));
            archive(CEREAL_NVP(islandSubtitle_));
            archive(CEREAL_NVP(landmarkBand_));
            archive(CEREAL_NVP(landmarkRule_));
            archive(CEREAL_NVP(landmarkTitle_));
            archive(CEREAL_NVP(landmarkSubtitle_));
            archive(CEREAL_NVP(fadeIn_secs_));
            archive(CEREAL_NVP(fadeOut_secs_));
            archive(CEREAL_NVP(vignetteBlendRate_));
            archive(CEREAL_NVP(bandBlendRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(islandVignette_));
            if (version >= 0) archive(CEREAL_NVP(islandRuleTop_));
            if (version >= 0) archive(CEREAL_NVP(islandRuleBottom_));
            if (version >= 0) archive(CEREAL_NVP(islandTitle_));
            if (version >= 0) archive(CEREAL_NVP(islandSubtitle_));
            if (version >= 0) archive(CEREAL_NVP(landmarkBand_));
            if (version >= 0) archive(CEREAL_NVP(landmarkRule_));
            if (version >= 0) archive(CEREAL_NVP(landmarkTitle_));
            if (version >= 0) archive(CEREAL_NVP(landmarkSubtitle_));
            if (version >= 0) archive(CEREAL_NVP(fadeIn_secs_));
            if (version >= 0) archive(CEREAL_NVP(fadeOut_secs_));
            if (version >= 0) archive(CEREAL_NVP(vignetteBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(bandBlendRate_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageArrivalCaption, 0);
