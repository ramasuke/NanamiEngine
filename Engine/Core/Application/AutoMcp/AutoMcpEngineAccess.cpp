#include "AutoMcpEngineAccess.h"

#include <ranges>

#include "DxLib.h"
#include "../Window/Main/AnimationView/AnimationPreviewSlot.h"
#include "../Window/Main/AnimationView/AnimationViewWindow.h"
#include "../Window/Main/Game/GameWindow.h"
#include "../Window/Main/ModelView/ModelViewWindow.h"
#include "../Window/Main/Preview/ModelPreviewStage.h"
#include "../Window/Popup/Group/PopupWindowGroup.h"
#include "../Window/Popup/Inspector/InspectorWindow.h"
#include "../Configuration/DebugDraw/ApplicationConfiguration_DebugDraw.h"
#include "../../../Module/Scene/Scene.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    std::vector<std::shared_ptr<NanamiEngine::Scene::Scene>> AutoMcpEngineAccess::Scenes(const MainWindow::GameWindow& gameWindow)
    {
        return gameWindow.Scenes();
    }

    std::shared_ptr<NanamiEngine::Scene::Scene> AutoMcpEngineAccess::MainScene(const MainWindow::GameWindow& gameWindow)
    {
        return gameWindow.mainScene_.lock();
    }

    const std::string& AutoMcpEngineAccess::FilePath(const NanamiEngine::Scene::Scene& scene)
    {
        return scene.filePath_;
    }

    std::shared_ptr<NanamiEngine::Scene::Scene> AutoMcpEngineAccess::ReloadScene(MainWindow::GameWindow& gameWindow, const ::Guid& guid)
    {
        const auto it = gameWindow.contents_.find(guid);
        if (it == gameWindow.contents_.end())
            return nullptr;

        const std::shared_ptr<NanamiEngine::Scene::Scene> oldScene = it->second;
        const bool isMainScene = gameWindow.mainScene_.lock() == oldScene;

        // 読み込みに失敗したら例外で抜け、元のシーンは残す
        const auto newScene = std::make_shared<NanamiEngine::Scene::Scene>(oldScene->filePath_);

        oldScene->RemoveImplementAllGameObject();
        gameWindow.contents_.erase(it);
        gameWindow.AddContent(newScene);
        if (isMainScene)
            gameWindow.ChangeMainScene(newScene);

        return newScene;
    }

    void AutoMcpEngineAccess::Play(MainWindow::GameWindow& gameWindow)
    {
        gameWindow.Play();
    }

    void AutoMcpEngineAccess::Stop(MainWindow::GameWindow& gameWindow)
    {
        gameWindow.Stop();
    }

    void AutoMcpEngineAccess::End(MainWindow::GameWindow& gameWindow)
    {
        gameWindow.End();
    }

    void AutoMcpEngineAccess::ForEachPopupWindow(PopupWindow::PopupWindowGroup& group, const std::function<void(const ::Guid&, PopupWindow::IPopupWindow&)>& action)
    {
        for (const auto& [guid, window] : group.popupWindows_)
        {
            if (window)
                action(guid, *window);
        }
    }

    bool AutoMcpEngineAccess::ClosePopupWindow(PopupWindow::PopupWindowGroup& group, const ::Guid& guid)
    {
        return group.popupWindows_.erase(guid) > 0;
    }

    std::vector<PopupWindow::InspectorWindow*> AutoMcpEngineAccess::ExistingInspectorWindows(PopupWindow::PopupWindowGroup& group)
    {
        std::vector<PopupWindow::InspectorWindow*> result;
        for (const auto& window : group.popupWindows_ | std::views::values)
        {
            if (auto* inspector = dynamic_cast<PopupWindow::InspectorWindow*>(window.get()))
                result.push_back(inspector);
        }
        return result;
    }

    std::vector<std::shared_ptr<Module::Asset::Mv1File>> AutoMcpEngineAccess::ModelViewContents(const MainWindow::ModelViewWindow& window)
    {
        std::vector<std::shared_ptr<Module::Asset::Mv1File>> result;
        for (const auto& file : window.contents_ | std::views::values)
            result.push_back(file);
        return result;
    }

    std::optional<::Guid> AutoMcpEngineAccess::ModelViewSelectedGuid(const MainWindow::ModelViewWindow& window)
    {
        return window.selectedGuid_;
    }

    void AutoMcpEngineAccess::ModelViewClose(MainWindow::ModelViewWindow& window, const ::Guid& guid)
    {
        window.CloseContent(guid);
    }

    MainWindow::ModelPreviewStage& AutoMcpEngineAccess::ModelViewStage(MainWindow::ModelViewWindow& window)
    {
        return window.stage_;
    }

    MainWindow::ModelPreviewStage& AutoMcpEngineAccess::AnimationViewStage(MainWindow::AnimationViewWindow& window)
    {
        return window.stage_;
    }

    std::shared_ptr<Module::Asset::Mv1File> AutoMcpEngineAccess::AnimationViewModel(const MainWindow::AnimationViewWindow& window)
    {
        return window.modelFile_.get();
    }

    MainWindow::AnimationPreviewSlot& AutoMcpEngineAccess::AnimationViewSlot(MainWindow::AnimationViewWindow& window, const bool isSlotB)
    {
        return isSlotB ? window.slotB_ : window.slotA_;
    }

    bool& AutoMcpEngineAccess::AnimationViewPlaying(MainWindow::AnimationViewWindow& window)
    {
        return window.isPlaying_;
    }

    bool& AutoMcpEngineAccess::AnimationViewUseBlend(MainWindow::AnimationViewWindow& window)
    {
        return window.useBlend_;
    }

    float& AutoMcpEngineAccess::AnimationViewBlendWeight(MainWindow::AnimationViewWindow& window)
    {
        return window.blendWeight_;
    }

    bool& AutoMcpEngineAccess::AnimationViewNameCheck(MainWindow::AnimationViewWindow& window)
    {
        return window.nameCheck_;
    }

    bool& AutoMcpEngineAccess::AnimationViewLockRootMotion(MainWindow::AnimationViewWindow& window)
    {
        return window.lockRootMotion_;
    }

    int& AutoMcpEngineAccess::AnimationViewRootFrameIndex(MainWindow::AnimationViewWindow& window)
    {
        return window.rootFrameIndex_;
    }

    std::shared_ptr<Module::Asset::Mv1File> AutoMcpEngineAccess::SlotAnimationFile(const MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.animationFile_.get();
    }

    void AutoMcpEngineAccess::SetSlotAnimationFile(MainWindow::AnimationPreviewSlot& slot, const std::shared_ptr<Module::Asset::Mv1File>& file)
    {
        if (file)
            slot.animationFile_ = file;
        else
            slot.animationFile_ = FIELD(Module::Asset::Mv1File)();
    }

    bool AutoMcpEngineAccess::IsSlotSourceReady(const MainWindow::AnimationPreviewSlot& slot)
    {
        const auto                  file       = slot.animationFile_.get();
        const std::optional<::Guid> wantedGuid = file ? std::optional<::Guid>(file->GetGuid()) : std::nullopt;
        if (wantedGuid != slot.sourceGuid_)
            return false;

        return !slot.sourceGuid_ || (slot.sourceHandle_ != -1 && CheckHandleASyncLoad(slot.sourceHandle_) == FALSE);
    }

    int AutoMcpEngineAccess::SlotClipSourceHandle(const MainWindow::AnimationPreviewSlot& slot, const int modelHandle)
    {
        const int handle = slot.ClipSourceHandle(modelHandle);
        return handle != -1 && CheckHandleASyncLoad(handle) == FALSE ? handle : -1;
    }

    void AutoMcpEngineAccess::SelectSlotClip(MainWindow::AnimationPreviewSlot& slot, const int clipIndex)
    {
        slot.SelectClip(clipIndex);
    }

    bool AutoMcpEngineAccess::IsSlotAttached(const MainWindow::AnimationPreviewSlot& slot)
    {
        // 切り替え要求が Sync に反映されるまでは、前のクリップ・アニメ元のアタッチを「アタッチ済み」と見なさない
        return slot.attachIndex_ != -1
            && slot.attachedClipIndex_ == slot.clipIndex_
            && IsSlotSourceReady(slot)
            && slot.attachedSourceHandle_ == (slot.sourceGuid_ ? slot.sourceHandle_ : -1);
    }

    int AutoMcpEngineAccess::SlotClipIndex(const MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.clipIndex_;
    }

    float AutoMcpEngineAccess::SlotTotalTime(const MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.totalTime_;
    }

    float& AutoMcpEngineAccess::SlotSpeed(MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.speed_;
    }

    bool& AutoMcpEngineAccess::SlotLoop(MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.isLoop_;
    }

    float& AutoMcpEngineAccess::SlotStartTime(MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.startTime_;
    }

    float& AutoMcpEngineAccess::SlotEndTime(MainWindow::AnimationPreviewSlot& slot)
    {
        return slot.endTime_;
    }

    void AutoMcpEngineAccess::SetSlotTime(MainWindow::AnimationPreviewSlot& slot, const float time)
    {
        slot.time_ = time;
        // 別クリップへの切り替え待ちの間は totalTime_ が古いので、ここではクランプしない
        if (slot.attachIndex_ != -1 && slot.attachedClipIndex_ == slot.clipIndex_)
            slot.Seek(time);
    }

    void AutoMcpEngineAccess::PreviewFrame(MainWindow::ModelPreviewStage& stage, const glm::vec3& viewDirection)
    {
        stage.frameViewDirection_ = glm::normalize(viewDirection);
        stage.pendingFrame_       = true;
    }

    glm::vec3 AutoMcpEngineAccess::PreviewCameraPosition(const MainWindow::ModelPreviewStage& stage)
    {
        return stage.camera_.GetPosition();
    }

    glm::quat AutoMcpEngineAccess::PreviewCameraRotation(const MainWindow::ModelPreviewStage& stage)
    {
        return stage.camera_.GetRotataion();
    }

    void AutoMcpEngineAccess::SetPreviewCamera(MainWindow::ModelPreviewStage& stage, const glm::vec3& position, const glm::quat& rotation)
    {
        stage.camera_.SetPosition(position);
        stage.camera_.SetRotation(rotation);
        // ロード完了待ちの自動フレーミングで上書きされないようにする
        stage.pendingFrame_ = false;
    }

    bool& AutoMcpEngineAccess::DebugDrawAllColliders()
    {
        return Configuration::DebugDrawConfiguration::showAllColliders_;
    }

    bool& AutoMcpEngineAccess::DebugDrawColliderKind(const Module::Physics::ColliderShapeKind kind)
    {
        return Configuration::DebugDrawConfiguration::showColliderKinds_.at(static_cast<std::size_t>(kind));
    }

    bool& AutoMcpEngineAccess::DebugDrawColliderLayer(const Module::Physics::Layer layer)
    {
        return Configuration::DebugDrawConfiguration::showColliderLayers_.at(static_cast<std::size_t>(layer));
    }

    bool& AutoMcpEngineAccess::DebugDrawTriggerColliders()
    {
        return Configuration::DebugDrawConfiguration::showTriggerColliders_;
    }

    bool& AutoMcpEngineAccess::DebugDrawMainCameraFrustum()
    {
        return Configuration::DebugDrawConfiguration::showMainCameraFrustum_;
    }

    bool& AutoMcpEngineAccess::DebugDrawVirtualCameraFrustums()
    {
        return Configuration::DebugDrawConfiguration::showVirtualCameraFrustums_;
    }
}
