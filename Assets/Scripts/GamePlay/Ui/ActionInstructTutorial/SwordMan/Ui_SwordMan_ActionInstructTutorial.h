#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    class SwordManActionInstructTutorial final : public Component::ComponentBase
    {
    public:
        void OnDisplayAttackText      () { OnDisplayText(attackButtonTutorialText_    ); }
        void OnDisplayRunText         () { OnDisplayText(runButtonTutorialText_       ); }
        void OnDisplayDashAttackText  () { OnDisplayText(dashAttackButtonTutorialText_); }
        void OnDisplayAvoidRollingText() { OnDisplayText(avoidButtonTutorialText_); }
        void Hide();

    private:
        void OnDisplayText(const std::string& text);
        
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) textBox_;
        [[serialize(1)]] std::string textBoxName_;
        [[serialize(2)]] std::string attackButtonTutorialText_     = "Aボタンを押して攻撃!"               ;
        [[serialize(2)]] std::string runButtonTutorialText_        = "Xボタンでダッシュ!"                 ;
        [[serialize(2)]] std::string dashAttackButtonTutorialText_ = "Xボタンを押しながら、Aでダッシュ攻撃！";
        [[serialize(3)]] std::string avoidButtonTutorialText_      = "Yボタンで回避";
        
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(textBox_);
            archive(textBoxName_);
            archive(attackButtonTutorialText_);
            archive(runButtonTutorialText_);
            archive(dashAttackButtonTutorialText_);
            archive(avoidButtonTutorialText_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(textBox_);
            if (version >= 1) archive(textBoxName_);
            if (version >= 2) archive(attackButtonTutorialText_);
            if (version >= 2) archive(runButtonTutorialText_);
            if (version >= 2) archive(dashAttackButtonTutorialText_);
            if (version >= 3) archive(avoidButtonTutorialText_);
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::SwordManActionInstructTutorial, 3);
CEREAL_REGISTER_TYPE(GamePlay::Ui::SwordManActionInstructTutorial);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Ui::SwordManActionInstructTutorial);
#pragma endregion