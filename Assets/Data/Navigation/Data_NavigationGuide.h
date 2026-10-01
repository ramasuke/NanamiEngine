#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/Condition/Condition_ICondition.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto NAVIGATION_GUIDE_EXTENSION_LABEL = ".navGuide";

    /** @brief ステップの目的地の候補1つ。今のシーンに targetId_ の目的地があれば、この title_ を字幕に出す */
    struct NavigationTargetOption
    {
        [[serialize(0)]] std::string targetId_;
        [[serialize(0)]] std::string title_;
        /** @brief 目印の横に添える短い名前(「教官」) */
        [[serialize(0)]] std::string label_;

        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(targetId_));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(label_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(targetId_));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(label_));
        }
    };

    /** @brief 次にすることの1段。activeConditions_ を満たし doneConditions_ を満たさない間だけ出る */
    struct NavigationStep
    {
        [[serialize(0)]] std::string                         id_;
        /** @brief どの候補も今のシーンに無いときの字幕 */
        [[serialize(0)]] std::string                         title_;
        [[serialize(0)]] GameCore::Condition::Conditions     activeConditions_;
        [[serialize(0)]] GameCore::Condition::Conditions     doneConditions_;
        /** @brief 優先順。今のシーンで最初に見つかったものを使う */
        [[serialize(0)]] std::vector<NavigationTargetOption> targets_;

        [[nodiscard]] bool IsCurrent(const GameCore::Condition::ConditionContext& context) const;
        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(id_));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(activeConditions_));
            archive(CEREAL_NVP(doneConditions_));
            archive(CEREAL_NVP(targets_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(id_));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(activeConditions_));
            if (version >= 0) archive(CEREAL_NVP(doneConditions_));
            if (version >= 0) archive(CEREAL_NVP(targets_));
        }
    };

    /**
     * @brief 「次にすること」の段の並び。上から見て最初に当てはまる段を出す
     */
    class NavigationGuide final : public ScriptableObject
    {
    public:
        explicit NavigationGuide(const std::string& contentPath = "");

        [[nodiscard]] const std::vector<NavigationStep>& Steps() const { return steps_; }
        /** @return 当てはまる段が無ければ nullptr */
        [[nodiscard]] const NavigationStep* FindCurrent(const GameCore::Condition::ConditionContext& context) const;

    private:
        [[serialize(0)]] std::vector<NavigationStep> steps_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(steps_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(steps_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::NavigationTargetOption, 0);
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::NavigationStep, 0);
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::NavigationGuide, 0);
#pragma endregion
