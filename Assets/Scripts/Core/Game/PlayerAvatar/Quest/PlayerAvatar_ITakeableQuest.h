#pragma once
#include <cstdint>
#include <memory>

#include "cereal/cereal.hpp"
#include "../../StatusParameter/Money/Money.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    struct QuestContext;
    
    class ITakeableQuest
    {
    public:
        virtual ~ITakeableQuest() = default;
        virtual void StartQuest(const QuestContext& context) = 0;
        virtual void OnDrawGui() = 0;
        [[nodiscard]] virtual const PlayerAvatar::QuestType& QuestType() const = 0;
        /** @brief true なら達成のたびに報酬を出し、達成済みとして残さない(何度でも受けられる) */
        [[nodiscard]] virtual bool IsRepeatable() const { return false; }

        [[nodiscard]] std::shared_ptr<ITakeableQuest> Clone() const;

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
