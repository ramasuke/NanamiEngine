#pragma once
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    /// 手帳のステータス面に並べる持ち物ひとつ(アイコンと個数)
    class PauseMenuItemCell final : public Component::ComponentBase
    {
    public:
        void SetContent(const std::weak_ptr<Asset::SpriteFile>& icon, int count) const;

    private:
        [[serialize(0)]] FIELD(Component::ImageRenderer) icon_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) countText_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(icon_));
            archive(CEREAL_NVP(countText_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(icon_));
            if (version >= 0) archive(CEREAL_NVP(countText_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::PauseMenuItemCell, 0);
