#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    // NOTE: NPC の頭上アイコンの表示切替。各アイコンの演出は子の ChatIcon*Motion が受け持つ
    class BillBoardNpcChatIcon final : public Component::ComponentBase
    {
    public:
        void Show(
            bool chattableIcon,
            bool chattingIcon,
            bool surpriseIcon);
        void Hide();
        void OnChattable();
        void OnExitChattable();
        void BeginReactionSurprise();
        void EndReactionSurprise();
        // NOTE: 今の目的の相手なら驚きアイコンを出す。Show/Hide やリアクションとは別に持ち、隠れている間は出さない
        void SetObjectiveSurprise(bool enable);

    private:
        // NOTE: 頼まれた状態・リアクション・目的を合わせてアイコンに反映する
        void Apply();
        // NOTE: まだ誰も Show/Hide を呼んでいなければ、今の見た目を頼まれた状態とみなす
        void CaptureRequestedIfNeeded();

        bool isShow_ = true;
        bool isReactionSurprise_  = false;
        bool isObjectiveSurprise_ = false;
        bool hasRequested_        = false;
        bool requestedChattable_  = false;
        bool requestedChatting_   = false;
        bool requestedSurprise_   = false;

        [[serialize(0)]] FIELD(GameObject::IGameObject) chattableIcon_; 
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattingIcon_;
        [[serialize(1)]] FIELD(GameObject::IGameObject) surpriseIcon_;
        [[serialize(3)]] FIELD(Asset::UiSoundBankData) uiSounds_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(chattableIcon_));
            archive(CEREAL_NVP(chattingIcon_));
            archive(CEREAL_NVP(surpriseIcon_));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(chattableIcon_));
            if (version >= 0) archive(CEREAL_NVP(chattingIcon_));
            if (version >= 1) archive(CEREAL_NVP(surpriseIcon_));
            if (version >= 3) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::BillBoardNpcChatIcon, 5);
