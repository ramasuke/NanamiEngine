#include "FloatingState.h"

#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "../Idle/SwordManAvatarIdleState.h"
#include "../Run/SwordManAvatarRunState.h"
#include "../Walk/SwordManAvatarWalkState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void FloatingState::DoEnter()
    {
        
    }

    void FloatingState::DoUpdate()
    {
        //TODO:製作展葉の補助処理なので、本来は必要なし
        if (Transform().GetWorldPos().y < -100)
        {
            Transform().SetLocalPos(glm::vec3{0.0f, 100.0f, 0.0f});
        }

        if (Input().Move().IsUpdatePressed() && Input().Run().IsUpdatePressed())
            OnChangeState<SwordManAvatarRunState>();
        if (Input().Move().IsUpdatePressed())
            OnChangeState<SwordManAvatarWalkState>();
        if (Input().NormalAttack().IsPressed())
            OnChangeState<SwordManAvatarNormalAttackState>();
        if (Conditions().IsGround())
            OnChangeState<SwordManAvatarIdleState>();
    }

    void FloatingState::DoExit()
    {
        
    }
}
