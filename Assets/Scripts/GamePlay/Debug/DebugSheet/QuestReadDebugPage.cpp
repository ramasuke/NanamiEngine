#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include <cereal/types/string.hpp>
#include <cereal/types/unordered_set.hpp>
#include "../../../../Data/EventNotice/Data_BoardQuest.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_MainStoryQuestBase.h"
#include "../../Ui/EventBoard/Model/QuestReadLog.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"

namespace GamePlay::Debug
{
    namespace
    {
        using ReadGuids = std::unordered_set<std::string>;

        struct MainStoryBoardQuest
        {
            std::string guid;
            std::string title;
        };

        /** @brief .meta から guid_ を抜き出す。読めなければ空 */
        std::string ReadMetaGuid(const std::filesystem::path& metaPath)
        {
            std::ifstream stream(metaPath, std::ios::binary);
            const std::string text{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };

            static const std::regex GUID_PATTERN(R"re("guid_"\s*:\s*\{[^}]*?"value_"\s*:\s*"([0-9A-Fa-f-]+)")re");
            std::smatch match;
            return std::regex_search(text, match, GUID_PATTERN) ? match[1].str() : std::string();
        }

        /**
         * @brief .boardQuest のうちメインストーリーのものを集める
         * NOTE: 型でアセットを列挙する API が無いので、.meta の guid から実体を引く
         */
        std::vector<MainStoryBoardQuest> CollectMainStoryBoardQuests()
        {
            namespace fs = std::filesystem;

            const std::string metaSuffix = std::string(NanamiEngine::Module::Asset::BOARD_QUEST_EXTENSION_LABEL) + ".meta";
            std::vector<MainStoryBoardQuest> quests;
            std::error_code error;
            for (const auto& entry : fs::recursive_directory_iterator("Assets", error))
            {
                if (!entry.is_regular_file() || !entry.path().generic_string().ends_with(metaSuffix))
                    continue;

                const std::string guid = ReadMetaGuid(entry.path());
                if (guid.empty())
                    continue;

                const auto quest = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                    .Catch<NanamiEngine::Module::Asset::BoardQuest>(Guid(guid)).lock();
                if (!quest || !dynamic_cast<const GameCore::PlayerAvatar::MainStoryQuestBase*>(quest->Quest().get()))
                    continue;

                quests.push_back({ quest->GetGuid().Value(), quest->Title() });
            }

            std::ranges::sort(quests, {}, &MainStoryBoardQuest::title);
            return quests;
        }

        void DrawQuestRead()
        {
            namespace Widgets   = NanamiEngine::DebugSheet::Widgets;
            namespace LocalPrefs = NanamiEngine::Module::LocalPrefs;

            static std::vector<MainStoryBoardQuest> quests;
            static bool isCollected = false;
            if (!isCollected)
            {
                quests      = CollectMainStoryBoardQuests();
                isCollected = true;
            }

            Widgets::Note("掲示板で見たメインストーリーの依頼。未読で受付中のものがあると掲示板に「！」が出る。");
            Widgets::Note("掲示板のアイコンは、近づいて離れたときかシーンに入り直したときに変わる。");

            // NOTE: ゲーム側に手を入れずに済むよう、保存ファイルを直接書き換える (QuestReadLog は作るたびに読み直す)
            auto readGuids = LocalPrefs::LoadOrDefault<ReadGuids>(Ui::QUEST_READ_LOG_SAVE_KEY, ReadGuids());
            bool changed   = false;

            const int pressed = Widgets::ButtonRow("まとめて", { "全部未読", "全部既読" });
            if (pressed == 0)
            {
                readGuids.clear();
                changed = true;
            }
            else if (pressed == 1)
            {
                for (const auto& quest : quests)
                    readGuids.insert(quest.guid);
                changed = true;
            }

            Widgets::Header("既読");
            if (quests.empty())
                Widgets::Note("メインストーリーの依頼書が見つからない");

            ImGui::PushID("QuestRead");
            for (const auto& quest : quests)
            {
                bool isRead = readGuids.contains(quest.guid);
                ImGui::PushID(quest.guid.c_str());
                if (Widgets::Toggle(quest.title, isRead))
                {
                    if (isRead) readGuids.insert(quest.guid);
                    else        readGuids.erase(quest.guid);
                    changed = true;
                }
                ImGui::PopID();
            }
            ImGui::PopID();

            if (changed)
                LocalPrefs::Save(Ui::QUEST_READ_LOG_SAVE_KEY, readGuids);

            Widgets::Header("一覧");
            if (Widgets::Button("依頼書を探し直す"))
                isCollected = false;
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(QuestRead, "ストーリー/依頼の既読", 11, GamePlay::Debug::DrawQuestRead)
#endif
