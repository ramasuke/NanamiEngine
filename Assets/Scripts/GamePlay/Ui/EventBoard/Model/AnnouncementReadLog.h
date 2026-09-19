#pragma once
#include <string>
#include <unordered_set>

namespace GamePlay::Ui
{
    constexpr auto ANNOUNCEMENT_READ_LOG_SAVE_KEY = "ReadAnnouncements";

    /**
     * @brief 読んだお知らせの guid。キャラクターに関わらずこのPCで一度読めば既読にする
     */
    class AnnouncementReadLog final
    {
    public:
        AnnouncementReadLog();

        [[nodiscard]] bool IsRead(const std::string& guid) const;
        /** @return 新しく既読にしたら true */
        bool MarkRead(const std::string& guid);

    private:
        std::unordered_set<std::string> readGuids_;
    };
}
