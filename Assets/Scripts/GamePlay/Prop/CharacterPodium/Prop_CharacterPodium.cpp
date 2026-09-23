#include "Prop_CharacterPodium.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void CharacterPodium::OnStart()
    {
        const auto displayRoot = displayRoot_.get();
        if (!displayRoot)
            return;

        for (const auto& asset : characterAssets_)
        {
            const auto character = asset.get();
            if (!character)
                continue;

            characters_.push_back(character);

            const auto prefab = character->DisplayModelPrefab();
            if (!prefab)
            {
                displayModels_.emplace_back();
                continue;
            }

            auto model = Scene::GameObject::Instantiate(prefab, displayRoot->Transform().GetWorldPos(), displayRoot->Transform().GetWorldRot());
            if (const auto locked = model.lock())
            {
                locked->Transform().SetParent(displayRoot);
                locked->SetEnable(false);
            }
            displayModels_.push_back(model);
        }
    }

    void CharacterPodium::ShowCharacter(const size_t index) const
    {
        for (size_t i = 0; i < displayModels_.size(); ++i)
        {
            if (const auto model = displayModels_[i].lock())
                model->SetEnable(i == index);
        }
    }

    void CharacterPodium::FocusCamera() const
    {
        if (const auto camera = podiumCamera_.get())
            camera->SetPriority(focusPriority_);
    }

    void CharacterPodium::RestoreCamera() const
    {
        if (const auto camera = podiumCamera_.get())
            camera->SetPriority(CineMachine::DISABLE_PRIORITY);
    }

    void CharacterPodium::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("characterAssets_", characterAssets_, [this]
        {
            if (ImGui::Button("Add"))
            {
                characterAssets_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("podiumCamera_", podiumCamera_);
        ImGuiHelper::OnDrawInputField("displayRoot_", displayRoot_);
        ImGuiHelper::OnDrawInputField("focusPriority_", focusPriority_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::CharacterPodium);
#pragma endregion
