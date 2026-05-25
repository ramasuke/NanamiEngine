#include "FloatingState.h"

#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../Idle/SwordManAvatarIdleState.h"

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
        
        if (Conditions().IsGround())
            OnChangeState<SwordManAvatarIdleState>();
    }

    void FloatingState::DoExit()
    {
        
    }
}
