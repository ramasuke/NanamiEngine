#pragma once
#include "../../../Status/Presenter/PlayerAvatar_StatusPresenterBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../GamePlay/Ui/PlayerStatus/Ui_LowHealthScreenEffect.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStatus;
}

namespace GamePlay::Ui
{
    class PlayerStatus;
}

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStatus;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;

    class StatusPresenter final : public StatusPresenterBase
    {
    public:
        void Initialize(
            const Ui::PlayerStatus& playerStatusView,
            const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& playerStatusModel,
            const std::weak_ptr<SwordManAvatar>& swordManAvatar);
        
    private:
        [[serialize(3)]] FIELD(Ui::LowHealthScreenEffect) lowHealthScreenEffect_;
        [[serialize(2)]] FIELD(Asset::PrefabGameObjectFile) controlGuidePrefab_;
        [[serialize(4)]] FIELD(Asset::PrefabGameObjectFile) itemBarPrefab_;
        [[serialize(5)]] FIELD(Asset::PrefabGameObjectFile) pauseMenuPrefab_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(lowHealthScreenEffect_));
            archive(CEREAL_NVP(controlGuidePrefab_));
            archive(CEREAL_NVP(itemBarPrefab_));
            archive(CEREAL_NVP(pauseMenuPrefab_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 3) archive(CEREAL_NVP(lowHealthScreenEffect_));
            if (version >= 2) archive(CEREAL_NVP(controlGuidePrefab_));
            if (version >= 4) archive(CEREAL_NVP(itemBarPrefab_));
            if (version >= 5) archive(CEREAL_NVP(pauseMenuPrefab_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::SwordMan::StatusPresenter, 5);
