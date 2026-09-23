#include "Prop_RestorationGate.h"

#include "../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    std::shared_ptr<RestorationGate> RestorationGate::Find(const GameCore::Story::Facility facility)
    {
        std::shared_ptr<RestorationGate> found;
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [&found, facility](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                if (found)
                    return;

                const auto gate = gameObject->Components().Catch<RestorationGate>().lock();
                if (gate && gate->facility_ == static_cast<int>(facility))
                    found = gate;
            });
        return found;
    }

    void RestorationGate::BeginPreview()
    {
        isPreviewing_ = true;
        Apply();
        if (previewCamera_)
            previewCamera_->SetPriority(previewPriority_);
    }

    void RestorationGate::EndPreview()
    {
        if (!isPreviewing_)
            return;

        isPreviewing_ = false;
        Apply();
        if (previewCamera_)
            previewCamera_->OnDisable();
    }

    void RestorationGate::OnStart()
    {
        Apply();
        GameCore::Story::StoryProgress::Instance().OnChanged().Subscribe(
            [this](NanamiEngine::R4::Unit)
        {
            Apply();
        }).AddTo(this);
    }

    void RestorationGate::Apply() const
    {
        const auto facility   = static_cast<GameCore::Story::Facility>(facility_);
        const bool isRestored = GameCore::Story::StoryProgress::Instance().IsRestored(facility);
        // NOTE: 下見の間は直す前でも直った見た目を出す
        const bool showRestored = isRestored || isPreviewing_;
        if (brokenObject_)
            brokenObject_->SetEnable(!showRestored);
        if (restoredObject_)
            restoredObject_->SetEnable(showRestored);
    }

    void RestorationGate::OnDrawGui()
    {
        auto facility = static_cast<GameCore::Story::Facility>(facility_);
        ImGuiHelper::OnDrawEnumField("facility_", facility, GameCore::Story::FACILITIES, GameCore::Story::ToString);
        facility_ = static_cast<int>(facility);
        ImGuiHelper::OnDrawInputField("brokenObject_", brokenObject_);
        ImGuiHelper::OnDrawInputField("restoredObject_", restoredObject_);
        ImGuiHelper::OnDrawInputField("previewCamera_", previewCamera_);
        ImGuiHelper::OnDrawInputField("previewPriority_", previewPriority_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::RestorationGate);
#pragma endregion
