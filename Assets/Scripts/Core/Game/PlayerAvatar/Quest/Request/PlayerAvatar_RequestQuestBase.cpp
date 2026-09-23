#include "PlayerAvatar_RequestQuestBase.h"

#include <algorithm>

#include "../Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::Request
{
    RequestQuestBase:: RequestQuestBase() = default;
    RequestQuestBase::~RequestQuestBase()
    {
        subscription_.Dispose();
    }

    void RequestQuestBase::StartQuest(const QuestContext& context)
    {
        records_ = &context.records;
        const int current = CurrentRecord(context.records);
        // ロードし直した受注中の依頼は、受注したときの基準をそのまま使う
        if (!startRecord_)
            startRecord_ = current;

        subscription_.Dispose();
        auto& completedQuests = context.completedQuests;
        subscription_ = ObserveRecord(context.records).Subscribe([this, &completedQuests](const int currentRecord)
        {
            CheckComplete(currentRecord, completedQuests);
        });

        // 受注の前から条件を満たしていることはないが、requiredCount_ が 0 以下の依頼書でもすぐ終わるように
        CheckComplete(current, completedQuests);
    }

    int RequestQuestBase::Progress() const
    {
        if (!records_ || !startRecord_)
            return 0;
        return (std::max)(0, CurrentRecord(*records_) - *startRecord_);
    }

    void RequestQuestBase::CheckComplete(const int currentRecord, ICompleteQuestGroup& completedQuests)
    {
        if (!startRecord_ || currentRecord - *startRecord_ < requiredCount_)
            return;

        // CompleteQuest の中で受注リストから外されて自分が破棄されるので、先に購読を切り、種別は写してから渡す。
        // これより後で this に触れない
        subscription_.Dispose();
        const auto type = questType_;
        completedQuests.CompleteQuest(type);
    }

    void RequestQuestBase::OnDrawGui()
    {
        DrawRewardGui();
        LibCore::ImGuiHelper::OnDrawEnumField("questType_", questType_, QUEST_TYPE_NAMES, PlayerAvatar::ToString);
        LibCore::ImGuiHelper::OnDrawInputField("requiredCount_", requiredCount_);
        requiredCount_ = (std::max)(requiredCount_, 1);
        DoDrawGui();

        if (startRecord_)
            ImGui::Text("Progress: %d / %d", Progress(), requiredCount_);
    }
}

CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::ITakeableQuest, GameCore::PlayerAvatar::Quest::Request::RequestQuestBase)
