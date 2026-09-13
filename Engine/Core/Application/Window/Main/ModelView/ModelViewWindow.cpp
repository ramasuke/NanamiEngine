#include "ModelViewWindow.h"

#include <filesystem>
#include <string>

#include "../AnimationView/AnimationViewWindow.h"

namespace NanamiEngine::Core::MainWindow
{
    ModelViewWindow::ModelViewWindow()
        : MainWindowBase(true)
    {
    }

    void ModelViewWindow::AddContent(const std::shared_ptr<Module::Asset::Mv1File>& content)
    {
        if (!content)
            return;

        MainWindowBase::AddContent(content);
        Select(content->GetGuid());
    }

    void ModelViewWindow::Select(const Guid& guid)
    {
        const auto it = contents_.find(guid);
        if (it == contents_.end())
            return;

        selectedGuid_ = guid;
        stage_.SetModel(Application::ApplicationBase::MainWindows().Catch<ModelViewWindow>(), it->second);
    }

    void ModelViewWindow::OnUpdate()
    {
        stage_.PollModelLoad();
        stage_.UpdateViewport();
        LifeCycle().OnUpdateForEditor();
    }

    void ModelViewWindow::OnDrawGui(MainWindowDrawGuiContext context)
    {
        ImGui::Begin("ModelView");

        stage_.DrawViewportGui();

        // ウィンドウ切り替えは一覧描画の後に行う
        std::shared_ptr<Module::Asset::Mv1File> openAnimationView;
        if (selectedGuid_ && ImGui::Button("Open in AnimationView"))
            openAnimationView = contents_.at(*selectedGuid_);

        ImGui::Separator();
        ImGui::Text("Models");

        // 一覧描画中に contents_ を書き換えないよう、閉じる操作はループ後に行う
        std::optional<Guid> closeGuid;
        for (const auto& [guid, file] : contents_)
        {
            ImGui::PushID(guid.Value().c_str());
            if (ImGui::SmallButton("x"))
                closeGuid = guid;
            ImGui::SameLine();

            const std::string label    = std::filesystem::path(file->GetContentPath()).filename().string();
            const bool        selected = selectedGuid_ && *selectedGuid_ == guid;
            if (ImGui::Selectable(label.c_str(), selected))
                Select(guid);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", file->GetContentPath().c_str());
            ImGui::PopID();
        }
        if (closeGuid)
            CloseContent(*closeGuid);

        if (selectedGuid_)
        {
            ImGui::Separator();
            stage_.DrawPreviewObjectGui();
        }

        ImGui::End();

        if (openAnimationView)
            OpenInAnimationView(openAnimationView);
    }

    void ModelViewWindow::OnSave()
    {
        // ビューアなので保存対象は無い
    }

    void ModelViewWindow::CloseContent(const Guid& guid)
    {
        contents_.erase(guid);

        if (!selectedGuid_ || *selectedGuid_ != guid)
            return;

        selectedGuid_.reset();
        if (!contents_.empty())
        {
            Select(contents_.begin()->first);
            return;
        }

        stage_.ClearModel();
    }

    void ModelViewWindow::OpenInAnimationView(const std::shared_ptr<Module::Asset::Mv1File>& model) const
    {
        const auto window = Application::ApplicationBase::MainWindows().Catch<AnimationViewWindow>();
        // ComponentGroup::Add<T> はカレント MainWindow の LifeCycle に登録するため、AddContent より先に切り替える
        Application::ApplicationBase::OnChangeWindow(window);
        window->AddContent(model);
    }
}
