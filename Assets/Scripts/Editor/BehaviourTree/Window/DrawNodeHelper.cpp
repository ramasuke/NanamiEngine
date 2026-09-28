#include "DrawNodeHelper.h"

#include <sstream>

#include "Engine/Module/Exception/Engine_Module_Exception.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Serialization/Engine_Module_Serialization.h"
#include "cereal/archives/portable_binary.hpp"
#include "Node/Npc_BehaviourNodeBase.h"
#include "Node/Npc_Behaviour_NodeFactory.h"
#include "../../Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h"
#include "../../Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionHeaders.h"

namespace
{
    std::stringstream s_copiedNodeBinary;
    bool s_hasCopiedNode = false;

    constexpr ImU32 K_SUCCESS_COLOR = IM_COL32(80 , 220, 80 , 255);
    constexpr ImU32 K_RUNNING_COLOR = IM_COL32(255, 210, 60 , 255);
    constexpr ImU32 K_FAILURE_COLOR = IM_COL32(220, 80 , 80 , 255);
    constexpr ImU32 K_ABORT_COLOR   = IM_COL32(160, 140, 220, 255);
}

void Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::CopyNode(const std::weak_ptr<NodeBase>& copyNode)
{
    const auto node = copyNode.lock();
    if (!node) return;

    s_copiedNodeBinary.str({});
    s_copiedNodeBinary.clear();

    {
        cereal::PortableBinaryOutputArchive archive(s_copiedNodeBinary);
        archive(node);
    }

    s_hasCopiedNode = true;
}

std::shared_ptr<Editor::Npc::Behaviour::NodeBase> Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::PasteNode()
{
    if (!s_hasCopiedNode)
        return nullptr;

    s_copiedNodeBinary.clear();
    s_copiedNodeBinary.seekg(0);

    std::shared_ptr<NodeBase> newNode;
    try
    {
        NanamiEngine::Module::Serialization::LoadPortableBinary(s_copiedNodeBinary, "BehaviourTree clipboard", [&newNode](cereal::PortableBinaryInputArchive& archive)
        {
            archive(newNode);
        });
    }
    catch (const NanamiEngine::Module::Exception::SerializationException& exception)
    {
        NanamiEngine::Module::LogError("DrawNodeHelper: ノードの貼り付けに失敗しました: " + std::string(exception.what()));
        return nullptr;
    }

    if (newNode) 
        newNode->ResetGuidRecursive();
    
    return newNode;
}

bool Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::HasCopiedNode()
{
    return s_hasCopiedNode;
}

std::optional<ImU32> Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::RuntimeStatusColor(const NodeBase& node)
{
    if (node.HasBeenTickedAsEnemy())
    {
        switch (node.LastEnemyTickStatus())
        {
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Success: return K_SUCCESS_COLOR;
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Running: return K_RUNNING_COLOR;
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Failure: return K_FAILURE_COLOR;
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Abort:   return K_ABORT_COLOR;
        default: return std::nullopt;
        }
    }

    if (node.HasBeenTickedAsFriendly())
    {
        switch (node.LastFriendlyTickStatus())
        {
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Success: return K_SUCCESS_COLOR;
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Running: return K_RUNNING_COLOR;
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Failure: return K_FAILURE_COLOR;
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Abort:   return K_ABORT_COLOR;
        default: return std::nullopt;
        }
    }

    return std::nullopt;
}
