#pragma once
#include <memory>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"

namespace GamePlay::Prop
{
    /** @brief 突進してきた敵の頭が刺さる障害物の目印 */
    class ChargeStuckObstacle final : public Component::ComponentBase
    {
    public:
        // NOTE: コライダーは子に付いているので、当たった GameObject から親をさかのぼって探す
        [[nodiscard]] static std::shared_ptr<ChargeStuckObstacle> FindFrom(GameObject::IGameObject& hitObject);

#pragma region Serialization Function
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}
