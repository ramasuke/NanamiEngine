#pragma once
#include <cstdint>
#include <string>

#include "cereal/cereal.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"

namespace GameCore::Navigation
{
    /** @brief シーンのコンテキストに並べる目的地。id はガイドの targetId_ と突き合わせる */
    struct NavigationTargetEntry
    {
        [[serialize(0)]] std::string                                              id_;
        [[serialize(0)]] FIELD(NanamiEngine::Module::GameObject::IGameObject)     object_;
        [[serialize(0)]] float                                                    markerHeight_ = 2.0f;

        /** @return 消すボタンが押されたら true */
        bool OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(id_));
            archive(CEREAL_NVP(object_));
            archive(CEREAL_NVP(markerHeight_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(id_));
            if (version >= 0) archive(CEREAL_NVP(object_));
            if (version >= 0) archive(CEREAL_NVP(markerHeight_));
        }
    };
}

CEREAL_CLASS_VERSION(GameCore::Navigation::NavigationTargetEntry, 0);
