#pragma once
#include <typeindex>

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarEventSceneStateMachine
    {
    public:
        virtual ~IPlayerAvatarEventSceneStateMachine() = default;
        ///@brief Stateの変更
        virtual void OnChangeState(std::type_index type) = 0;
        ///@brief Stateが行う動作を有効化
        virtual void OnEnable()  = 0;
        ///@brief Stateが行う動作を無効化
        virtual void OnDisable() = 0;
    };
}
