#include "Ui_AssetUpdateTag.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr const char* ASSET_UPDATE_NOTE_OFFER        = "受け取ると自動で再起動します";
        constexpr const char* ASSET_UPDATE_NOTE_RELAUNCH     = "ゲームを再起動して荷を開けます";
        constexpr const char* ASSET_UPDATE_NOTE_MANUAL       = "ゲームを起動し直してください";
        constexpr const char* ASSET_UPDATE_WAIT_RECEIVING    = "このまま少しお待ちください";
        constexpr const char* ASSET_UPDATE_WAIT_UNPACKING    = "もうすぐ終わります";
        constexpr const char* ASSET_UPDATE_WARNING_FAILED    = "このままでは冒険に出られません";
        constexpr const char* ASSET_UPDATE_WARNING_TOO_OLD   = "新しい版のゲームが要ります";

        using LibCore::EaseType;
        using LibCore::Tween::Ease;
        using LibCore::Tween::Ms;

        /** @brief UTF-8 の1文字の長さ。壊れた先頭バイトは1バイトとして進める */
        size_t AssetUpdateCharLength(const unsigned char lead)
        {
            if (lead < 0x80) return 1;
            if ((lead & 0xE0) == 0xC0) return 2;
            if ((lead & 0xF0) == 0xE0) return 3;
            if ((lead & 0xF8) == 0xF0) return 4;
            return 1;
        }

        /**
         * @brief 半角を1、それ以外を2と数えて lineUnits ごとに改行する。maxLines に収まらなければ最後の行の末尾を … にする。
         * 荷札の幅に収めるため。tools/art/asset_update.py の wrap_units と同じ規則
         */
        std::string AssetUpdateWrap(const std::string& text, const int lineUnits, const int maxLines)
        {
            std::vector<std::string> lines(1);
            int used = 0;
            for (size_t i = 0; i < text.size();)
            {
                const size_t length = std::min(AssetUpdateCharLength(static_cast<unsigned char>(text[i])), text.size() - i);
                const int width = length == 1 ? 1 : 2;
                if (text[i] == '\n' || (used + width > lineUnits && used > 0))
                {
                    lines.emplace_back();
                    used = 0;
                    if (text[i] == '\n')
                    {
                        ++i;
                        continue;
                    }
                }
                lines.back().append(text, i, length);
                used += width;
                i += length;
            }

            if (maxLines > 0 && static_cast<int>(lines.size()) > maxLines)
            {
                lines.resize(static_cast<size_t>(maxLines));
                std::string& last = lines.back();
                // 最後の1文字を落として … を足す (UTF-8 の継続バイトは残さない)
                while (!last.empty() && (static_cast<unsigned char>(last.back()) & 0xC0) == 0x80)
                    last.pop_back();
                if (!last.empty())
                    last.pop_back();
                last += "…";
            }

            std::string joined;
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (i > 0)
                    joined += '\n';
                joined += lines[i];
            }
            return joined;
        }
    }

    std::string FormatAssetUpdateBytes(const std::uint64_t bytes)
    {
        if (bytes < 1024)
            return std::to_string(bytes) + " B";
        if (bytes < 1024 * 1024)
            return std::to_string(bytes / 1024) + " KB";

        char formatted[32] = {};
        std::snprintf(formatted, sizeof(formatted), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
        return formatted;
    }

    void AssetUpdateTagUi::OnStart()
    {
        EnsureStarted();
    }

    void AssetUpdateTagUi::EnsureStarted()
    {
        if (isStarted_)
            return;
        isStarted_ = true;

        if (const auto tag = tagRoot_.get())
            tagBasePos_ = tag->Transform().GetLocalPos();
        hoofPopTweens_.assign(hoofPrints_.size(), LibCore::Tween::TweenPlayer<float>{});
        HideStamps();

        // 最初は何も出さない。状態が決まってから Show* で開く
        if (phase_ == Phase::Hidden)
        {
            if (const auto root = visualRoot_.get())
                root->SetEnable(false);
        }
    }

    void AssetUpdateTagUi::ShowOffer(const AssetUpdateParcel& parcel)
    {
        Open("新しい荷が届きました", Body::Details);
        WriteParcel(parcel);
        if (const auto note = noteText_.get())
            note->SetText(ASSET_UPDATE_NOTE_OFFER);
        SetHints("受け取る", "あとで");
    }

    void AssetUpdateTagUi::ShowReceiving()
    {
        Open("荷を受け取っています", Body::Progress);
        targetProgress_ = 0.0f;
        displayedProgress_ = 0.0f;
        ApplyHoofPrints(0);
        if (const auto percent = percentText_.get())
            percent->SetText("0%");
        if (const auto amount = amountText_.get())
            amount->SetText("");
        if (const auto wait = waitText_.get())
            wait->SetText(ASSET_UPDATE_WAIT_RECEIVING);
        SetHints("", "");
    }

    void AssetUpdateTagUi::ShowUnpacking()
    {
        Open("荷を解いています", Body::Progress);
        targetProgress_ = 1.0f;
        if (const auto wait = waitText_.get())
            wait->SetText(ASSET_UPDATE_WAIT_UNPACKING);
        SetHints("", "");
    }

    void AssetUpdateTagUi::SetProgress(const float rate01, const std::string& amountText)
    {
        targetProgress_ = std::max(targetProgress_, std::clamp(rate01, 0.0f, 1.0f));
        if (const auto amount = amountText_.get())
            amount->SetText(amountText);
    }

    void AssetUpdateTagUi::ShowUndelivered(const std::string& error)
    {
        Open("荷が届きませんでした", Body::Failure);
        WriteFailure(ASSET_UPDATE_WARNING_FAILED, error);
        PressStamp(undeliveredStamp_);
        SetHints("もう一度", "あとで");
    }

    void AssetUpdateTagUi::ShowReceived(const AssetUpdateParcel& parcel, const bool canRelaunch)
    {
        Open("荷を受け取りました", Body::Details);
        WriteParcel(parcel);
        if (const auto note = noteText_.get())
            note->SetText(canRelaunch ? ASSET_UPDATE_NOTE_RELAUNCH : ASSET_UPDATE_NOTE_MANUAL);
        PressStamp(receivedStamp_);
        SetHints(canRelaunch ? "再起動する" : "閉じる", "");
    }

    void AssetUpdateTagUi::ShowWrongVersion(const std::string& error)
    {
        Open("荷を受け取れません", Body::Failure);
        WriteFailure(ASSET_UPDATE_WARNING_TOO_OLD, error);
        PressStamp(wrongVersionStamp_);
        SetHints("", "閉じる");
    }

    void AssetUpdateTagUi::Hide()
    {
        if (phase_ == Phase::Hidden || phase_ == Phase::Leaving)
            return;

        phase_ = Phase::Leaving;
        // 引っ込むのは降りるより速く、加速しながら上へ抜ける
        const float leaveSecs = dropDuration_secs_ * 0.7f;
        dropTween_.Play(tweeny::from(0.0f).to(-dropDistance_px_).during(Ms(leaveSecs)).via(Ease(EaseType::InQuad)));
        veilTween_.Play(tweeny::from(1.0f).to(0.0f).during(Ms(leaveSecs)));
        hasConfirm_ = false;
        hasCancel_ = false;
    }

    void AssetUpdateTagUi::SubscribeHintClicks(std::function<void()> onConfirm, std::function<void()> onCancel)
    {
        if (const auto button = confirmButton_.get())
        {
            button->OnClick().Subscribe([this, onConfirm](NanamiUi::MouseState)
            {
                if (IsShown() && hasConfirm_)
                    onConfirm();
            }).AddTo(this);
        }
        if (const auto button = cancelButton_.get())
        {
            button->OnClick().Subscribe([this, onCancel](NanamiUi::MouseState)
            {
                if (IsShown() && hasCancel_)
                    onCancel();
            }).AddTo(this);
        }
    }

    void AssetUpdateTagUi::Open(const std::string& headline, const Body body)
    {
        EnsureStarted();
        if (const auto root = visualRoot_.get())
            root->SetEnable(true);
        if (const auto text = headlineText_.get())
            text->SetText(headline);
        if (const auto root = detailsRoot_.get())
            root->SetEnable(body == Body::Details);
        if (const auto root = progressRoot_.get())
            root->SetEnable(body == Body::Progress);
        if (const auto root = failureRoot_.get())
            root->SetEnable(body == Body::Failure);
        HideStamps();

        // 出ている札の中身だけを書き換えるときは降ろし直さない
        if (phase_ == Phase::Hidden || phase_ == Phase::Leaving)
        {
            phase_ = Phase::Entering;
            dropTween_.Play(tweeny::from(-dropDistance_px_).to(0.0f)
                .during(Ms(dropDuration_secs_)).via(Ease(EaseType::OutBack, dropOvershoot_)));
            veilTween_.Play(tweeny::from(0.0f).to(1.0f).during(Ms(dropDuration_secs_)).via(Ease(EaseType::OutCubic)));
            UpdateMotion(0.0f);
        }
    }

    void AssetUpdateTagUi::SetHints(const std::string& confirmLabel, const std::string& cancelLabel)
    {
        hasConfirm_ = !confirmLabel.empty();
        hasCancel_ = !cancelLabel.empty();
        if (const auto hint = confirmHint_.get())
            hint->SetEnable(hasConfirm_);
        if (const auto hint = cancelHint_.get())
            hint->SetEnable(hasCancel_);
        if (const auto label = confirmLabel_.get(); label && hasConfirm_)
            label->SetText(confirmLabel);
        if (const auto label = cancelLabel_.get(); label && hasCancel_)
            label->SetText(cancelLabel);
    }

    void AssetUpdateTagUi::WriteParcel(const AssetUpdateParcel& parcel) const
    {
        if (const auto text = fileCountText_.get())
            text->SetText(std::to_string(parcel.fileCount) + " 件");
        if (const auto text = sizeText_.get())
            text->SetText(FormatAssetUpdateBytes(parcel.bytes));
        if (const auto text = versionText_.get())
            text->SetText(parcel.version.empty() ? "—" : parcel.version + " 版");
    }

    void AssetUpdateTagUi::WriteFailure(const std::string& warning, const std::string& error) const
    {
        if (const auto text = warningText_.get())
            text->SetText(warning);
        if (const auto text = errorText_.get())
            text->SetText(AssetUpdateWrap(error, errorLineUnits_, errorMaxLines_));
    }

    void AssetUpdateTagUi::PressStamp(const FIELD(NanamiUi::BlendImageRenderer)& stamp)
    {
        const auto renderer = stamp.get();
        if (!renderer)
            return;

        renderer->SetEnable(true);
        renderer->SetBlendRate(0);
        pressingStamp_ = renderer;
        stampBaseScale_ = renderer->Transform().GetLocalScale();
        stampScaleTween_.Play(tweeny::from(stampStartScale_).to(1.0f)
            .during(Ms(stampDuration_secs_)).via(Ease(EaseType::OutCubic)));
        // 朱は押す時間の前半で乗り切る
        stampAlphaTween_.Play(tweeny::from(0.0f).to(1.0f).during(Ms(stampDuration_secs_ * 0.5f)));
        UpdateStamp(0.0f);
    }

    void AssetUpdateTagUi::HideStamps()
    {
        // 押している途中の判子は大きさを戻してから隠す
        if (const auto pressing = pressingStamp_.lock(); pressing && stampScaleTween_.IsPlaying())
            pressing->Transform().SetLocalScale(stampBaseScale_);
        pressingStamp_.reset();
        stampScaleTween_.Stop();

        for (const auto* stamp : {&receivedStamp_, &undeliveredStamp_, &wrongVersionStamp_})
        {
            if (const auto renderer = stamp->get())
                renderer->SetEnable(false);
        }
    }

    void AssetUpdateTagUi::ApplyHoofPrints(const int litCount)
    {
        litHoofCount_ = litCount;
        for (size_t i = 0; i < hoofPrints_.size(); ++i)
        {
            const auto print = hoofPrints_[i].get();
            if (!print)
                continue;

            const bool isLit = static_cast<int>(i) < litCount;
            print->SetSprite(std::weak_ptr<Asset::SpriteFile>(isLit ? hoofFilledSprite_.get() : hoofEmptySprite_.get()));
            print->Transform().SetLocalScale(glm::vec3(1.0f));
            if (i < hoofPopTweens_.size())
                hoofPopTweens_[i].Stop();
        }
    }

    void AssetUpdateTagUi::OnUpdate()
    {
        if (phase_ == Phase::Hidden)
            return;

        const float deltaSecs = Time::DeltaTime();
        UpdateMotion(deltaSecs);
        if (phase_ == Phase::Hidden)
            return;

        UpdateProgress(deltaSecs);
        UpdateStamp(deltaSecs);
        UpdateHoofPops(deltaSecs);
    }

    void AssetUpdateTagUi::UpdateMotion(const float deltaSecs)
    {
        if (phase_ != Phase::Entering && phase_ != Phase::Leaving)
            return;

        const bool isFinished = dropTween_.Tick(deltaSecs);
        veilTween_.Tick(deltaSecs);
        const auto tag = tagRoot_.get();
        if (tag)
            tag->Transform().SetLocalPos(tagBasePos_ + glm::vec3(0.0f, dropTween_.Value(), 0.0f));
        ApplyVeil(veilTween_.Value());
        if (!isFinished)
            return;

        if (phase_ == Phase::Entering)
        {
            phase_ = Phase::Shown;
            return;
        }

        phase_ = Phase::Hidden;
        HideStamps();
        if (tag)
            tag->Transform().SetLocalPos(tagBasePos_);
        if (const auto root = visualRoot_.get())
            root->SetEnable(false);
    }

    void AssetUpdateTagUi::UpdateProgress(const float deltaSecs)
    {
        if (const auto root = progressRoot_.get(); !root || !root->IsEnable())
            return;

        // 実際の進みへ指数的に寄せ、後戻りはさせない (ロード画面と同じ)
        const float follow = 1.0f - std::exp(-std::max(progressFollowRate_, 0.01f) * deltaSecs);
        displayedProgress_ = std::max(displayedProgress_, displayedProgress_ + (targetProgress_ - displayedProgress_) * follow);
        if (targetProgress_ >= 1.0f && targetProgress_ - displayedProgress_ < 0.002f)
            displayedProgress_ = 1.0f;

        if (const auto percent = percentText_.get())
            percent->SetText(std::to_string(static_cast<int>(displayedProgress_ * 100.0f)) + "%");

        const int count = static_cast<int>(hoofPrints_.size());
        const int lit = std::clamp(static_cast<int>(displayedProgress_ * static_cast<float>(count) + 0.0001f), 0, count);
        for (int i = litHoofCount_; i < lit; ++i)
        {
            const auto print = hoofPrints_[static_cast<size_t>(i)].get();
            if (!print)
                continue;
            print->SetSprite(std::weak_ptr<Asset::SpriteFile>(hoofFilledSprite_.get()));
            if (static_cast<size_t>(i) < hoofPopTweens_.size())
            {
                hoofPopTweens_[static_cast<size_t>(i)].Play(tweeny::from(hoofPopScale_).to(1.0f)
                    .during(Ms(hoofPopDuration_secs_)).via(Ease(EaseType::OutCubic)));
            }
        }
        // 一度に何個灯っても蹄の音は 1 回
        if (lit > litHoofCount_)
            Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::HoofTick);
        litHoofCount_ = std::max(litHoofCount_, lit);
    }

    void AssetUpdateTagUi::UpdateStamp(const float deltaSecs)
    {
        const auto stamp = pressingStamp_.lock();
        if (!stamp || !stampScaleTween_.IsPlaying())
            return;

        // 降りてくる途中では押さない。札が止まってから押す
        if (phase_ == Phase::Entering)
            return;

        const bool isFinished = stampScaleTween_.Tick(deltaSecs);
        stampAlphaTween_.Tick(deltaSecs);
        stamp->Transform().SetLocalScale(stampBaseScale_ * stampScaleTween_.Value());
        stamp->SetBlendRate(static_cast<int>(255.0f * stampAlphaTween_.Value()));

        if (isFinished)
        {
            stamp->Transform().SetLocalScale(stampBaseScale_);
            stamp->SetBlendRate(255);
        }
    }

    void AssetUpdateTagUi::UpdateHoofPops(const float deltaSecs)
    {
        for (size_t i = 0; i < hoofPopTweens_.size() && i < hoofPrints_.size(); ++i)
        {
            auto& pop = hoofPopTweens_[i];
            if (!pop.IsPlaying())
                continue;

            pop.Tick(deltaSecs);
            if (const auto print = hoofPrints_[i].get())
                print->Transform().SetLocalScale(glm::vec3(pop.Value()));
        }
    }

    void AssetUpdateTagUi::ApplyVeil(const float rate) const
    {
        const float clamped = std::clamp(rate, 0.0f, 1.0f);
        if (const auto veil = veil_.get())
            veil->SetBlendRate(static_cast<int>(static_cast<float>(veilBlendRate_) * clamped));
        if (const auto veil = veilBlack_.get())
            veil->SetBlendRate(static_cast<int>(static_cast<float>(veilBlackBlendRate_) * clamped));
    }

    void AssetUpdateTagUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("tagRoot_", tagRoot_);
        ImGuiHelper::OnDrawInputField("veilBlack_", veilBlack_);
        ImGuiHelper::OnDrawInputField("veil_", veil_);
        ImGuiHelper::OnDrawInputField("headlineText_", headlineText_);
        ImGuiHelper::OnDrawInputField("detailsRoot_", detailsRoot_);
        ImGuiHelper::OnDrawInputField("fileCountText_", fileCountText_);
        ImGuiHelper::OnDrawInputField("sizeText_", sizeText_);
        ImGuiHelper::OnDrawInputField("versionText_", versionText_);
        ImGuiHelper::OnDrawInputField("noteText_", noteText_);
        ImGuiHelper::OnDrawInputField("progressRoot_", progressRoot_);
        ImGuiHelper::OnDrawInputField("hoofPrints_", hoofPrints_, [this]
        {
            if (ImGui::Button("Add Hoof Print"))
            {
                hoofPrints_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("hoofFilledSprite_", hoofFilledSprite_);
        ImGuiHelper::OnDrawInputField("hoofEmptySprite_", hoofEmptySprite_);
        ImGuiHelper::OnDrawInputField("percentText_", percentText_);
        ImGuiHelper::OnDrawInputField("amountText_", amountText_);
        ImGuiHelper::OnDrawInputField("waitText_", waitText_);
        ImGuiHelper::OnDrawInputField("failureRoot_", failureRoot_);
        ImGuiHelper::OnDrawInputField("warningText_", warningText_);
        ImGuiHelper::OnDrawInputField("errorText_", errorText_);
        ImGuiHelper::OnDrawInputField("receivedStamp_", receivedStamp_);
        ImGuiHelper::OnDrawInputField("undeliveredStamp_", undeliveredStamp_);
        ImGuiHelper::OnDrawInputField("wrongVersionStamp_", wrongVersionStamp_);
        ImGuiHelper::OnDrawInputField("confirmHint_", confirmHint_);
        ImGuiHelper::OnDrawInputField("confirmLabel_", confirmLabel_);
        ImGuiHelper::OnDrawInputField("confirmButton_", confirmButton_);
        ImGuiHelper::OnDrawInputField("cancelHint_", cancelHint_);
        ImGuiHelper::OnDrawInputField("cancelLabel_", cancelLabel_);
        ImGuiHelper::OnDrawInputField("cancelButton_", cancelButton_);
        ImGuiHelper::OnDrawInputField("veilBlendRate_", veilBlendRate_);
        ImGuiHelper::OnDrawInputField("veilBlackBlendRate_", veilBlackBlendRate_);
        ImGuiHelper::OnDrawInputField("dropDistance_px_", dropDistance_px_);
        ImGuiHelper::OnDrawInputField("dropDuration_secs_", dropDuration_secs_);
        ImGuiHelper::OnDrawInputField("stampDuration_secs_", stampDuration_secs_);
        ImGuiHelper::OnDrawInputField("stampStartScale_", stampStartScale_);
        ImGuiHelper::OnDrawInputField("hoofPopDuration_secs_", hoofPopDuration_secs_);
        ImGuiHelper::OnDrawInputField("hoofPopScale_", hoofPopScale_);
        ImGuiHelper::OnDrawInputField("progressFollowRate_", progressFollowRate_);
        ImGuiHelper::OnDrawInputField("errorLineUnits_", errorLineUnits_);
        ImGuiHelper::OnDrawInputField("errorMaxLines_", errorMaxLines_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("dropOvershoot_", dropOvershoot_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::AssetUpdateTagUi);
#pragma endregion
