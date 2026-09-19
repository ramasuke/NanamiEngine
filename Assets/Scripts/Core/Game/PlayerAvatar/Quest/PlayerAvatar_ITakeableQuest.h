#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"
#include "../../StatusParameter/Money/Money.h"
#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    struct QuestContext;

    /// 職業を問わないクエストの口。剣士専用の ITakeableSwordManQuest と同じ形
    class ITakeableQuest
    {
    public:
        virtual ~ITakeableQuest() = default;
        virtual void StartQuest(const QuestContext& context) = 0;
        virtual void OnDrawGui() = 0;
        [[nodiscard]] virtual const PlayerAvatar::QuestType& QuestType() const = 0;

        /** @brief 達成時にプレイヤーへ入る額 */
        [[nodiscard]] const StatusParameter::Money& RewardMoney() const { return rewardMoney_; }

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { archive(CEREAL_NVP(rewardMoney_)); }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { if (version >= 0) archive(CEREAL_NVP(rewardMoney_)); }

    protected:
        void DrawRewardGui() { LibCore::ImGuiHelper::OnDrawInputField("rewardMoney_", rewardMoney_); }

    private:
        [[serialize(0)]] StatusParameter::Money rewardMoney_;
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::ITakeableQuest, 0)
