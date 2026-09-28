#pragma once
#include <cstdint>
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    /** @brief 設定画面の左に縦に並ぶカテゴリの札1枚。選んでいる札は明るい札に差し替える */
    class SettingsTabUi final : public Component::ComponentBase
    {
    public:
        void Show(const std::string& name);
        void SetSelected(bool isSelected);

    private:
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) plate_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) selectedPlate_;
        [[serialize(0)]] Color32 selectedColor_   = Color32(246, 234, 206);
        [[serialize(0)]] Color32 unselectedColor_ = Color32(200, 184, 152);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(plate_));
            archive(CEREAL_NVP(selectedPlate_));
            archive(CEREAL_NVP(selectedColor_));
            archive(CEREAL_NVP(unselectedColor_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(plate_));
            if (version >= 0) archive(CEREAL_NVP(selectedPlate_));
            if (version >= 0) archive(CEREAL_NVP(selectedColor_));
            if (version >= 0) archive(CEREAL_NVP(unselectedColor_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::SettingsTabUi, 0);
