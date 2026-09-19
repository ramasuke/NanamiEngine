#pragma once
#include "../../../Status/Presenter/PlayerAvatar_StatusPresenterBase.h"
#include "../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarStatus;
}

namespace GamePlay::Ui
{
    class PlayerStatus;
}

namespace GamePlay::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatar;

    class StatusPresenter final : public StatusPresenterBase
    {
    public:
        void Initialize(
            const Ui::PlayerStatus& playerStatusView,
            const GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus& playerStatusModel,
            const std::weak_ptr<MagicCasterAvatar>& magicCasterAvatar);

    private:
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) spellPalettePrefab_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) controlGuidePrefab_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(spellPalettePrefab_));
            archive(CEREAL_NVP(controlGuidePrefab_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(spellPalettePrefab_));
            if (version >= 1) archive(CEREAL_NVP(controlGuidePrefab_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::MagicCaster::StatusPresenter, 1)
