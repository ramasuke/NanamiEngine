#pragma once
#include <string>

#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Font/Ttf/TtfFontFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable.h"

namespace GamePlay::Ui
{
    /**
     * @brief 目的が変わったときに上中央へ一度だけ出す字幕。
     * NOTE: 会話中・操作ロック中は明けるのを待ってから出す
     */
    class NavigationBanner final : public Component::ComponentBase,
                                   public LifeCycleCallback::IUpdatable,
                                   public LifeCycleCallback::IUserInterfaceRenderable
    {
    private:
        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        /** @return 0..1。出ていなければ 0 */
        [[nodiscard]] float Alpha() const;
        void DrawCenteredText(const std::string& text, const glm::vec2& centre, float scale, const Color32& color, float alpha) const;

        [[serialize(0)]] int                        renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile)   bandSprite_;
        [[serialize(0)]] FIELD(Asset::TtfFontFile)  font_;
        [[serialize(0)]] std::string                headingText_ = "— 新 た な 目 的 —";
        [[serialize(0)]] float                      centerY_px_ = 196.0f;
        [[serialize(0)]] float                      headingOffsetY_px_ = -22.0f;
        [[serialize(0)]] float                      titleOffsetY_px_ = 12.0f;
        [[serialize(0)]] float                      headingScale_ = 0.3f;
        [[serialize(0)]] float                      titleScale_ = 0.62f;
        [[serialize(0)]] Color32                    headingColor_ = Color32(255, 206, 104);
        [[serialize(0)]] Color32                    titleColor_ = Color32(255, 255, 247);
        [[serialize(0)]] float                      bandAlphaRate_ = 1.0f;
        [[serialize(0)]] float                      riseDistance_px_ = 10.0f;
        [[serialize(0)]] float                      fadeIn_secs_ = 0.4f;
        [[serialize(0)]] float                      hold_secs_ = 3.5f;
        [[serialize(0)]] float                      fadeOut_secs_ = 0.6f;

        std::string title_;
        float       elapsed_secs_ = 0.0f;
        bool        isPlaying_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(bandSprite_));
            archive(CEREAL_NVP(font_));
            archive(CEREAL_NVP(headingText_));
            archive(CEREAL_NVP(centerY_px_));
            archive(CEREAL_NVP(headingOffsetY_px_));
            archive(CEREAL_NVP(titleOffsetY_px_));
            archive(CEREAL_NVP(headingScale_));
            archive(CEREAL_NVP(titleScale_));
            archive(CEREAL_NVP(headingColor_));
            archive(CEREAL_NVP(titleColor_));
            archive(CEREAL_NVP(bandAlphaRate_));
            archive(CEREAL_NVP(riseDistance_px_));
            archive(CEREAL_NVP(fadeIn_secs_));
            archive(CEREAL_NVP(hold_secs_));
            archive(CEREAL_NVP(fadeOut_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(bandSprite_));
            if (version >= 0) archive(CEREAL_NVP(font_));
            if (version >= 0) archive(CEREAL_NVP(headingText_));
            if (version >= 0) archive(CEREAL_NVP(centerY_px_));
            if (version >= 0) archive(CEREAL_NVP(headingOffsetY_px_));
            if (version >= 0) archive(CEREAL_NVP(titleOffsetY_px_));
            if (version >= 0) archive(CEREAL_NVP(headingScale_));
            if (version >= 0) archive(CEREAL_NVP(titleScale_));
            if (version >= 0) archive(CEREAL_NVP(headingColor_));
            if (version >= 0) archive(CEREAL_NVP(titleColor_));
            if (version >= 0) archive(CEREAL_NVP(bandAlphaRate_));
            if (version >= 0) archive(CEREAL_NVP(riseDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(fadeIn_secs_));
            if (version >= 0) archive(CEREAL_NVP(hold_secs_));
            if (version >= 0) archive(CEREAL_NVP(fadeOut_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::NavigationBanner, 0);
