#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "Data_AnnouncementKind.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto ANNOUNCEMENT_EXTENSION_LABEL = ".announcement";

    /**
     * @brief 掲示板の「お知らせ」1件。掲載時刻は "YYYY-MM-DD HH:MM"(日本時間) で書き、それより前は出さない。
     */
    class Announcement final : public ScriptableObject
    {
    public:
        explicit Announcement(const std::string& contentPath = "");

        [[nodiscard]] AnnouncementKind                Kind     () const { return kind_;      }
        [[nodiscard]] const std::string&              Title    () const { return title_;     }
        [[nodiscard]] const std::vector<std::string>& BodyLines() const { return bodyLines_; }
        /** @return 書式が崩れていれば nullopt */
        [[nodiscard]] std::optional<std::chrono::sys_seconds> PostedTime() const;

    private:
        [[serialize(0)]] AnnouncementKind         kind_ = AnnouncementKind::Update;
        [[serialize(0)]] std::string              title_;
        [[serialize(0)]] std::string              postedAt_;
        [[serialize(0)]] std::vector<std::string> bodyLines_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(kind_));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(postedAt_));
            archive(CEREAL_NVP(bodyLines_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(kind_));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(postedAt_));
            if (version >= 0) archive(CEREAL_NVP(bodyLines_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::Announcement, 0);
#pragma endregion
