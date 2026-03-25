#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    constexpr auto ATTACK_BUTTON_TUTORIAL_TEXT      = "Aボタンを押して攻撃!"              ;
    constexpr auto RUN_BUTTON_TUTORIAL_TEXT         = "Xボタンでダッシュ!"                ;
    constexpr auto DASH_ATTACK_BUTTON_TUTORIAL_TEXT = "Xボタンを押しながら、Aでダッシュ攻撃！";
    
    class SwordManActionInstructTutorial final : public Component::ComponentBase
    {
    public:
        void OnDisplayAttackText    () const { OnDisplayText(ATTACK_BUTTON_TUTORIAL_TEXT     ); }
        void OnDisplayRunText       () const { OnDisplayText(RUN_BUTTON_TUTORIAL_TEXT        ); }
        void OnDisplayDashAttackText() const { OnDisplayText(DASH_ATTACK_BUTTON_TUTORIAL_TEXT); }
        void Hide();

    private:
        void OnDisplayText      (const std::string& text) const;
        
        FIELD(NanamiUi::TextRenderer) textBox_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(textBox_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(textBox_);
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::SwordManActionInstructTutorial, 0);
CEREAL_REGISTER_TYPE(GamePlay::Ui::SwordManActionInstructTutorial);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Ui::SwordManActionInstructTutorial);
#pragma endregion