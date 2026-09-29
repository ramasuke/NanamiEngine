#pragma once
#include <memory>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "../../../Core/Game/Condition/Condition_ICondition.h"

namespace GamePlay::Prop
{
    class ConditionalObject final : public Component::ComponentBase,
                                    public LifeCycleCallback::IStartable
    {
    private:
        void OnStart() override;
        void Apply();

        [[serialize(0)]] GameCore::Condition::Conditions conditions_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) target_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) prefab_;

        std::weak_ptr<GameObject::IGameObject> spawned_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(conditions_));
            archive(CEREAL_NVP(target_));
            archive(CEREAL_NVP(prefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(conditions_));
            if (version >= 0) archive(CEREAL_NVP(target_));
            if (version >= 0) archive(CEREAL_NVP(prefab_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::ConditionalObject, 0);
