#pragma once
#include <functional>
#include <string>
#include <vector>

namespace GamePlay::Ui
{
    // NOTE: 設定画面の1行。値は choices の添字で読み書きする
    struct SettingsItem
    {
        std::string label;
        std::vector<std::string> choices;
        // NOTE: 選んでいる値の説明。choices と同じ並び
        std::vector<std::string> descriptions;
        std::function<int()> get;
        std::function<void(int)> set;
        // NOTE: 端で反対側へ回すか。音量のように大小がある値は false (端で止める)
        bool wrap = true;
    };

    // NOTE: 左の札1枚ぶんの項目
    struct SettingsCategory
    {
        std::string name;
        std::vector<SettingsItem> items;
    };

    // NOTE: 設定画面に並べるカテゴリと行。行を足すときはここに足す
    [[nodiscard]] const std::vector<SettingsCategory>& SettingsCatalog();
}
