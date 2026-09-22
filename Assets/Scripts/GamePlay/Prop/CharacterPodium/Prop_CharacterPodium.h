#pragma once
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../Data/Character/Data_CharacterData.h"

namespace GamePlay::Prop
{
    /**
     * @brief 酒場の展示台。名簿に載るキャラの実モデルを台の上に立て、選んだ一体だけを見せる。
     */
    class CharacterPodium final : public Component::ComponentBase,
                                  public LifeCycleCallback::IStartable
    {
    public:
        [[nodiscard]] const std::vector<std::shared_ptr<Asset::CharacterData>>& Characters() const { return characters_; }

        /** @brief 選ばれた一体だけを見せる。範囲外なら全部隠す */
        void ShowCharacter(size_t index) const;
        void FocusCamera  () const;
        void RestoreCamera() const;

    private:
        void OnStart() override;

        [[serialize(0)]] std::vector<FIELD(Asset::CharacterData)> characterAssets_;
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) podiumCamera_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) displayRoot_;
        [[serialize(0)]] int focusPriority_ = 100;

        std::vector<std::shared_ptr<Asset::CharacterData>> characters_;
        std::vector<std::weak_ptr<GameObject::IGameObject>> displayModels_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(characterAssets_));
            archive(CEREAL_NVP(podiumCamera_));
            archive(CEREAL_NVP(displayRoot_));
            archive(CEREAL_NVP(focusPriority_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(characterAssets_));
            if (version >= 0) archive(CEREAL_NVP(podiumCamera_));
            if (version >= 0) archive(CEREAL_NVP(displayRoot_));
            if (version >= 0) archive(CEREAL_NVP(focusPriority_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::CharacterPodium, 0)
