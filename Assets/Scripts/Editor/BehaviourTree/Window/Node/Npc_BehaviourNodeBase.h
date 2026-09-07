#pragma once
#include <memory>
#include <vector>

#include "Npc_Behaviour_NodeFactory.h"
#include "vec2.hpp"
#include "../../../../../../Engine/Core/Object/IObject.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/Action/TickContext/Enemy_Behaviour_TickContext.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/TickStatus/TickStatus.h"
#include "../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct TickContext;
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

        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context);
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context);
        virtual void OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList, const std::weak_ptr<NodeBase>& ownPtr) = 0;
        virtual void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) = 0;
        [[nodiscard]] virtual const std::string& NodeName() const = 0;
        [[nodiscard]] glm::vec2&  PositionRef() { return position_; }
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        void ResetGuid();

        // このノードがGraphEditor上で直接ぶら下げている子ノード。
        // 親ノードをドラッグ移動したときに子孫を追従させるために使用する。
        [[nodiscard]] virtual std::vector<std::shared_ptr<NodeBase>> Children() const { return {}; }

        // 実行時状態（シリアライズ対象外）。BehaviourTreeビューアがノードの色分け表示に使う。
        [[nodiscard]] bool HasBeenTickedAsEnemy() const { return hasBeenTickedAsEnemy_; }
        [[nodiscard]] bool HasBeenTickedAsFriendly() const { return hasBeenTickedAsFriendly_; }
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus LastEnemyTickStatus() const { return lastEnemyTickStatus_; }
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus LastFriendlyTickStatus() const { return lastFriendlyTickStatus_; }

    private:
        virtual void DoOnDrawGui() = 0;
        [[nodiscard]] virtual GameCore::Npc::Enemy::Behaviour::TickStatus DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) = 0;
        [[nodiscard]] virtual GameCore::Npc::Friendly::Behaviour::TickStatus DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) = 0;

        Guid guid_;
        glm::vec2 position_;

        bool hasBeenTickedAsEnemy_ = false;
        bool hasBeenTickedAsFriendly_ = false;
        GameCore::Npc::Enemy::Behaviour::TickStatus lastEnemyTickStatus_ = GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;
        GameCore::Npc::Friendly::Behaviour::TickStatus lastFriendlyTickStatus_ = GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;

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