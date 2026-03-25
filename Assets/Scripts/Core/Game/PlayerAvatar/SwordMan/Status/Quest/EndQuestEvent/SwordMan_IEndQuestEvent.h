#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class IEndQuestEvent 
    {
    public:
        virtual ~IEndQuestEvent() = default;
        
        //Questが終了した時に呼ばれる関数
        virtual void OnEndQuest() = 0;
        virtual void DoDrawGui () = 0;

#pragma region Serialization Function
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version) { }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::Quest::IEndQuestEvent, 0)