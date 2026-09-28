#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/Condition/Condition_BoardTime.h"
#include "../../Scripts/Core/Game/Condition/Condition_ICondition.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto EVENT_NOTICE_EXTENSION_LABEL = ".eventNotice";
    /** 告知の時刻は日本時間で書き、日本時間で表示する */
    constexpr std::chrono::hours EVENT_NOTICE_UTC_OFFSET = GameCore::Condition::BOARD_TIME_UTC_OFFSET;

    using GameCore::Condition::ParseBoardTime;
    using GameCore::Condition::DrawBoardTimeWarning;

    /**
     * @brief 掲示板に貼るイベント告知1件。開始・終了は "YYYY-MM-DD HH:MM"(日本時間)
     */
    class EventNotice final : public ScriptableObject
    {
    public:
        explicit EventNotice(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              Title           () const { return title_;            }
        [[nodiscard]] const std::string&              TagText         () const { return tagText_;          }
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        [[nodiscard]] std::shared_ptr<SpriteFile>     BannerSprite    () const { return bannerSprite_.get(); }

        /** @return 書式が崩れていれば nullopt */
        [[nodiscard]] std::optional<std::chrono::sys_seconds> StartTime() const;
        [[nodiscard]] std::optional<std::chrono::sys_seconds> EndTime  () const;
        /** @brief 書式が崩れていれば false */
        [[nodiscard]] bool IsOngoing(std::chrono::sys_seconds now) const;
        /** @brief unlockConditions_ を全部満たしていれば true。満たさない催しは掲示板に出さない */
        [[nodiscard]] bool IsUnlocked(const GameCore::Condition::ConditionContext& context) const;

    private:
        [[serialize(0)]] std::string              title_;
        [[serialize(0)]] std::string              tagText_;
        [[serialize(0)]] std::string              startAt_;
        [[serialize(0)]] std::string              endAt_;
        [[serialize(0)]] std::vector<std::string> descriptionLines_;
        [[serialize(0)]] FIELD(SpriteFile)        bannerSprite_;
        [[serialize(1)]] GameCore::Condition::Conditions unlockConditions_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(tagText_));
            archive(CEREAL_NVP(startAt_));
            archive(CEREAL_NVP(endAt_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(bannerSprite_));
            archive(CEREAL_NVP(unlockConditions_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(tagText_));
            if (version >= 0) archive(CEREAL_NVP(startAt_));
            if (version >= 0) archive(CEREAL_NVP(endAt_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(bannerSprite_));
            if (version >= 1) archive(CEREAL_NVP(unlockConditions_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EventNotice, 1);
#pragma endregion
