#pragma once
#include <memory>

#include "Npc_Behaviour_NodeFactory.h"
#include "vec2.hpp"
#include "../../../../../../Engine/Core/Object/IObject.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/Action/TickContext/Enemy_Behaviour_TickContext.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/TickStatus/TickStatus.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct TickContext;
}

namespace GameCore::Npc::Friendly::Behaviour
{
    enum class TickStatus;
}

struct ImVec2;
struct ImDrawList;

namespace Editor::Npc::Behaviour
{
    extern const ImVec2 NODE_SIZE;
    
    class NodeBase : public virtual Object::IObject
    {
    public:
        virtual ~NodeBase() override = default;
        
        [[nodiscard]] virtual GameCore::Npc::Enemy::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) = 0;
        [[nodiscard]] virtual GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) = 0;
        virtual void OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList, const std::weak_ptr<NodeBase>& ownPtr) = 0;
        virtual void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) = 0;
        [[nodiscard]] virtual const std::string& NodeName() const = 0;
        [[nodiscard]] glm::vec2&  PositionRef() { return position_; }
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        void ResetGuid();

    private:
        virtual void DoOnDrawGui() = 0;

        Guid guid_;
        glm::vec2 position_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<IObject>(this));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(position_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<IObject>(this));
            if (version >= 0) archive(CEREAL_NVP(guid_));
            if (version >= 0) archive(CEREAL_NVP(position_));
        }
#pragma endregion
    };
};

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::NodeBase, 0);
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::NodeBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, Editor::Npc::Behaviour::NodeBase);
#pragma endregion