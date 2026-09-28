#pragma once
#include <memory>
#include <optional>

#include "ImGuiHelper.h"

namespace Editor::Npc::Behaviour
{
    class NodeBase;
}

namespace Editor::Npc::Behaviour::DrawGraphEditorGuiHelper
{
    void CopyNode(const std::weak_ptr<NodeBase>& copyNode);
    std::shared_ptr<NodeBase> PasteNode();
    bool HasCopiedNode();

    std::optional<ImU32> RuntimeStatusColor(const NodeBase& node);
};
