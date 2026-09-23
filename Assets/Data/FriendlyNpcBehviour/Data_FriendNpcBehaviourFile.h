#pragma once
#include "Engine/Module/ScriptableObject/ScriptableObject.h"

namespace GameCore::Npc::Friendly
{
    class BehaviourTree;
}

namespace NanamiEngine::Module::Asset
{
    constexpr auto FRIENDLY_NPC_BEHAVIOUR_DATA_EXTENSION_LABEL = ".friendBehaviourData";
    
    class FriendNpcBehaviourFile final : public ScriptableObject
    {
    public:
        explicit FriendNpcBehaviourFile(const std::string& contentPath = "");

        /** EnemyBehaviourTreeのCopyを作成し取得する */
        [[nodiscard]] std::unique_ptr<GameCore::Npc::Friendly::BehaviourTree> OnLoadCopyContent() const;
        
    private:
        void OnDoubleClick () override;
        void OnSaveCallback() override;
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::FriendNpcBehaviourFile, 0);
#pragma endregion
