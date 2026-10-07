#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../../../../Data/Item/Data_ItemData.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Item/ItemPouch.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/FileSystem/Directory/Directory.h"

namespace GamePlay::Debug
{
    namespace
    {
        using ItemData = NanamiEngine::Module::Asset::ItemData;

        void CollectItems(NanamiEngine::Core::FileSystem::Directory& directory, std::vector<std::shared_ptr<ItemData>>& items)
        {
            for (auto& file : directory.Files())
            {
                if (auto item = std::dynamic_pointer_cast<ItemData>(file.GetContent()))
                    items.push_back(std::move(item));
            }

            for (auto& child : directory.GetDirectories())
                CollectItems(child, items);
        }

        /** @brief Assets/ 以下の ItemData。初めて開いたときに一度だけ集める */
        const std::vector<std::shared_ptr<ItemData>>& Items()
        {
            static std::vector<std::shared_ptr<ItemData>> items;
            static bool isCollected = false;
            if (!isCollected)
            {
                CollectItems(NanamiEngine::Core::Application::ApplicationBase::AssetsDirectory(), items);
                std::ranges::sort(items, {}, [](const auto& item) { return item->DisplayName(); });
                isCollected = true;
            }
            return items;
        }

        void DrawMoney(GameCore::IPlayerAvatar& avatar)
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            using GameCore::StatusParameter::Money;

            auto& wallet = avatar.PlayerStatus().Wallet();
            Widgets::Label("所持金", std::to_string(wallet.Balance().Value()));

            static int amount = 5000;
            const int pressed = Widgets::ButtonRow("増やす", { "+100", "+1000", "+10000" });
            constexpr int PRESETS[] = { 100, 1000, 10000 };
            if (pressed >= 0)
            {
                wallet.Earn(Money(PRESETS[pressed]));
                avatar.SaveStatus();
            }

            Widgets::InputInt("任意の額", amount, 100);
            if (Widgets::Button("この額を増やす"))
            {
                wallet.Earn(Money(amount));
                avatar.SaveStatus();
            }
        }

        void DrawItems(GameCore::IPlayerAvatar& avatar)
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            const auto& items = Items();
            if (items.empty())
            {
                Widgets::Note("ItemData が見つからない。");
                return;
            }

            auto& pouch = avatar.PlayerStatus().Pouch();
            for (const auto& item : items)
            {
                ImGui::PushID(item.get());
                const std::string label = item->DisplayName() + " (" + std::to_string(pouch.CountOf(*item)) + ")";
                const int pressed = Widgets::ButtonRow(label, { "+1", "+10", "満タン" });
                if (pressed >= 0)
                {
                    const int count = pressed == 0 ? 1 : pressed == 1 ? 10 : pouch.ReceivableCount(*item);
                    if (count > 0 && pouch.Add(item, count) > 0)
                        avatar.SaveStatus();
                }
                ImGui::PopID();
            }
            Widgets::Note("持てる数を超えた分は入らない。");
        }

        void DrawMoneyAndItems()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            const auto avatar = GameCore::PlayerAvatar::Owner();
            if (!avatar)
            {
                Widgets::Note("プレイヤーがいない。ステージに入ってから使う。");
                return;
            }

            Widgets::Header("所持金");
            DrawMoney(*avatar);

            Widgets::Header("アイテム");
            DrawItems(*avatar);
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(CheatMoneyAndItems, "チート/所持金・アイテム", 30, GamePlay::Debug::DrawMoneyAndItems)
#endif
