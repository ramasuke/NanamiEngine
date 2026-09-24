#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "vec3.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    /** @brief 荷札の「荷の数・重さ・送り状」に書く値 */
    struct AssetUpdateParcel final
    {
        std::uint64_t fileCount = 0;
        std::uint64_t bytes     = 0;
        std::string   version;
    };

    /** @brief 1024 未満は B、1 MB 未満は KB、それ以上は小数1桁の MB */
    [[nodiscard]] std::string FormatAssetUpdateBytes(std::uint64_t bytes);

    /**
     * @brief タイトル画面のアセット更新「早馬の荷札」の見た目。
     * 画面を Backdrop で沈め、釘から麻紐で吊るした荷札に、状態ごとの文字と蹄の跡・朱の判子を出す。
     * 荷札・影・麻紐・「早馬便 荷札」の見出し・早馬の丸印は絵に焼いてあり、ここでは変わる所だけを書く。
     * 出るときは上から降りてきて、引っ込むときは上へ戻る。判子は押した大きさから縮んで残る。
     */
    class AssetUpdateTagUi final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable,
                                   public LifeCycleCallback::IUpdatable
    {
    public:
        /** @brief 更新がある: 荷の中身を書き、受け取るか尋ねる */
        void ShowOffer(const AssetUpdateParcel& parcel);
        /** @brief 受け取り中。進み具合は SetProgress で毎フレーム渡す */
        void ShowReceiving();
        /** @brief 受け取り終えて Assets/ へ入れている間 */
        void ShowUnpacking();
        void SetProgress(float rate01, const std::string& amountText);
        /** @brief 落とせなかった / 入れられなかった。「不着」を押す */
        void ShowUndelivered(const std::string& error);
        /** @brief 入れ終えた。「受領」を押し、再起動を促す。canRelaunch が false なら起動し直してもらう */
        void ShowReceived(const AssetUpdateParcel& parcel, bool canRelaunch);
        /** @brief ゲーム本体が古くて受け取れない。「版違い」を押す */
        void ShowWrongVersion(const std::string& error);
        /** @brief 上へ引っ込める */
        void Hide();

        [[nodiscard]] bool IsShown() const { return phase_ == Phase::Entering || phase_ == Phase::Shown; }
        [[nodiscard]] bool HasConfirm() const { return hasConfirm_; }
        [[nodiscard]] bool HasCancel() const { return hasCancel_; }
        void SubscribeHintClicks(std::function<void()> onConfirm, std::function<void()> onCancel);

    private:
        enum class Phase
        {
            Hidden,
            Entering,
            Shown,
            Leaving,
        };

        enum class Body
        {
            Details,
            Progress,
            Failure,
        };

        void OnStart () override;
        void OnUpdate() override;

        /** @brief 呼び出し側の OnStart が先に走って Show* されても、元の位置を取り損ねないように */
        void EnsureStarted();
        void Open(const std::string& headline, Body body);
        void SetHints(const std::string& confirmLabel, const std::string& cancelLabel);
        void WriteParcel(const AssetUpdateParcel& parcel) const;
        void WriteFailure(const std::string& warning, const std::string& error) const;
        void PressStamp(const FIELD(NanamiUi::BlendImageRenderer)& stamp);
        void HideStamps();
        void ApplyHoofPrints(int litCount);
        void UpdateMotion(float deltaSecs);
        void UpdateProgress(float deltaSecs);
        void UpdateStamp(float deltaSecs);
        void UpdateHoofPops(float deltaSecs);
        void ApplyVeil(float rate) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) tagRoot_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) veilBlack_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) veil_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) headlineText_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) detailsRoot_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) fileCountText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) sizeText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) versionText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) noteText_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) progressRoot_;
        [[serialize(0)]] std::vector<FIELD(Component::ImageRenderer)> hoofPrints_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) hoofFilledSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) hoofEmptySprite_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) percentText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) amountText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) waitText_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) failureRoot_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) warningText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) errorText_;

        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) receivedStamp_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) undeliveredStamp_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) wrongVersionStamp_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) confirmHint_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) confirmLabel_;
        [[serialize(0)]] FIELD(NanamiUi::Button) confirmButton_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) cancelHint_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) cancelLabel_;
        [[serialize(0)]] FIELD(NanamiUi::Button) cancelButton_;

        [[serialize(0)]] int   veilBlendRate_        = 235;
        [[serialize(0)]] int   veilBlackBlendRate_   = 120;
        [[serialize(0)]] float dropDistance_px_      = 1000.0f;
        [[serialize(0)]] float dropDuration_secs_    = 0.45f;
        [[serialize(0)]] float stampDuration_secs_   = 0.3f;
        [[serialize(0)]] float stampStartScale_      = 1.6f;
        [[serialize(0)]] float hoofPopDuration_secs_ = 0.2f;
        [[serialize(0)]] float hoofPopScale_         = 1.35f;
        [[serialize(0)]] float progressFollowRate_   = 6.0f;
        [[serialize(0)]] int   errorLineUnits_       = 34;
        [[serialize(0)]] int   errorMaxLines_        = 3;
        [[serialize(1)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        // 降りるときに行き過ぎてから戻る量 (OutBack)
        [[serialize(2)]] float dropOvershoot_ = 1.4f;

        bool  isStarted_ = false;
        Phase phase_ = Phase::Hidden;
        /** 札の基準位置からの縦のずれ */
        LibCore::Tween::TweenPlayer<float> dropTween_;
        LibCore::Tween::TweenPlayer<float> veilTween_;
        glm::vec3 tagBasePos_ = glm::vec3(0.0f);
        bool  hasConfirm_ = false;
        bool  hasCancel_  = false;

        float targetProgress_    = 0.0f;
        float displayedProgress_ = 0.0f;
        int   litHoofCount_      = 0;
        std::vector<LibCore::Tween::TweenPlayer<float>> hoofPopTweens_;

        std::weak_ptr<NanamiUi::BlendImageRenderer> pressingStamp_;
        glm::vec3 stampBaseScale_ = glm::vec3(1.0f);
        /** 押している途中だけ再生中 */
        LibCore::Tween::TweenPlayer<float> stampScaleTween_;
        LibCore::Tween::TweenPlayer<float> stampAlphaTween_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(tagRoot_));
            archive(CEREAL_NVP(veilBlack_));
            archive(CEREAL_NVP(veil_));
            archive(CEREAL_NVP(headlineText_));
            archive(CEREAL_NVP(detailsRoot_));
            archive(CEREAL_NVP(fileCountText_));
            archive(CEREAL_NVP(sizeText_));
            archive(CEREAL_NVP(versionText_));
            archive(CEREAL_NVP(noteText_));
            archive(CEREAL_NVP(progressRoot_));
            archive(CEREAL_NVP(hoofPrints_));
            archive(CEREAL_NVP(hoofFilledSprite_));
            archive(CEREAL_NVP(hoofEmptySprite_));
            archive(CEREAL_NVP(percentText_));
            archive(CEREAL_NVP(amountText_));
            archive(CEREAL_NVP(waitText_));
            archive(CEREAL_NVP(failureRoot_));
            archive(CEREAL_NVP(warningText_));
            archive(CEREAL_NVP(errorText_));
            archive(CEREAL_NVP(receivedStamp_));
            archive(CEREAL_NVP(undeliveredStamp_));
            archive(CEREAL_NVP(wrongVersionStamp_));
            archive(CEREAL_NVP(confirmHint_));
            archive(CEREAL_NVP(confirmLabel_));
            archive(CEREAL_NVP(confirmButton_));
            archive(CEREAL_NVP(cancelHint_));
            archive(CEREAL_NVP(cancelLabel_));
            archive(CEREAL_NVP(cancelButton_));
            archive(CEREAL_NVP(veilBlendRate_));
            archive(CEREAL_NVP(veilBlackBlendRate_));
            archive(CEREAL_NVP(dropDistance_px_));
            archive(CEREAL_NVP(dropDuration_secs_));
            archive(CEREAL_NVP(stampDuration_secs_));
            archive(CEREAL_NVP(stampStartScale_));
            archive(CEREAL_NVP(hoofPopDuration_secs_));
            archive(CEREAL_NVP(hoofPopScale_));
            archive(CEREAL_NVP(progressFollowRate_));
            archive(CEREAL_NVP(errorLineUnits_));
            archive(CEREAL_NVP(errorMaxLines_));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(dropOvershoot_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(tagRoot_));
            if (version >= 0) archive(CEREAL_NVP(veilBlack_));
            if (version >= 0) archive(CEREAL_NVP(veil_));
            if (version >= 0) archive(CEREAL_NVP(headlineText_));
            if (version >= 0) archive(CEREAL_NVP(detailsRoot_));
            if (version >= 0) archive(CEREAL_NVP(fileCountText_));
            if (version >= 0) archive(CEREAL_NVP(sizeText_));
            if (version >= 0) archive(CEREAL_NVP(versionText_));
            if (version >= 0) archive(CEREAL_NVP(noteText_));
            if (version >= 0) archive(CEREAL_NVP(progressRoot_));
            if (version >= 0) archive(CEREAL_NVP(hoofPrints_));
            if (version >= 0) archive(CEREAL_NVP(hoofFilledSprite_));
            if (version >= 0) archive(CEREAL_NVP(hoofEmptySprite_));
            if (version >= 0) archive(CEREAL_NVP(percentText_));
            if (version >= 0) archive(CEREAL_NVP(amountText_));
            if (version >= 0) archive(CEREAL_NVP(waitText_));
            if (version >= 0) archive(CEREAL_NVP(failureRoot_));
            if (version >= 0) archive(CEREAL_NVP(warningText_));
            if (version >= 0) archive(CEREAL_NVP(errorText_));
            if (version >= 0) archive(CEREAL_NVP(receivedStamp_));
            if (version >= 0) archive(CEREAL_NVP(undeliveredStamp_));
            if (version >= 0) archive(CEREAL_NVP(wrongVersionStamp_));
            if (version >= 0) archive(CEREAL_NVP(confirmHint_));
            if (version >= 0) archive(CEREAL_NVP(confirmLabel_));
            if (version >= 0) archive(CEREAL_NVP(confirmButton_));
            if (version >= 0) archive(CEREAL_NVP(cancelHint_));
            if (version >= 0) archive(CEREAL_NVP(cancelLabel_));
            if (version >= 0) archive(CEREAL_NVP(cancelButton_));
            if (version >= 0) archive(CEREAL_NVP(veilBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(veilBlackBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(dropDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(dropDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(stampDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(stampStartScale_));
            if (version >= 0) archive(CEREAL_NVP(hoofPopDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(hoofPopScale_));
            if (version >= 0) archive(CEREAL_NVP(progressFollowRate_));
            if (version >= 0) archive(CEREAL_NVP(errorLineUnits_));
            if (version >= 0) archive(CEREAL_NVP(errorMaxLines_));
            if (version >= 1) archive(CEREAL_NVP(uiSounds_));
            if (version >= 2) archive(CEREAL_NVP(dropOvershoot_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::AssetUpdateTagUi, 2);
