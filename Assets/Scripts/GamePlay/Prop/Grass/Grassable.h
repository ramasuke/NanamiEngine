#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Prop
{
    // GrassField の配置モードで草を生やせる地面の目印。Raycast が当たるようコライダーと同じ GameObject に付ける
    class Grassable final : public Component::ComponentBase
    {
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::Grassable, 0)
