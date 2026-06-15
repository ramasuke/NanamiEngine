#pragma once
#include "fwd.hpp"
#include "../../../../Module/Guid/Guid.h"
#include "detail/type_quat.hpp"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Core::Network
{
    struct SpawnNetworkObject final
    {
        int playerId_;

        Guid objectGuid_;
        glm::vec3 position_;
        glm::quat rotation_;
        NetworkObjectId networkObjectId_;


        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(playerId_));
            archive(CEREAL_NVP(objectGuid_));
            archive(CEREAL_NVP(position_));
            archive(CEREAL_NVP(rotation_));
            archive(CEREAL_NVP(networkObjectId_));
        }
    };
}