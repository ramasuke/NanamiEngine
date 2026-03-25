#pragma once
namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarState
    {
    public:
        virtual ~IPlayerAvatarState() = default;
        
        ///Stateに入ったときの処理
        virtual void OnEnter () = 0;

        ///Stateの更新処理
        ///NOTE: 1フレームごとに呼ばれる
        virtual void OnUpdate() = 0;

        ///Stateから抜けたときの処理
        virtual void OnExit  () = 0;
    };
}
