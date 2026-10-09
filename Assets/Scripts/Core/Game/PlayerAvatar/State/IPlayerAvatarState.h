#pragma once
namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarState
    {
    public:
        virtual ~IPlayerAvatarState() = default;
        
        virtual void OnEnter () = 0;

        virtual void OnUpdate() = 0;

        virtual void OnFixedUpdate() = 0;
        
        virtual void OnExit  () = 0;
    };
}
