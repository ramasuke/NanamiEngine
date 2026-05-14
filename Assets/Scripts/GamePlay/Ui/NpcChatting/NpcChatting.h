#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Data
{
    class Chat;
}

namespace NanamiEngine::Module::Asset
{
    class NpcChat;
}

namespace GamePlay::Ui
{
    class NpcChatting final : public Component::ComponentBase
    {
    public:
        Coroutine::Task<void> OnDisplayChatAsync(
            const std::string & npcName,
            const Asset::NpcChat& npcChat) const;

    private:
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) textRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) npcNameTextBox_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(textRenderer_));
            archive(CEREAL_NVP(npcNameTextBox_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(textRenderer_));
            if (version >= 1) archive(CEREAL_NVP(npcNameTextBox_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::NpcChatting, 1);
CEREAL_REGISTER_TYPE(GamePlay::Ui::NpcChatting);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Ui::NpcChatting);
#pragma endregion
