#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../Data/EventNotice/Data_EventBoard.h"
#include "../../../Core/Game/PlayerAvatar/Interactable/IPlayerInteractable.h"
#include "../../Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"

namespace GamePlay::Prop
{
    /**
     * @brief 拠点のイベント掲示板。近づくとアイコンが変わり、調べると告知一覧のUIを出す。
     * 一覧の中身はUIプレハブ側が .eventBoard から読むので、ここは開くだけ。
     * board_ にまだ見ていない受付中のメインストーリーの依頼があれば、離れている間はビックリマークを出す。
     */
    class EventNoticeBoard final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable,
                                   public LifeCycleCallback::IUpdatable,
                                   public GameCore::PlayerAvatar::IPlayerInteractable
    {
    private:
        void OnStart           () override;
        void OnUpdate          () override;
        void OnInteractable    () override;
        void OnExitInteractable() override;
        void OnInteract        () override;
        [[nodiscard]] const GameObject::Transform& InteractableTransform() const override;
        /** @brief プレイヤーがまだいなくて判定できなければ isResolved_ を立てない */
        void ResolveHasUnread();
        void ApplyIdleIcon() const;

        bool isResolved_     = false;
        bool hasUnread_      = false;
        bool isInteractable_ = false;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) eventBoardUiPrefab_;
        [[serialize(0)]] FIELD(Ui::BillBoardNpcChatIcon) chatIcon_;
        [[serialize(1)]] FIELD(Asset::EventBoardData) board_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(eventBoardUiPrefab_));
            archive(CEREAL_NVP(chatIcon_));
            archive(CEREAL_NVP(board_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(eventBoardUiPrefab_));
            if (version >= 0) archive(CEREAL_NVP(chatIcon_));
            if (version >= 1) archive(CEREAL_NVP(board_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::EventNoticeBoard, 1);
