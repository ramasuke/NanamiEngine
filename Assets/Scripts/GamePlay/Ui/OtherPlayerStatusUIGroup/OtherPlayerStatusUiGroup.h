#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../PlayerStatus/Ui_PlayerStatus.h"

namespace GamePlay::Ui
{
    class OtherPlayerStatusUiGroup final : public Component::ComponentBase
    {
    public:
        void AddPlayerStatus(const std::weak_ptr<PlayerStatus>& playerStatus);

    private:
        std::vector<std::weak_ptr<PlayerStatus>> playerStatuses_;
        [[serialize(0)]] glm::vec3 spacing_ = {};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(spacing_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(spacing_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::OtherPlayerStatusUiGroup, 0)
