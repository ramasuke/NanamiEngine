#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/NanamiUI/BillBoard3D/BillboardAnimation3D.h"
#include "Engine/Module/NanamiUI/BillBoard3D/DrawBillboard3D.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    class BillBoardNpcChatIcon final : public Component::ComponentBase,
                                       public LifeCycleCallback::IUpdatable
    {
    public:
        void Show(
            bool chattableIcon,
            bool chattingIcon,
            bool surpriseIcon);
        void Hide();
        void OnChattable();
        void OnExitChattable();
        /** @brief 攻撃された・ぶつかられた間だけビックリマークだけを出す。End で元の表示に戻す */
        void BeginReactionSurprise();
        void EndReactionSurprise();

    private:
        enum class IconMotion
        {
            Surprise,   // ゆっくり上下 + 定期的にコトッと傾いた直後に枠を光が走る
            Chattable,  // 下向きに弾む
            Chatting,   // 呼吸するように拡大縮小
        };

        // アイコンごとの演出状態（シリアライズしない）
        struct IconState
        {
            bool      isCaptured     = false;
            bool      wasEnabled     = false;
            float     shownTime_secs = 0.0f;
            glm::vec3 basePos        = {};
            glm::vec3 baseScale      = {};
            float     baseAngle      = 0.0f;
            // 表示された瞬間のポップ
            LibCore::Tween::TweenPlayer<float> popScale;
            LibCore::Tween::TweenPlayer<float> popAlpha;
            std::weak_ptr<NanamiUi::Billboard3D> billboard;
        };

        void OnUpdate() override;
        // リアクション中は実際の表示ではなく、戻す時の表示状態を書き換える
        void SetIconEnable(GameObject::IGameObject* icon, bool& reactionSaved, bool enable) const;
        void UpdateIcon(
            const std::shared_ptr<GameObject::IGameObject>& object,
            const std::shared_ptr<NanamiUi::BillboardAnimation3D>& rimGlow,
            IconState& state,
            IconMotion motion) const;

        bool isShow_ = true;
        bool isReactionSurprise_ = false;
        bool savedChattable_     = false;
        bool savedChatting_      = false;
        bool savedSurprise_      = false;
        IconState chattableState_;
        IconState chattingState_;
        IconState surpriseState_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattableIcon_; 
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattingIcon_;
        [[serialize(1)]] FIELD(GameObject::IGameObject) surpriseIcon_;
        [[serialize(2)]] FIELD(NanamiUi::BillboardAnimation3D) surpriseRimGlow_;
        [[serialize(3)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        // 表示された瞬間のポップ（拡大して少し行き過ぎて戻る + フェードイン）
        [[serialize(4)]] float popDuration_secs_          = 0.25f;
        [[serialize(4)]] float surpriseFloatAmplitude_    = 0.2f;
        [[serialize(4)]] float surpriseFloatSpeed_        = 2.0f;
        // 周期の先頭で枠を光が走り、周期の最後にコトッと傾く（傾いた直後に次の光が走る）
        [[serialize(4)]] float surpriseCycle_secs_        = 3.0f;
        [[serialize(4)]] float surpriseSweepDuration_secs_ = 0.6f;
        [[serialize(4)]] float surpriseTiltDuration_secs_ = 0.5f;
        [[serialize(4)]] float surpriseTiltAngle_         = 0.2f;
        [[serialize(4)]] float chattableBounceAmplitude_  = 0.12f;
        [[serialize(4)]] float chattableBounceSpeed_      = 4.0f;
        [[serialize(4)]] float chattingBreathScale_       = 0.05f;
        [[serialize(4)]] float chattingBreathPeriod_secs_ = 1.6f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(chattableIcon_));
            archive(CEREAL_NVP(chattingIcon_));
            archive(CEREAL_NVP(surpriseIcon_));
            archive(CEREAL_NVP(surpriseRimGlow_));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(popDuration_secs_));
            archive(CEREAL_NVP(surpriseFloatAmplitude_));
            archive(CEREAL_NVP(surpriseFloatSpeed_));
            archive(CEREAL_NVP(surpriseCycle_secs_));
            archive(CEREAL_NVP(surpriseSweepDuration_secs_));
            archive(CEREAL_NVP(surpriseTiltDuration_secs_));
            archive(CEREAL_NVP(surpriseTiltAngle_));
            archive(CEREAL_NVP(chattableBounceAmplitude_));
            archive(CEREAL_NVP(chattableBounceSpeed_));
            archive(CEREAL_NVP(chattingBreathScale_));
            archive(CEREAL_NVP(chattingBreathPeriod_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(chattableIcon_));
            if (version >= 0) archive(CEREAL_NVP(chattingIcon_));
            if (version >= 1) archive(CEREAL_NVP(surpriseIcon_));
            if (version >= 2) archive(CEREAL_NVP(surpriseRimGlow_));
            if (version >= 3) archive(CEREAL_NVP(uiSounds_));
            if (version >= 4) archive(CEREAL_NVP(popDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(surpriseFloatAmplitude_));
            if (version >= 4) archive(CEREAL_NVP(surpriseFloatSpeed_));
            if (version >= 4) archive(CEREAL_NVP(surpriseCycle_secs_));
            if (version >= 4) archive(CEREAL_NVP(surpriseSweepDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(surpriseTiltDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(surpriseTiltAngle_));
            if (version >= 4) archive(CEREAL_NVP(chattableBounceAmplitude_));
            if (version >= 4) archive(CEREAL_NVP(chattableBounceSpeed_));
            if (version >= 4) archive(CEREAL_NVP(chattingBreathScale_));
            if (version >= 4) archive(CEREAL_NVP(chattingBreathPeriod_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::BillBoardNpcChatIcon, 4);
