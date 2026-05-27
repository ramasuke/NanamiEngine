#pragma once
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Font/Ttf/TtfFontFile.h"
#include "../../Color/Color32.h"
#include "../../Component/ComponentBase.h"

namespace NanamiEngine::Module::NanamiUi
{
    class TextRenderer final : public Component::ComponentBase,
                               public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetText(const std::string& text);
        void SetFont(const std::shared_ptr<Asset::TtfFontFile>& font);
        void SetTextColor(const Color32& color);
        void SetWorldMode(bool isWorld);

    private:
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        void UpdateTextTexture();

    private:
        [[serialize(0)]] FIELD(Asset::TtfFontFile) fontFile_;
        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] std::string text_;
        [[serialize(0)]] Color32 textColor_;
        [[serialize(0)]] bool isWorldPos_ = false;

        // キャッシュ
        std::string cachedSjis_;
        bool isDirty_ = true;

        // MakeScreen
        int textScreen_ = -1;
        int screenW_ = 256;
        int screenH_ = 64;

#pragma region Serialization
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(fontFile_));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(text_));
            archive(CEREAL_NVP(textColor_));
            archive(CEREAL_NVP(isWorldPos_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(fontFile_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(text_));
            if (version >= 0) archive(CEREAL_NVP(textColor_));
            if (version >= 1) archive(CEREAL_NVP(isWorldPos_));
            isDirty_ = true;
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::TextRenderer, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::NanamiUi::TextRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiEngine::Module::NanamiUi::TextRenderer);
#pragma endregion
