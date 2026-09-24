#pragma once
#include <string>
#include <unordered_set>

namespace GamePlay::Ui
{
    constexpr auto QUEST_READ_LOG_SAVE_KEY = "ReadMainStoryQuests";

    /**
     * @brief 掲示板で見たメインストーリーの依頼書の guid。キャラクターに関わらずこのPCで一度見れば既読にする
     */
    class QuestReadLog final
    {
    public:
        QuestReadLog();

        [[nodiscard]] bool IsRead(const std::string& guid) const;
        /** @return 新しく既読にしたら true */
        bool MarkRead(const std::string& guid);

    private:
        std::unordered_set<std::string> readGuids_;
    };
}
