#pragma once
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ImGuiHelper.h"

#include "Npc_Behaviour_NodeFactory.h"
#include "vec2.hpp"
#include "Engine/Core/Object/IObject.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/Action/TickContext/Enemy_Behaviour_TickContext.h"
#include "../../../../Core/Game/Npc/Enemy/Behaviour/TickStatus/TickStatus.h"
#include "../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct TickContext;
}

struct ImVec2;

namespace Editor::Npc::Behaviour
{
    extern const ImVec2 NODE_SIZE;

    // NOTE: 子ノードの繋がっていた位置。付け替えで同じ親に戻すときに順番と重みを保つ
    struct ChildSlot
    {
        std::size_t index  = 0;
        int         weight = 100;
    };

    class NodeBase : public virtual Object::IObject
    {
    public:
        // NOTE: MaxChildren の「制限なし」
        static constexpr std::size_t UNLIMITED_CHILDREN = std::numeric_limits<std::size_t>::max();

        virtual ~NodeBase() override = default;

        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context);
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context);
        // NOTE: 子を末尾に繋ぐ。子を 1 つしか持てないノードは置き換える
        virtual void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) = 0;
        [[nodiscard]] virtual const std::string& NodeName() const = 0;

        // NOTE: グラフエディタ用
        // NOTE: ノードの見出し。既定は NodeName()
        [[nodiscard]] virtual std::string GraphNodeTitle() const { return NodeName(); }
        // NOTE: ノード本文に出す補足（アクションの型など）。空なら何も出さない
        [[nodiscard]] virtual std::string GraphNodeDetail() const { return {}; }
        [[nodiscard]] virtual ImU32 GraphHeaderColor() const = 0;
        [[nodiscard]] virtual std::size_t MaxChildren() const { return 0; }
        // NOTE: 直接の子 child を外し、繋がっていた位置を返す。child が子でなければ nullopt
        virtual std::optional<ChildSlot> RemoveChild(const NodeBase* child) { return std::nullopt; }
        // NOTE: RemoveChild で外した子を元の位置に戻す（範囲外なら末尾）。既定は SetConnectToNextNode
        virtual void InsertChild(std::shared_ptr<NodeBase> child, const ChildSlot&) { SetConnectToNextNode(std::move(child)); }
        // NOTE: ノードの右クリックメニューに項目を足す（ActionNode のアクション型選択など）
        virtual void DrawGraphContextMenuItems() {}

        [[nodiscard]] glm::vec2&  PositionRef() { return position_; }
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        void ResetGuid();
        // NOTE: 自身と子孫すべての guid を振り直す（貼り付けで guid が重複しないように）
        void ResetGuidRecursive();

        // NOTE: GraphEditor 上で直接ぶら下げている子ノード
        [[nodiscard]] virtual std::vector<std::shared_ptr<NodeBase>> Children() const { return {}; }

        // NOTE: 自身と子孫の実行時状態（WaitSeconds の経過時間など）を初期化する
        void ResetRuntimeState();

        // NOTE: 直近の Tick の結果（シリアライズ対象外）。グラフ表示の色分け用
        [[nodiscard]] bool HasBeenTickedAsEnemy() const { return hasBeenTickedAsEnemy_; }
        [[nodiscard]] bool HasBeenTickedAsFriendly() const { return hasBeenTickedAsFriendly_; }
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus LastEnemyTickStatus() const { return lastEnemyTickStatus_; }
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus LastFriendlyTickStatus() const { return lastFriendlyTickStatus_; }

    private:
        virtual void DoOnDrawGui() = 0;
        [[nodiscard]] virtual GameCore::Npc::Enemy::Behaviour::TickStatus DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) = 0;
        [[nodiscard]] virtual GameCore::Npc::Friendly::Behaviour::TickStatus DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) = 0;
        virtual void DoResetRuntimeState() {}

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
#pragma endregion
