#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "../../../Core/Game/PlayerAvatar/Interactable/IPlayerInteractable.h"
#include "../../Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"

namespace GamePlay::Prop
{
    /**
     * @brief 拠点のイベント掲示板。近づくとアイコンが変わり、調べると告知一覧のUIを出す。
     * 一覧の中身はUIプレハブ側が .eventBoard から読むので、ここは開くだけ。
     */
    class EventNoticeBoard final : public Component::ComponentBase,
                                   public LifeCycleCallback::IStartable,
                                   public GameCore::PlayerAvatar::IPlayerInteractable
    {
    private:
        void OnStart        () override;
        void OnInteractable    () override;
        void OnExitInteractable() override;
        void OnInteract         () override;
        [[nodiscard]] const GameObject::Transform& InteractableTransform() const override;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) eventBoardUiPrefab_;
        [[serialize(0)]] FIELD(Ui::BillBoardNpcChatIcon) chatIcon_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(eventBoardUiPrefab_));
            archive(CEREAL_NVP(chatIcon_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(eventBoardUiPrefab_));
            if (version >= 0) archive(CEREAL_NVP(chatIcon_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::EventNoticeBoard, 0)
