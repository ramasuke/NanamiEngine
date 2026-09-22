#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Awake/IAwakable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    /**
     * @brief ゲームオーバー画面の選択肢1枚（釘で打った鉄札）。
     *        選ばれている札は縁の焼けた絵へ切り替わり、縁の熾火がゆっくり脈打つ。
     *        時間は GameOverScreenUi が壁時計で Tick して進める
     */
    class GameOverButton final : public Component::ComponentBase,
                                 public LifeCycleCallback::IAwakable
    {
    public:
        /** @param appearRate 0で見えない、1で出切った状態 */
        void SetAppearRate(float appearRate);
        void SetHighlighted(bool isHighlighted);
        void Tick(float deltaSecs);

        [[nodiscard]] NanamiEngine::R4::Observable<NanamiUi::MouseState> OnClick() const;
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnHover() const;

    private:
        void OnAwake() override;
        void Apply() const;

        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) plate_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) plateLit_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) ember_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) label_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) labelShadow_;
        [[serialize(0)]] Color32 labelColor_ = Color32(188, 196, 204);
        [[serialize(0)]] Color32 labelLitColor_ = Color32(255, 232, 198);
        [[serialize(0)]] float highlightFadeSecs_ = 0.12f;
        [[serialize(0)]] float emberPulseHz_ = 0.8f;
        [[serialize(0)]] int emberMinBlendRate_ = 110;
        [[serialize(0)]] int emberMaxBlendRate_ = 220;

        std::weak_ptr<NanamiUi::Button> button_;
        float appearRate_ = 0.0f;
        float highlight_ = 0.0f;
        bool isHighlighted_ = false;
        float emberPhase_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(plate_));
            archive(CEREAL_NVP(plateLit_));
            archive(CEREAL_NVP(ember_));
            archive(CEREAL_NVP(label_));
            archive(CEREAL_NVP(labelShadow_));
            archive(CEREAL_NVP(labelColor_));
            archive(CEREAL_NVP(labelLitColor_));
            archive(CEREAL_NVP(highlightFadeSecs_));
            archive(CEREAL_NVP(emberPulseHz_));
            archive(CEREAL_NVP(emberMinBlendRate_));
            archive(CEREAL_NVP(emberMaxBlendRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(plate_));
            if (version >= 0) archive(CEREAL_NVP(plateLit_));
            if (version >= 0) archive(CEREAL_NVP(ember_));
            if (version >= 0) archive(CEREAL_NVP(label_));
            if (version >= 0) archive(CEREAL_NVP(labelShadow_));
            if (version >= 0) archive(CEREAL_NVP(labelColor_));
            if (version >= 0) archive(CEREAL_NVP(labelLitColor_));
            if (version >= 0) archive(CEREAL_NVP(highlightFadeSecs_));
            if (version >= 0) archive(CEREAL_NVP(emberPulseHz_));
            if (version >= 0) archive(CEREAL_NVP(emberMinBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(emberMaxBlendRate_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameOverButton, 0)
