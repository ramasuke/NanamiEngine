#pragma once
#include <memory>
#include <string>

#include "Condition_ICondition.h"

namespace GameCore::Condition
{
    class ConditionList final
    {
    public:
        ConditionList() = delete;

        // NOTE: 全部満たせば true。空なら true
        [[nodiscard]] static bool AreAllSatisfied(const Conditions& conditions, const ConditionContext& context);

        static void DrawListGui(const std::string& label, Conditions& conditions);
        static void DrawSingleGui(const std::string& label, std::shared_ptr<ICondition>& condition);

    private:
        [[nodiscard]] static std::shared_ptr<ICondition> DrawCreateCombo(const char* label);
        // NOTE: 消すボタンが押されたら true を返す
        static bool DrawConditionNode(const std::shared_ptr<ICondition>& condition);
    };
}
