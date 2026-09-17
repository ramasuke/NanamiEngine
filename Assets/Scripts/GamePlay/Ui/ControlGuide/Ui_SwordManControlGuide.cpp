#include "Ui_SwordManControlGuide.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

#include "Ui_SwordManControlGuideRow.h"
#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/Status/ControlGuideFocus/SwordMan_IControlGuideFocusPresentation.h"
#include "../../PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GamePlay::Ui
{
    namespace
    {
        using GameCore::PlayerAvatar::PlayerAvatarInputDevice;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarControlAcceptance;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarInput;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarInputPhase;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateAction;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateType;
        using GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocus;
        using GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocusState;

        float MoveTowards(const float current, const float target, const float maxDelta)
        {
            if (current < target)
                return std::min(current + maxDelta, target);
            return std::max(current - maxDelta, target);
        }

        float StepRate(const float deltaTime, const float duration_secs)
        {
            return duration_secs > 0.0f ? deltaTime / duration_secs : 1.0f;
        }

        int ToBlendRate(const float alpha)
        {
            return std::clamp(static_cast<int>(alpha), 0, 255);
        }
    }

    class SwordManControlGuide::RequestCollector final : public GameCore::PlayerAvatar::SwordMan::ISwordManAvatarTransitionVisitor
    {
    public:
        [[nodiscard]] const RowRequests& Requests() const { return requests_; }

        bool Automatic(SwordManAvatarStateType, bool) override { return false; }

        bool OnInput(const SwordManAvatarStateType to, const SwordManAvatarInput input, const SwordManAvatarInputPhase phase, const bool isUsable) override
        {
            if (const auto label = TransitionLabel(to, phase))
                Offer(InputGlyph(input), *label, isUsable);
            return false;
        }

        bool OnInputWhenReady(const SwordManAvatarStateType to, const SwordManAvatarInput input, const SwordManAvatarInputPhase phase, const bool isUsable, bool) override
        {
            return OnInput(to, input, phase, isUsable);
        }

        void Action(const SwordManAvatarStateAction action, const bool isUsable) override
        {
            switch (action)
            {
            case SwordManAvatarStateAction::Move:          Offer(Glyph::Move,           Label::Move,          isUsable); return;
            case SwordManAvatarStateAction::ComboAttack:   Offer(Glyph::Attack,         Label::Attack,        isUsable); return;
            case SwordManAvatarStateAction::LockOn:        Offer(Glyph::LockOn,         Label::LockOn,        isUsable); return;
            case SwordManAvatarStateAction::LockOnRelease: Offer(Glyph::LockOn,         Label::LockOnRelease, isUsable); return;
            case SwordManAvatarStateAction::CannonTurn:    Offer(Glyph::MoveHorizontal, Label::CannonTurn,    isUsable); return;
            case SwordManAvatarStateAction::CannonFire:    Offer(Glyph::Attack,         Label::CannonFire,    isUsable); return;
            // アイテムの切替/使用は専用のアイテム欄が出すので、操作ガイドには行を持たない
            case SwordManAvatarStateAction::CycleItem:
            case SwordManAvatarStateAction::UseItem:       return;
            }
        }

        [[nodiscard]] static Row RowOf(const Label label)
        {
            switch (label)
            {
            case Label::Move:
            case Label::CannonTurn:          return Row::Move;
            case Label::Attack:
            case Label::DashAttack:
            case Label::JumpAttack:
            case Label::CannonFire:          return Row::Attack;
            case Label::ChargeAttackHold:
            case Label::ChargeAttackRelease: return Row::ChargeAttack;
            case Label::Run:                 return Row::Run;
            case Label::Jump:                return Row::Jump;
            case Label::AvoidRolling:        return Row::AvoidRolling;
            case Label::LockOn:
            case Label::LockOnRelease:       return Row::LockOn;
            case Label::Chat:
            case Label::WakeUp:              return Row::Interact;
            }
            return Row::Move;
        }

    private:
        // 同じ行に複数届いたら、先に届いた使える方を出す
        void Offer(const Glyph glyph, const Label label, const bool isUsable)
        {
            if (!isUsable && IsHiddenWhenUnusable(label))
                return;

            auto& request = requests_[static_cast<std::size_t>(RowOf(label))];
            if (request.isShown && (request.isUsable || !isUsable))
                return;
            request = RowRequest{ true, isUsable, glyph, label };
        }

        [[nodiscard]] static std::optional<Label> TransitionLabel(const SwordManAvatarStateType to, const SwordManAvatarInputPhase phase)
        {
            switch (to)
            {
            case SwordManAvatarStateType::Walk:
            case SwordManAvatarStateType::InjuredWalk:
                return phase == SwordManAvatarInputPhase::Holding ? std::optional(Label::Move) : std::nullopt;
            case SwordManAvatarStateType::Run:
            case SwordManAvatarStateType::InjuredRun:
                return phase == SwordManAvatarInputPhase::Holding ? std::optional(Label::Run) : std::nullopt;
            case SwordManAvatarStateType::Jump:                 return Label::Jump;
            case SwordManAvatarStateType::AvoidRolling:         return Label::AvoidRolling;
            case SwordManAvatarStateType::NormalAttack:
                return phase == SwordManAvatarInputPhase::Pressed ? std::optional(Label::Attack) : std::nullopt;
            case SwordManAvatarStateType::JumpAttackAir:
                return phase == SwordManAvatarInputPhase::Pressed ? std::optional(Label::JumpAttack) : std::nullopt;
            case SwordManAvatarStateType::DashAttack:           return Label::DashAttack;
            case SwordManAvatarStateType::ChargeAttackCharging: return Label::ChargeAttackHold;
            case SwordManAvatarStateType::ChargeAttackRelease:  return Label::ChargeAttackRelease;
            case SwordManAvatarStateType::Chatting:             return Label::Chat;
            case SwordManAvatarStateType::WakeUp:               return Label::WakeUp;
            default:                                            return std::nullopt;
            }
        }

        [[nodiscard]] static Glyph InputGlyph(const SwordManAvatarInput input)
        {
            switch (input)
            {
            case SwordManAvatarInput::Move:         return Glyph::Move;
            case SwordManAvatarInput::Run:          return Glyph::Run;
            case SwordManAvatarInput::Jump:         return Glyph::Jump;
            case SwordManAvatarInput::AvoidRolling: return Glyph::AvoidRolling;
            case SwordManAvatarInput::NormalAttack:
            case SwordManAvatarInput::DashAttack:
            case SwordManAvatarInput::CannonAttack: return Glyph::Attack;
            case SwordManAvatarInput::Chat:         return Glyph::Interact;
            case SwordManAvatarInput::LockOn:       return Glyph::LockOn;
            }
            return Glyph::Move;
        }

        // 条件が揃ったときにだけ現れる操作
        [[nodiscard]] static bool IsHiddenWhenUnusable(const Label label)
        {
            return label == Label::Chat || label == Label::WakeUp || label == Label::LockOn || label == Label::LockOnRelease;
        }

        RowRequests requests_{};
    };

    void SwordManControlGuide::Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar)
    {
        swordManAvatar_ = swordManAvatar;
        SpawnRows();
    }

    void SwordManControlGuide::SpawnRows()
    {
        if (!rowViews_.empty() || !rowPrefab_)
            return;

        if (!rows_)
            return;
        const auto rowsObject = rows_.get();

        // 生成順を Row の添字として使うので、生成に失敗した行も詰めずに残す
        for (std::size_t i = 0; i < static_cast<std::size_t>(Row::Count); ++i)
        {
            const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab_.get(), rowsObject).lock();
            rowViews_.push_back(rowObject ? rowObject->Components().Catch<SwordManControlGuideRow>() : std::weak_ptr<SwordManControlGuideRow>{});
        }
    }

    SwordManControlGuide::Row SwordManControlGuide::FocusRow(const SwordManControlGuideFocus target)
    {
        switch (target)
        {
        case SwordManControlGuideFocus::Move:         return Row::Move;
        case SwordManControlGuideFocus::Attack:       return Row::Attack;
        case SwordManControlGuideFocus::ChargeAttack: return Row::ChargeAttack;
        case SwordManControlGuideFocus::Run:          return Row::Run;
        case SwordManControlGuideFocus::Jump:         return Row::Jump;
        case SwordManControlGuideFocus::AvoidRolling: return Row::AvoidRolling;
        case SwordManControlGuideFocus::LockOn:       return Row::LockOn;
        case SwordManControlGuideFocus::Interact:     return Row::Interact;
        case SwordManControlGuideFocus::None:         break;
        }
        return Row::Count;
    }

    void SwordManControlGuide::ApplyFocusRequest(const SwordManControlGuideFocus target)
    {
        const Row row = FocusRow(target);
        if (row == Row::Count)
            return;

        auto& request = requests_[static_cast<std::size_t>(row)];
        if (request.isShown)
            return;

        // State が出していない操作でも、指された行は「まだ使えない」姿で見せる
        switch (row)
        {
        case Row::Move:         request = RowRequest{ true, false, Glyph::Move,         Label::Move             }; return;
        case Row::Attack:       request = RowRequest{ true, false, Glyph::Attack,       Label::Attack           }; return;
        case Row::ChargeAttack: request = RowRequest{ true, false, Glyph::Attack,       Label::ChargeAttackHold }; return;
        case Row::Run:          request = RowRequest{ true, false, Glyph::Run,          Label::Run              }; return;
        case Row::Jump:         request = RowRequest{ true, false, Glyph::Jump,         Label::Jump             }; return;
        case Row::AvoidRolling: request = RowRequest{ true, false, Glyph::AvoidRolling, Label::AvoidRolling     }; return;
        case Row::LockOn:       request = RowRequest{ true, false, Glyph::LockOn,       Label::LockOn           }; return;
        case Row::Interact:     request = RowRequest{ true, false, Glyph::Interact,     Label::Chat             }; return;
        case Row::Count:        return;
        }
    }

    void SwordManControlGuide::AnimateRow(RowState& row, const RowRequest& request, const bool isFocused, const float deltaTime) const
    {
        const bool isActive = request.isShown && request.isUsable;
        const bool isContentChanged = request.isShown && (row.glyph != request.glyph || row.label != request.label);
        if (isActive && (!row.isActive || isContentChanged))
            row.pulseElapsed_secs = 0.0f;
        else
            row.pulseElapsed_secs += deltaTime;
        row.isActive = isActive;

        // 消えていく行は直前の中身のままフェードさせる
        if (isContentChanged)
        {
            row.glyph          = request.glyph;
            row.label          = request.label;
            row.isContentDirty = true;
        }

        if (row.isFocused != isFocused)
        {
            row.isFocused    = isFocused;
            row.isFocusDirty = true;
        }

        const float step = StepRate(deltaTime, rowFadeDuration_secs_);
        row.visibility = MoveTowards(row.visibility, request.isShown  ? 1.0f : 0.0f, step);
        row.usableRate = MoveTowards(row.usableRate, request.isUsable ? 1.0f : 0.0f, step);
        row.focusRate  = MoveTowards(row.focusRate, isFocused ? 1.0f : 0.0f, StepRate(deltaTime, focusFadeDuration_secs_));
    }

    void SwordManControlGuide::PresentRow(SwordManControlGuideRow& view, RowState& row, const RowRequest& request, const bool isCleared) const
    {
        // 出始めた行はすぐ有効にして枠を確保し、消える行はフェードし終えてから無効にしてレイアウトから外す
        const bool isEnabled = guideAlpha_ > 0.0f && (request.isShown || row.visibility > 0.0f);
        if (view.IsEnable() != isEnabled)
        {
            if (const auto entity = view.Entity().lock())
                entity->SetEnable(isEnabled);
        }
        if (!isEnabled)
            return;

        if (row.isContentDirty)
        {
            view.SetContent(GlyphSprite(row.glyph), LabelText(row.label));
            row.isContentDirty = false;
        }
        if (row.isFocusDirty)
        {
            view.SetFocused(row.isFocused);
            row.isFocusDirty = false;
        }

        const float usableAlphaRate = std::lerp(static_cast<float>(dimAlpha_) / 255.0f, 1.0f, row.usableRate);
        // 指されている行は常に最前面の明るさ、それ以外はフォーカス中だけさらに沈める
        const float focusDimRate = std::lerp(1.0f - unfocusedDimRate_ * anyFocusRate_, 1.0f, row.focusRate);
        const float bodyAlpha  = 255.0f * guideAlpha_ * row.visibility * std::max(usableAlphaRate, row.focusRate) * focusDimRate;
        const float pulse      = pulseDuration_secs_ > 0.0f ? std::max(0.0f, 1.0f - row.pulseElapsed_secs / pulseDuration_secs_) : 0.0f;
        const float pulseAlpha = pulse * guideAlpha_ * row.visibility;
        const float hidden     = 1.0f - row.visibility;
        // 行の枠もフェードと一緒に smoothstep で伸び縮みさせ、上下の行を跳ねさせない
        const float slotRate   = row.visibility * row.visibility * (3.0f - 2.0f * row.visibility);

        const float focusAlpha  = row.focusRate * guideAlpha_ * row.visibility;
        const float breath      = 0.5f + 0.5f * std::sin(focusElapsed_secs_ * 2.0f * std::numbers::pi_v<float> / std::max(focusPulsePeriod_secs_, 0.01f));
        const float markAlpha   = 255.0f * focusAlpha * std::lerp(0.55f, 1.0f, breath);

        view.Apply(SwordManControlGuideRow::Appearance{
            .slotRate            = slotRate,
            .slideOffset_px      = -slideDistance_px_ * hidden * hidden,
            .bodyAlpha           = ToBlendRate(bodyAlpha),
            .labelShadowAlpha    = ToBlendRate(bodyAlpha * labelShadowAlphaRate_),
            .accentGlowAlpha     = ToBlendRate(static_cast<float>(accentGlowMaxAlpha_) * pulseAlpha),
            .glyphFlashAlpha     = ToBlendRate(std::max(static_cast<float>(glyphFlashMaxAlpha_) * pulseAlpha,
                                                       static_cast<float>(focusGlyphFlashMaxAlpha_) * focusAlpha * breath)),
            .focusStripAlpha     = ToBlendRate(255.0f * focusAlpha),
            .focusArrowAlpha     = ToBlendRate(isCleared ? 0.0f : markAlpha),
            .focusCheckAlpha     = ToBlendRate(isCleared ? 255.0f * focusAlpha : 0.0f),
            .focusArrowOffset_px = focusArrowSwing_px_ * breath,
        });
    }

    void SwordManControlGuide::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();
        const auto swordManAvatar = swordManAvatar_.lock();
        const auto state = swordManAvatar ? swordManAvatar->GetStateMachine().CurrentStateValue() : nullptr;
        const auto acceptance = state ? state->ControlAcceptance() : SwordManAvatarControlAcceptance::None;
        if (swordManAvatar)
            device_ = swordManAvatar->GetInputAction().CurrentDevice();

        if (acceptance == SwordManAvatarControlAcceptance::Accept)
        {
            RequestCollector collector;
            state->VisitTransitions(collector);
            requests_ = collector.Requests();
        }

        const SwordManControlGuideFocusState focus =
            swordManAvatar ? swordManAvatar->PlayerStatus().GuideFocusPresentation().Current() : SwordManControlGuideFocusState{};
        const Row focusedRow = FocusRow(focus.target);
        if (focusedRow != Row::Count)
            ApplyFocusRequest(focus.target);

        const bool isGuideShown = acceptance != SwordManAvatarControlAcceptance::None;
        guideAlpha_ = MoveTowards(guideAlpha_, isGuideShown ? 1.0f : 0.0f, StepRate(deltaTime, guideFadeDuration_secs_));
        focusElapsed_secs_ = focusedRow == Row::Count ? 0.0f : focusElapsed_secs_ + deltaTime;

        const std::size_t count = std::min(rowStates_.size(), rowViews_.size());
        anyFocusRate_ = 0.0f;
        for (std::size_t i = 0; i < count; ++i)
        {
            if (isGuideShown)
                AnimateRow(rowStates_[i], requests_[i], static_cast<std::size_t>(focusedRow) == i, deltaTime);
            anyFocusRate_ = std::max(anyFocusRate_, rowStates_[i].focusRate);
        }
        for (std::size_t i = 0; i < count; ++i)
        {
            if (const auto view = rowViews_[i].lock())
                PresentRow(*view, rowStates_[i], requests_[i], focus.isCleared);
        }

        ReportFocusAnchor(swordManAvatar, focusedRow);
    }

    void SwordManControlGuide::ReportFocusAnchor(
        const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar, const Row focusedRow) const
    {
        if (!swordManAvatar)
            return;

        auto& presentation = swordManAvatar->PlayerStatus().GuideFocusPresentation();
        const std::size_t index = static_cast<std::size_t>(focusedRow);
        if (focusedRow == Row::Count || index >= rowViews_.size() || guideAlpha_ <= 0.0f)
        {
            presentation.ReportFocusAnchor(std::nullopt);
            return;
        }

        const auto view = rowViews_[index].lock();
        const auto entity = view ? view->Entity().lock() : nullptr;
        if (!entity || !view->IsEnable())
        {
            presentation.ReportFocusAnchor(std::nullopt);
            return;
        }

        const glm::vec3 worldPos = entity->Transform().GetWorldPos();
        presentation.ReportFocusAnchor(glm::vec2(worldPos.x, worldPos.y));
    }

    std::shared_ptr<Asset::SpriteFile> SwordManControlGuide::GlyphSprite(const Glyph glyph) const
    {
        const bool isPad = device_ == PlayerAvatarInputDevice::Gamepad;
        switch (glyph)
        {
        case Glyph::Move:           return (isPad ? padMoveSprite_           : keyMoveSprite_          ).get();
        case Glyph::MoveHorizontal: return (isPad ? padMoveHorizontalSprite_ : keyMoveHorizontalSprite_).get();
        case Glyph::Attack:         return (isPad ? padAttackSprite_         : keyAttackSprite_        ).get();
        case Glyph::Run:            return (isPad ? padRunSprite_            : keyRunSprite_           ).get();
        case Glyph::Jump:           return (isPad ? padJumpSprite_           : keyJumpSprite_          ).get();
        case Glyph::AvoidRolling:   return (isPad ? padAvoidRollingSprite_   : keyAvoidRollingSprite_  ).get();
        case Glyph::LockOn:         return (isPad ? padLockOnSprite_         : keyLockOnSprite_        ).get();
        case Glyph::Interact:       return (isPad ? padInteractSprite_       : keyInteractSprite_      ).get();
        }
        return nullptr;
    }

    const std::string& SwordManControlGuide::LabelText(const Label label) const
    {
        switch (label)
        {
        case Label::Move:                return moveLabel_;
        case Label::Attack:              return attackLabel_;
        case Label::DashAttack:          return dashAttackLabel_;
        case Label::JumpAttack:          return jumpAttackLabel_;
        case Label::ChargeAttackHold:    return chargeAttackHoldLabel_;
        case Label::ChargeAttackRelease: return chargeAttackReleaseLabel_;
        case Label::Run:                 return runLabel_;
        case Label::Jump:                return jumpLabel_;
        case Label::AvoidRolling:        return avoidRollingLabel_;
        case Label::LockOn:              return lockOnLabel_;
        case Label::LockOnRelease:       return lockOnReleaseLabel_;
        case Label::Chat:                return chatLabel_;
        case Label::WakeUp:              return wakeUpLabel_;
        case Label::CannonTurn:          return cannonTurnLabel_;
        case Label::CannonFire:          return cannonFireLabel_;
        }
        return moveLabel_;
    }

    void SwordManControlGuide::OnDrawGui()
    {
        ImGui::Text("guideAlpha_: %.2f", guideAlpha_);
        ImGui::Text("device_: %s", device_ == PlayerAvatarInputDevice::Gamepad ? "Gamepad" : "KeyboardMouse");

        ImGuiHelper::OnDrawInputField("rows_", rows_);
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("keyMoveSprite_", keyMoveSprite_);
        ImGuiHelper::OnDrawInputField("keyMoveHorizontalSprite_", keyMoveHorizontalSprite_);
        ImGuiHelper::OnDrawInputField("keyAttackSprite_", keyAttackSprite_);
        ImGuiHelper::OnDrawInputField("keyRunSprite_", keyRunSprite_);
        ImGuiHelper::OnDrawInputField("keyJumpSprite_", keyJumpSprite_);
        ImGuiHelper::OnDrawInputField("keyAvoidRollingSprite_", keyAvoidRollingSprite_);
        ImGuiHelper::OnDrawInputField("keyLockOnSprite_", keyLockOnSprite_);
        ImGuiHelper::OnDrawInputField("keyInteractSprite_", keyInteractSprite_);
        ImGuiHelper::OnDrawInputField("padMoveSprite_", padMoveSprite_);
        ImGuiHelper::OnDrawInputField("padMoveHorizontalSprite_", padMoveHorizontalSprite_);
        ImGuiHelper::OnDrawInputField("padAttackSprite_", padAttackSprite_);
        ImGuiHelper::OnDrawInputField("padRunSprite_", padRunSprite_);
        ImGuiHelper::OnDrawInputField("padJumpSprite_", padJumpSprite_);
        ImGuiHelper::OnDrawInputField("padAvoidRollingSprite_", padAvoidRollingSprite_);
        ImGuiHelper::OnDrawInputField("padLockOnSprite_", padLockOnSprite_);
        ImGuiHelper::OnDrawInputField("padInteractSprite_", padInteractSprite_);

        ImGuiHelper::OnDrawInputField("moveLabel_", moveLabel_);
        ImGuiHelper::OnDrawInputField("attackLabel_", attackLabel_);
        ImGuiHelper::OnDrawInputField("dashAttackLabel_", dashAttackLabel_);
        ImGuiHelper::OnDrawInputField("jumpAttackLabel_", jumpAttackLabel_);
        ImGuiHelper::OnDrawInputField("chargeAttackHoldLabel_", chargeAttackHoldLabel_);
        ImGuiHelper::OnDrawInputField("chargeAttackReleaseLabel_", chargeAttackReleaseLabel_);
        ImGuiHelper::OnDrawInputField("runLabel_", runLabel_);
        ImGuiHelper::OnDrawInputField("jumpLabel_", jumpLabel_);
        ImGuiHelper::OnDrawInputField("avoidRollingLabel_", avoidRollingLabel_);
        ImGuiHelper::OnDrawInputField("lockOnLabel_", lockOnLabel_);
        ImGuiHelper::OnDrawInputField("lockOnReleaseLabel_", lockOnReleaseLabel_);
        ImGuiHelper::OnDrawInputField("chatLabel_", chatLabel_);
        ImGuiHelper::OnDrawInputField("wakeUpLabel_", wakeUpLabel_);
        ImGuiHelper::OnDrawInputField("cannonTurnLabel_", cannonTurnLabel_);
        ImGuiHelper::OnDrawInputField("cannonFireLabel_", cannonFireLabel_);

        ImGuiHelper::OnDrawInputField("slideDistance_px_", slideDistance_px_);
        ImGuiHelper::OnDrawInputField("guideFadeDuration_secs_", guideFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("rowFadeDuration_secs_", rowFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("pulseDuration_secs_", pulseDuration_secs_);
        ImGuiHelper::OnDrawInputField("accentGlowMaxAlpha_", accentGlowMaxAlpha_);
        ImGuiHelper::OnDrawInputField("glyphFlashMaxAlpha_", glyphFlashMaxAlpha_);
        ImGuiHelper::OnDrawInputField("dimAlpha_", dimAlpha_);
        ImGuiHelper::OnDrawInputField("labelShadowAlphaRate_", labelShadowAlphaRate_);
        ImGuiHelper::OnDrawInputField("focusFadeDuration_secs_", focusFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("focusPulsePeriod_secs_", focusPulsePeriod_secs_);
        ImGuiHelper::OnDrawInputField("focusArrowSwing_px_", focusArrowSwing_px_);
        ImGuiHelper::OnDrawInputField("focusGlyphFlashMaxAlpha_", focusGlyphFlashMaxAlpha_);
        ImGuiHelper::OnDrawInputField("unfocusedDimRate_", unfocusedDimRate_);
    }
}
