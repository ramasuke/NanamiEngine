#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cereal/cereal.hpp"
#include "PlayerAvatar_QuestUnlockContext.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    class IQuestUnlockCondition
    {
    public:
        virtual ~IQuestUnlockCondition() = default;
        [[nodiscard]] virtual bool IsSatisfied(const QuestUnlockContext& context) const = 0;
        [[nodiscard]] virtual std::string Describe() const = 0;
        virtual void OnDrawGui() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const {}
        template<class Archive> void load(Archive& archive, const std::uint32_t version) {}
    };

    using QuestUnlockConditions = std::vector<std::shared_ptr<IQuestUnlockCondition>>;

    /** @brief 全部満たせば true。空なら true */
    [[nodiscard]] bool AreAllSatisfied(const QuestUnlockConditions& conditions, const QuestUnlockContext& context);

    /** @brief 種類を選んで足す・消す・中身を編集する GUI */
    void DrawQuestUnlockConditions(const std::string& label, QuestUnlockConditions& conditions);
    /** @brief 1つだけ持つ枠の GUI。空なら種類を選んで入れる */
    void DrawQuestUnlockCondition(const std::string& label, std::shared_ptr<IQuestUnlockCondition>& condition);
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition, 0)
