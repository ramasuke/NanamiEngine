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
        //TODO:製作展葉の補助処理なので、本来は必要なし
        if (Transform().GetWorldPos().y < -100)
        {
            Transform().SetLocalPos(glm::vec3{0.0f, 100.0f, 0.0f});
        }

        
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

    void FloatingState::DoUpdate()
    {

    }

    void FloatingState::DoExit()
    {

    }
}
