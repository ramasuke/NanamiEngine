#pragma once
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "Context/Data_Chat.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto FRIENDLY_NPC_CHAT_EXTENSION_LABEL = ".npcChat";
    using namespace GamePlay::Data;
    
    class NpcChat final : public ScriptableObject
    {
    public:
        explicit NpcChat(const std::string& contentPath = "");
        [[nodiscard]] const std::vector<Chat>& Get() const { return npcChatTexts_; }
        
    private:
        std::vector<Chat> npcChatTexts_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));

            const int count = static_cast<int>(npcChatTexts_.size());
            archive(cereal::make_nvp("count", count));

            for (int i = 0; i < count; ++i)
            {
                archive(cereal::make_nvp(("item_" + std::to_string(i)).c_str(), npcChatTexts_[i]));
            }
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));

            int count = 0;
            archive(cereal::make_nvp("count", count));
            npcChatTexts_.resize(count);
            for (int i = 0; i < count; ++i)
            {
                archive(cereal::make_nvp(("item_" + std::to_string(i)).c_str(), npcChatTexts_[i]));
            }
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(NpcChat, FRIENDLY_NPC_CHAT_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::NpcChat, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::NpcChat);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::NpcChat);
#pragma endregion
