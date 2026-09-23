#pragma once
#include <algorithm>

#include "Engine/Module/ScriptableObject/ScriptableObject.h"

namespace GameCore::Npc::Enemy
{
    class BehaviourTree;
}

namespace NanamiEngine::Module::Asset
{
    constexpr auto ENEMY_BEHAVIOUR_DATA_LABEL = ".enemyBehaviourData";
    
    class EnemyBehaviourFile final : public ScriptableObject
    {
    public:
        explicit EnemyBehaviourFile(const std::string& contentPath = "");

        /** EnemyBehaviourTreeのCopyを作成し取得する */
        [[nodiscard]] std::unique_ptr<GameCore::Npc::Enemy::BehaviourTree> OnLoadCopyContent() const;
        
    private:
        void OnDoubleClick () override;
        void OnSaveCallback() override;
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EnemyBehaviourFile, 0);
#pragma endregion
