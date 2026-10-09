#pragma once
#include <optional>
#include <string>

#include "Ui_NavigationObjective.h"
#include "Libs/Singleton/LibCore_SingletonBase.h"

namespace GamePlay::Ui
{
    class NavigationMemory final : public SingletonBase<NavigationMemory>
    {
    public:
        // NOTE: 最後に字幕で出した目的文
        [[nodiscard]] const std::string& AnnouncedTitle() const { return announcedTitle_; }
        void SetAnnouncedTitle(std::string title) { announcedTitle_ = std::move(title); }

        // NOTE: 今の目的。当てはまる段が無いか、シーンの切り替え中なら空
        [[nodiscard]] const std::optional<NavigationObjective>& Current() const { return current_; }
        void SetCurrent(std::optional<NavigationObjective> current) { current_ = std::move(current); }

    private:
        std::string                        announcedTitle_;
        std::optional<NavigationObjective> current_;
    };
}
