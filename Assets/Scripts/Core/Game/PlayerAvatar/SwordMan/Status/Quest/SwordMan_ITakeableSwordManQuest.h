#pragma once
#include <cstdint>

#include "../rxcpp/rx.hpp"
#include "../cereal/include/cereal/cereal.hpp"
#include "../../../../StatusParameter/Money/Money.h"
#include "../../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct TickContext;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class IObservableStatusEvent;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct SwordManQuestContext;

    class ITakeableSwordManQuest
    {
    public:
        virtual ~ITakeableSwordManQuest() = default;
        virtual void StartQuest(const SwordManQuestContext& context) = 0;
        virtual void OnDrawGui() = 0;
        [[nodiscard]] virtual const PlayerAvatar::QuestType& QuestType() const = 0;
        
        /** @brief 達成時にプレイヤーへ入る額 */
        [[nodiscard]] const StatusParameter::Money& RewardMoney() const { return rewardMoney_; }

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { archive(CEREAL_NVP(rewardMoney_)); }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { if (version >= 1) archive(CEREAL_NVP(rewardMoney_)); }

    protected:
        void DrawRewardGui() { LibCore::ImGuiHelper::OnDrawInputField("rewardMoney_", rewardMoney_); }

    private:
        [[serialize(1)]] StatusParameter::Money rewardMoney_;
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest, 1)