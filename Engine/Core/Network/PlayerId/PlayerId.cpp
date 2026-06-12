#include "PlayerId.h"

namespace NanamiEngine::Core::Network
{
    PlayerId::PlayerId(const int playerId)
        : playerId_(playerId)
    {
        
    }

    PlayerId PlayerId::Invalid()
    {
        return PlayerId(-1);
    }

    std::string PlayerId::ToString() const
    {
        return std::to_string(playerId_);        
    }
}
