#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "../../../../Data/Item/Data_ItemData.h"
#include "../../../Core/Game/PlayerAvatar/Interactable/IPlayerInteractable.h"
#include "../../Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"

namespace GamePlay::Prop
{
    /** @brief 調べると株が消えて item_ が飛び出す薬草。採ったことは保存しない */
    class HerbPatch final : public Component::ComponentBase,
                            public LifeCycleCallback::IStartable,
                            public GameCore::PlayerAvatar::IPlayerInteractable
    {
    private:
        void OnStart           () override;
        void OnInteractable    () override;
        void OnExitInteractable() override;
        void OnInteract        () override;
        [[nodiscard]] bool CanInteract() const override { return !isHarvested_; }
        [[nodiscard]] const GameObject::Transform& InteractableTransform() const override;
        [[nodiscard]] glm::vec3 DropPosition() const;

        [[serialize(0)]] FIELD(Asset::ItemData) item_;
        [[serialize(0)]] int count_ = 1;
        [[serialize(0)]] FIELD(GameObject::IGameObject) dropPoint_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) harvestParticle_;
        [[serialize(0)]] FIELD(Asset::SoundFile) harvestSound_;
        [[serialize(0)]] FIELD(Ui::BillBoardNpcChatIcon) chatIcon_;

        // NOTE: 破棄はフレーム末なので、同じフレームの2回目で二重に出さない
        bool isHarvested_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(item_));
            archive(CEREAL_NVP(count_));
            archive(CEREAL_NVP(dropPoint_));
            archive(CEREAL_NVP(harvestParticle_));
            archive(CEREAL_NVP(harvestSound_));
            archive(CEREAL_NVP(chatIcon_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(item_));
            if (version >= 0) archive(CEREAL_NVP(count_));
            if (version >= 0) archive(CEREAL_NVP(dropPoint_));
            if (version >= 0) archive(CEREAL_NVP(harvestParticle_));
            if (version >= 0) archive(CEREAL_NVP(harvestSound_));
            if (version >= 0) archive(CEREAL_NVP(chatIcon_));
        }
#pragma endregion
    };
}
