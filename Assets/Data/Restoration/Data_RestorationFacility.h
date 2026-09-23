#pragma once
#include <optional>
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/Story/Story_Facility.h"
#include "../../Scripts/Core/Game/Story/Story_StoryFlag.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto RESTORATION_FACILITY_EXTENSION_LABEL = ".restorationFacility";

    /**
     * @brief 掲示板の「復興」に貼る普請の見積もり1枚。お金を払うと facility_ が直ったことになる。
     * 前提(requiredStoryFlag_ / requiredFacility_)は -1 で「なし」。一覧は docs/Story.md §5
     * 建つ場所と姿は、シーンの同じ facility の RestorationGate が持つ(掲示板で選ぶとそこへカメラが寄る)
     */
    class RestorationFacility final : public ScriptableObject
    {
    public:
        explicit RestorationFacility(const std::string& contentPath = "");

        [[nodiscard]] GameCore::Story::Facility       Facility        () const { return static_cast<GameCore::Story::Facility>(facility_); }
        [[nodiscard]] const std::string&              Name            () const { return name_;             }
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        [[nodiscard]] int                             Cost            () const { return cost_;             }
        [[nodiscard]] std::optional<GameCore::Story::StoryFlag> RequiredStoryFlag() const;
        [[nodiscard]] std::optional<GameCore::Story::Facility>  RequiredFacility () const;
        /** @brief 前提を満たしていないときに出す文言 (例: 「草原の大顎を倒してから」) */
        [[nodiscard]] const std::string&              ConditionText   () const { return conditionText_;    }

    private:
        [[serialize(0)]] int                      facility_ = 0;
        [[serialize(0)]] std::string              name_;
        [[serialize(0)]] std::vector<std::string> descriptionLines_;
        [[serialize(0)]] int                      cost_ = 0;
        [[serialize(0)]] int                      requiredStoryFlag_ = -1;
        [[serialize(0)]] int                      requiredFacility_ = -1;
        [[serialize(0)]] std::string              conditionText_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(facility_));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(cost_));
            archive(CEREAL_NVP(requiredStoryFlag_));
            archive(CEREAL_NVP(requiredFacility_));
            archive(CEREAL_NVP(conditionText_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(facility_));
            if (version >= 0) archive(CEREAL_NVP(name_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(cost_));
            if (version >= 0) archive(CEREAL_NVP(requiredStoryFlag_));
            if (version >= 0) archive(CEREAL_NVP(requiredFacility_));
            if (version >= 0) archive(CEREAL_NVP(conditionText_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::RestorationFacility, 0);
#pragma endregion
