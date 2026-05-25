#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Data/FriendlyNpcBehviour/Data_FriendNpcBehaviourFile.h"
#include "../../../../Data/FriendlyNpcStatus/Base/Data_FriendlyNpcBaseStatus.h"
#include "../../../Core/Game/Npc/Friendly/IFriendlyNpc.h"
#include "../../../Core/Game/PlayerAvatar/Chattable/IPlayerChattable.h"
#include "../../Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"
#include "../../Ui/NpcChatting/Ui_NpcChatting.h"

namespace GamePlay::Npc::Friendly
{
    class FriendlyNpc final : public Component::ComponentBase,
                              public LifeCycleCallback::IAwakable,
                              public LifeCycleCallback::IUpdatable,
                              public GameCore::PlayerAvatar::IPlayerChattable,
                              public GameCore::Npc::IFriendlyNpc
    {
    public:
        explicit FriendlyNpc();
        ~FriendlyNpc() override;
        
    private:
        void OnAwake        () override;
        void OnUpdate       () override;
        void OnChattable    () override;
        void OnExitChattable() override;
        void OnChat         () override;
        [[nodiscard]] const GameObject::Transform& ChattableTransform() const override;

        [[serialize(0)]] std::string name_;
        [[serialize(0)]] FIELD(Asset::FriendNpcBehaviourFile) friendlyNpcBehaviourFile_;
        [[serialize(2)]] FIELD(Asset::FriendlyNpcResources) baseStatus_;
        [[serialize(6)]] FIELD(Ui::BillBoardNpcChatIcon) billboardNpcChatIcon_;
        
        std::unique_ptr<GameCore::Npc::Friendly::BehaviourTree> behaviour_;
        bool isChatting_  = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(friendlyNpcBehaviourFile_));
            archive(CEREAL_NVP(baseStatus_));
            archive(CEREAL_NVP(billboardNpcChatIcon_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(name_));
            if (version >= 0) archive(CEREAL_NVP(friendlyNpcBehaviourFile_));
            if (version >= 2) archive(CEREAL_NVP(baseStatus_));
            if (version >= 6) archive(CEREAL_NVP(billboardNpcChatIcon_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Npc::Friendly::FriendlyNpc, 6)
CEREAL_REGISTER_TYPE(GamePlay::Npc::Friendly::FriendlyNpc) 
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Npc::Friendly::FriendlyNpc)