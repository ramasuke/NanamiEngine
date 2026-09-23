#pragma once
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace GamePlay::Ui
{
    class ControlGuideRow;

    // 操作ガイドの見せ方だけを受け持つ。行は rowPrefab_ から rows_ の子へ生成し、行の積み上げは VerticalLayoutGroup が行う。
    // どの行に何を出すかはキャラごとのガイド（SwordManControlGuide など）が毎フレーム Present で渡す
    class ControlGuide final : public Component::ComponentBase
    {
    public:
        struct RowRequest
        {
            bool isShown  = false;
            bool isUsable = false;
            std::shared_ptr<Asset::SpriteFile> glyph;
            std::string_view label;
        };

        /// 生成順を行の添字として使う
        void SpawnRows(std::size_t count);
        /// @param focusedRow チュートリアルが指している行
        void Present(bool isShown, std::span<const RowRequest> requests, std::optional<std::size_t> focusedRow, bool isFocusCleared);
        /// 吹き出しを出す側が行の位置を知れるように、表示中の行の画面座標を返す
        [[nodiscard]] std::optional<glm::vec2> RowAnchor(std::size_t row) const;

    private:
        struct RowState
        {
            std::shared_ptr<Asset::SpriteFile> glyph;
            std::string label;
            LibCore::Tween::TweenPlayer<float> visibility;
            LibCore::Tween::TweenPlayer<float> usableRate;
            LibCore::Tween::TweenPlayer<float> focusRate;
            LibCore::Tween::TweenPlayer<float> pulse;
            bool  isActive          = false;
            bool  isContentDirty    = true;
            bool  isFocused         = false;
            bool  isFocusDirty      = true;
        };

        void AnimateRow(RowState& row, const RowRequest& request, bool isFocused, float deltaTime) const;
        void PresentRow(ControlGuideRow& view, RowState& row, const RowRequest& request, bool isCleared) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) rows_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;

        [[serialize(0)]] float slideDistance_px_ = 14.0f;
        [[serialize(0)]] float guideFadeDuration_secs_ = 0.3f;
        [[serialize(0)]] float rowFadeDuration_secs_ = 0.15f;
        [[serialize(0)]] float pulseDuration_secs_ = 0.45f;
        [[serialize(0)]] int accentGlowMaxAlpha_ = 230;
        [[serialize(0)]] int glyphFlashMaxAlpha_ = 115;
        [[serialize(0)]] int dimAlpha_ = 110;
        [[serialize(0)]] float labelShadowAlphaRate_ = 0.7f;

        [[serialize(0)]] float focusFadeDuration_secs_ = 0.18f;
        [[serialize(0)]] float focusPulsePeriod_secs_ = 0.9f;
        [[serialize(0)]] float focusArrowSwing_px_ = 5.0f;
        [[serialize(0)]] int focusGlyphFlashMaxAlpha_ = 90;
        /// フォーカス中、指していない行をさらに沈める割合
        [[serialize(0)]] float unfocusedDimRate_ = 0.3f;

        std::vector<std::weak_ptr<ControlGuideRow>> rowViews_;
        std::vector<RowState> rowStates_;
        LibCore::Tween::TweenPlayer<float> guideFade_;
        float focusElapsed_secs_ = 0.0f;
        float anyFocusRate_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(rows_));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(slideDistance_px_));
            archive(CEREAL_NVP(guideFadeDuration_secs_));
            archive(CEREAL_NVP(rowFadeDuration_secs_));
            archive(CEREAL_NVP(pulseDuration_secs_));
            archive(CEREAL_NVP(accentGlowMaxAlpha_));
            archive(CEREAL_NVP(glyphFlashMaxAlpha_));
            archive(CEREAL_NVP(dimAlpha_));
            archive(CEREAL_NVP(labelShadowAlphaRate_));
            archive(CEREAL_NVP(focusFadeDuration_secs_));
            archive(CEREAL_NVP(focusPulsePeriod_secs_));
            archive(CEREAL_NVP(focusArrowSwing_px_));
            archive(CEREAL_NVP(focusGlyphFlashMaxAlpha_));
            archive(CEREAL_NVP(unfocusedDimRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rows_));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            if (version >= 0) archive(CEREAL_NVP(slideDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(guideFadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(rowFadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(pulseDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(accentGlowMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(glyphFlashMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(dimAlpha_));
            if (version >= 0) archive(CEREAL_NVP(labelShadowAlphaRate_));
            if (version >= 0) archive(CEREAL_NVP(focusFadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(focusPulsePeriod_secs_));
            if (version >= 0) archive(CEREAL_NVP(focusArrowSwing_px_));
            if (version >= 0) archive(CEREAL_NVP(focusGlyphFlashMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(unfocusedDimRate_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ControlGuide, 0);
