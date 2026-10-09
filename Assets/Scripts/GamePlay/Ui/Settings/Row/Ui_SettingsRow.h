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
    // NOTE: 設定画面の一覧の1行。選択中は帯と左右の矢印を出す
    class SettingsRowUi final : public Component::ComponentBase
    {
    public:
        void Show(const std::string& label, const std::string& value);
        void SetSelected(bool isSelected);

    private:
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) labelText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) valueText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) band_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) leftArrow_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) rightArrow_;
        [[serialize(0)]] Color32 selectedColor_   = Color32(246, 234, 206);
        [[serialize(0)]] Color32 unselectedColor_ = Color32(200, 184, 152);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(labelText_));
            archive(CEREAL_NVP(valueText_));
            archive(CEREAL_NVP(band_));
            archive(CEREAL_NVP(leftArrow_));
            archive(CEREAL_NVP(rightArrow_));
            archive(CEREAL_NVP(selectedColor_));
            archive(CEREAL_NVP(unselectedColor_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(labelText_));
            if (version >= 0) archive(CEREAL_NVP(valueText_));
            if (version >= 0) archive(CEREAL_NVP(band_));
            if (version >= 0) archive(CEREAL_NVP(leftArrow_));
            if (version >= 0) archive(CEREAL_NVP(rightArrow_));
            if (version >= 0) archive(CEREAL_NVP(selectedColor_));
            if (version >= 0) archive(CEREAL_NVP(unselectedColor_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::SettingsRowUi, 0);
