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

    // NOTE: 保存データにはそのまま配列で書かれるので、クラスにせず vector のままにする
    using QuestUnlockConditions = std::vector<std::shared_ptr<IQuestUnlockCondition>>;

    /** @brief QuestUnlockConditions と、条件を 1 つだけ持つ枠に対する判定と GUI */
    class QuestUnlockConditionList final
    {
    public:
        QuestUnlockConditionList() = delete;

        /** @brief 全部満たせば true。空なら true */
        [[nodiscard]] static bool AreAllSatisfied(const QuestUnlockConditions& conditions, const QuestUnlockContext& context);

        /** @brief 種類を選んで足す・消す・中身を編集する GUI */
        static void DrawListGui(const std::string& label, QuestUnlockConditions& conditions);
        /** @brief 1つだけ持つ枠の GUI。空なら種類を選んで入れる */
        static void DrawSingleGui(const std::string& label, std::shared_ptr<IQuestUnlockCondition>& condition);

    private:
        /** @return 選ばれた種類の新しい条件。選ばれなければ nullptr */
        [[nodiscard]] static std::shared_ptr<IQuestUnlockCondition> DrawCreateCombo(const char* label);
        /** @return 消すボタンが押されたら true */
        static bool DrawConditionNode(const std::shared_ptr<IQuestUnlockCondition>& condition);
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition, 0)
