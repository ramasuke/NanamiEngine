#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cereal/cereal.hpp"
#include "Condition_ConditionContext.h"

namespace GameCore::Condition
{
    class ICondition
    {
    public:
        virtual ~ICondition() = default;
        [[nodiscard]] virtual bool IsSatisfied(const ConditionContext& context) const = 0;
        [[nodiscard]] virtual std::string Describe() const = 0;
        virtual void OnDrawGui() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const {}
        template<class Archive> void load(Archive& archive, const std::uint32_t version) {}
    };

    // NOTE: 保存データにはそのまま配列で書かれるので、クラスにせず vector のままにする
    using Conditions = std::vector<std::shared_ptr<ICondition>>;

    /** @brief Conditions と、条件を 1 つだけ持つ枠に対する判定と GUI */
    class ConditionList final
    {
    public:
        ConditionList() = delete;

        /** @brief 全部満たせば true。空なら true */
        [[nodiscard]] static bool AreAllSatisfied(const Conditions& conditions, const ConditionContext& context);

        /** @brief 種類を選んで足す・消す・中身を編集する GUI */
        static void DrawListGui(const std::string& label, Conditions& conditions);
        /** @brief 1つだけ持つ枠の GUI。空なら種類を選んで入れる */
        static void DrawSingleGui(const std::string& label, std::shared_ptr<ICondition>& condition);

    private:
        /** @return 選ばれた種類の新しい条件。選ばれなければ nullptr */
        [[nodiscard]] static std::shared_ptr<ICondition> DrawCreateCombo(const char* label);
        /** @return 消すボタンが押されたら true */
        static bool DrawConditionNode(const std::shared_ptr<ICondition>& condition);
    };
}

CEREAL_CLASS_VERSION(GameCore::Condition::ICondition, 0)
