#include "Prop_EventNoticeBoard.h"

#include <chrono>

#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../Ui/EventBoard/Model/QuestBoardModel.h"
#include "../../Ui/EventBoard/Model/QuestReadLog.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../Core/Game/PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../Core/Game/Story/Story_StoryProgress.h"

namespace GamePlay::Prop
{
    void EventNoticeBoard::OnStart()
    {
        ResolveHasUnread();
        ApplyIdleIcon();
    }

    void EventNoticeBoard::OnUpdate()
    {
        // NOTE: シーン開始時はまだプレイヤーがいないことがあるので、見つかるまで判定を先送りする
        if (isResolved_)
            return;

        ResolveHasUnread();
        if (isResolved_ && !isInteractable_)
            ApplyIdleIcon();
    }

    void EventNoticeBoard::OnInteractable()
    {
        isInteractable_ = true;
        if (const auto icon = chatIcon_.get())
            icon->Show(false, true, false);
    }

    void EventNoticeBoard::OnExitInteractable()
    {
        isInteractable_ = false;
        // 掲示板を見た後や、依頼が解放された後に離れたときに付け直す
        ResolveHasUnread();
        ApplyIdleIcon();
    }

    void EventNoticeBoard::OnInteract()
    {
        if (const auto prefab = eventBoardUiPrefab_.get())
            Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f));
    }

    const GameObject::Transform& EventNoticeBoard::InteractableTransform() const
    {
        return Transform();
    }

    void EventNoticeBoard::ResolveHasUnread()
    {
        const auto board = board_.get();
        if (!board)
        {
            hasUnread_  = false;
            isResolved_ = true;
            return;
        }

        const auto owner = GameCore::PlayerAvatar::Owner();
        if (!owner)
            return;

        const Ui::QuestBoardModel model(
            board->Quests(),
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()),
            &owner->PlayerStatus().Quest(),
            &owner->PlayerStatus().CompletedQuest(),
            &GameCore::Story::StoryProgress::Instance(),
            0);
        hasUnread_  = model.HasUnreadMainStory(Ui::QuestReadLog());
        isResolved_ = true;
    }

    void EventNoticeBoard::ApplyIdleIcon() const
    {
        if (const auto icon = chatIcon_.get())
            icon->Show(!hasUnread_, false, hasUnread_);
    }

    void EventNoticeBoard::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("eventBoardUiPrefab_", eventBoardUiPrefab_);
        ImGuiHelper::OnDrawInputField("chatIcon_", chatIcon_);
        ImGuiHelper::OnDrawInputField("board_", board_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::EventNoticeBoard);
#pragma endregion
