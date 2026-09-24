#include "EditorToolbarWidgetRegistry.h"

#include <algorithm>

#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Toolbar
{
    void EditorToolbarWidgetRegistry::DrawAll(EditorToolbarWidgetContext& context) const
    {
        for (const auto& entry : entries_)
        {
            if (!entry.widget->IsVisible())
                continue;
            entry.widget->OnDraw(context);
            ImGui::SameLine();
        }
    }

    void EditorToolbarWidgetRegistry::Add(Entry entry)
    {
        const auto position = std::ranges::upper_bound(entries_, entry, [](const Entry& lhs, const Entry& rhs)
        {
            return lhs.order != rhs.order ? lhs.order < rhs.order : lhs.name < rhs.name;
        });
        entries_.insert(position, std::move(entry));
    }
}
