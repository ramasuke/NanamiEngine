#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../../../../Data/Decoration/Data_DecorationData.h"
#include "../../../Core/Game/Decoration/Decoration_DecorationCollection.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/FileSystem/Directory/Directory.h"

namespace GamePlay::Debug
{
    namespace
    {
        using DecorationData = NanamiEngine::Module::Asset::DecorationData;

        void CollectDecorations(NanamiEngine::Core::FileSystem::Directory& directory, std::vector<std::shared_ptr<DecorationData>>& decorations)
        {
            for (auto& file : directory.Files())
            {
                if (auto decoration = std::dynamic_pointer_cast<DecorationData>(file.GetContent()))
                    decorations.push_back(std::move(decoration));
            }

            for (auto& child : directory.GetDirectories())
                CollectDecorations(child, decorations);
        }

        /** @brief Assets/ 以下の DecorationData。初めて開いたときに一度だけ集める */
        const std::vector<std::shared_ptr<DecorationData>>& Decorations()
        {
            static std::vector<std::shared_ptr<DecorationData>> decorations;
            static bool isCollected = false;
            if (!isCollected)
            {
                CollectDecorations(NanamiEngine::Core::Application::ApplicationBase::AssetsDirectory(), decorations);
                std::ranges::sort(decorations, {}, [](const auto& decoration) { return decoration->Name(); });
                isCollected = true;
            }
            return decorations;
        }

        void DrawDecorations()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            auto& collection = GameCore::Decoration::DecorationCollection::Instance();
            Widgets::Note("切り替えるとその場で保存される。ConditionalObject の見た目もすぐ変わる。");

            const auto& decorations = Decorations();
            if (decorations.empty())
            {
                Widgets::Note("DecorationData が見つからない。");
                return;
            }

            for (const auto& decoration : decorations)
            {
                ImGui::PushID(decoration.get());
                bool isOwned = collection.IsOwned(decoration->GetGuid());
                if (Widgets::Toggle(decoration->Name(), isOwned))
                {
                    if (isOwned) collection.Add(decoration->GetGuid());
                    else         collection.Remove(decoration->GetGuid());
                }
                ImGui::PopID();
            }

            Widgets::Header("まとめて");
            if (Widgets::Button("全部得る"))
            {
                for (const auto& decoration : decorations)
                    collection.Add(decoration->GetGuid());
            }
            if (Widgets::Button("全部消す"))
            {
                for (const auto& decoration : decorations)
                    collection.Remove(decoration->GetGuid());
            }
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(Decorations, "ストーリー/島の飾り", 13, GamePlay::Debug::DrawDecorations)
#endif
