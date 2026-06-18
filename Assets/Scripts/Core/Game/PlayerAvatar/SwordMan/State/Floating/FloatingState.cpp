#include "FloatingState.h"

#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void FloatingState::DoEnter()
    {

    }

    void FloatingState::DoFixedUpdate()
    {
        
    }

    void FloatingState::DoUpdate()
    {
        if (Conditions().IsGround())
        {
            if (Input().NormalAttack().IsPressed())
                OnChangeState(SwordManAvatarStateType::NormalAttack);
            else if (Input().Move().IsUpdatePressed() && Input().Run().IsUpdatePressed())
                OnChangeState(SwordManAvatarStateType::Run);
            else if (Input().Move().IsUpdatePressed())
                OnChangeState(SwordManAvatarStateType::Walk);
            else 
                OnChangeState(SwordManAvatarStateType::Idle);
        }
    }

    void FloatingState::DoExit()
    {

    }
}
