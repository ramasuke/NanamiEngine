#include "SettingsCatalog.h"

#include "../../../../Core/Game/Settings/GameSettings.h"

namespace GamePlay::Ui
{
    namespace
    {
        /** @brief 0..VOLUME_STEPS の音量行。説明はどの値でも同じ */
        SettingsItem VolumeItem(std::string label, const std::string& description, std::function<int()> get, std::function<void(int)> set)
        {
            constexpr int steps = GameCore::GameSettings::VOLUME_STEPS;
            SettingsItem item{ .label = std::move(label), .get = std::move(get), .set = std::move(set), .wrap = false };
            for (int i = 0; i <= steps; ++i)
            {
                item.choices.push_back(std::to_string(i));
                item.descriptions.push_back(description);
            }
            return item;
        }
    }

    const std::vector<SettingsCategory>& SettingsCatalog()
    {
        using GameCore::ChatAdvanceMode;
        using GameCore::GameSettings;

        static const std::vector<SettingsCategory> catalog = {
            SettingsCategory{
                .name = "ゲーム",
                .items = {
                    SettingsItem{
                        .label = "会話の送り方",
                        .choices = {"自分で送る", "自動で送る"},
                        .descriptions = {"ボタンを押すまで、次の言葉を待ちます。", "読み終えたころに、次の言葉へ進みます。"},
                        .get = []
                        {
                            return GameSettings::GetInstance().GetChatAdvanceMode() == ChatAdvanceMode::Manual ? 0 : 1;
                        },
                        .set = [](const int index)
                        {
                            GameSettings::GetInstance().SetChatAdvanceMode(index == 0 ? ChatAdvanceMode::Manual : ChatAdvanceMode::Auto);
                        },
                    },
                },
            },
            SettingsCategory{
                .name = "音",
                .items = {
                    VolumeItem("全体の音量", "聞こえる音すべての大きさです。",
                        [] { return GameSettings::GetInstance().GetMasterVolume(); },
                        [](const int step) { GameSettings::GetInstance().SetMasterVolume(step); }),
                    VolumeItem("音楽の音量", "流れている曲の大きさです。",
                        [] { return GameSettings::GetInstance().GetBgmVolume(); },
                        [](const int step) { GameSettings::GetInstance().SetBgmVolume(step); }),
                    VolumeItem("効果音の音量", "足音や剣の音、魔物の声などの大きさです。",
                        [] { return GameSettings::GetInstance().GetSeVolume(); },
                        [](const int step) { GameSettings::GetInstance().SetSeVolume(step); }),
                },
            },
        };
        return catalog;
    }
}
