#pragma once
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    /**
     * @brief 手帳の目次の1行。選択中だけ朱の印と下線を出す。
     * 文言は実行時に SetContent で流し込むので、行ごとの設定は持たない。
     */
    class PauseMenuRow final : public Component::ComponentBase
    {
    public:
        void SetContent(const std::string& name, const std::string& description, int number) const;
        void SetHighlighted(bool isHighlighted) const;

    private:
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) descriptionText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) numberText_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) marker_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) underline_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(descriptionText_));
            archive(CEREAL_NVP(numberText_));
            archive(CEREAL_NVP(marker_));
            archive(CEREAL_NVP(underline_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(descriptionText_));
            if (version >= 0) archive(CEREAL_NVP(numberText_));
            if (version >= 0) archive(CEREAL_NVP(marker_));
            if (version >= 0) archive(CEREAL_NVP(underline_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::PauseMenuRow, 0);
