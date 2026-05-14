#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Ui
{
    class BillBoardNpcChatIcon final : public Component::ComponentBase,
                                       public LifeCycleCallback::IAwakable,
                                       public LifeCycleCallback::IUpdatable
    {
    public:
        void Show();
        void Hide();
        void OnChattable();
        void OnExitChattable();
        
    private:
        void OnAwake () override;
        void OnUpdate() override;

        bool isShow_ = true;
        glm::vec3 basePosChattable_ = {};
        glm::vec3 basePosSurprise_  = {};
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattableIcon_; 
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattingIcon_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) surpriseIcon_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(chattableIcon_));
            archive(CEREAL_NVP(chattingIcon_));
            archive(CEREAL_NVP(surpriseIcon_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(chattableIcon_));
            if (version >= 0) archive(CEREAL_NVP(chattingIcon_));
            if (version >= 1) archive(CEREAL_NVP(surpriseIcon_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::BillBoardNpcChatIcon, 1);
CEREAL_REGISTER_TYPE(GamePlay::Ui::BillBoardNpcChatIcon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Ui::BillBoardNpcChatIcon);
#pragma endregion