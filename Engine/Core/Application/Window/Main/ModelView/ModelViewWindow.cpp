#include "ModelViewWindow.h"

#include <DxLib.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <string>

#include "../../../../Object/Registry/ObjectRegistry.h"
#include "../../../../../Module/Asset/Asset.h"
#include "../../../../../Module/Component/ModelRenderer/ModelRenderer.h"
#include "../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"

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
        if (!contents_.contains(guid))
            return;

        selectedGuid_ = guid;
        ApplySelectedModel();
        pendingFrame_ = true;
    }

    void ModelViewWindow::OnUpdate()
    {
        if (const auto renderer = modelRenderer_.lock(); renderer && selectedGuid_)
        {
            if (renderer->modelDxLibHandle_ == -1)
            {
                // 非同期ロード中に SetMv1File すると -1 になるので、ロード完了後に取り直す
                const auto it = contents_.find(*selectedGuid_);
                if (it != contents_.end() && it->second->IsLoadCompleted())
                    renderer->SetMv1File(it->second);
            }
            else if (pendingFrame_)
            {
                FrameCamera();
                pendingFrame_ = false;
            }
        }

        // 描画より先にカメラを更新し、FrameCamera の結果をこのフレームの描画に反映させる
        camera_.OnUpdate();
        if (showGrid_)
            DrawGrid();
        LifeCycle().OnUpdateForEditor();
    }

    void ModelViewWindow::OnDrawGui(MainWindowDrawGuiContext context)
    {
        ImGui::Begin("ModelView");

        if (ImGui::Button("Reset Camera"))
            pendingFrame_ = true;
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &showGrid_);

        const auto renderer = modelRenderer_.lock();
        const int  handle   = renderer ? renderer->modelDxLibHandle_ : -1;
        if (selectedGuid_ && handle == -1)
            ImGui::Text("handle: -1 (not loaded)");
        else
            ImGui::Text("handle: %d", handle);
        ImGui::Text("LoadingResource Count: %d", Module::Asset::Asset::GetLoadingResourceCount());
        ImGui::TextDisabled("Right-drag: look / WASD+Space: move / LShift: fast");

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

        if (previewObject_ && selectedGuid_)
        {
            ImGui::Separator();
            if (ImGui::TreeNodeEx("Preview Object", ImGuiTreeNodeFlags_DefaultOpen))
            {
                previewObject_->OnDrawGui();
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void ModelViewWindow::OnSave()
    {
        // ビューアなので保存対象は無い
    }

    void ModelViewWindow::EnsurePreviewObject()
    {
        if (previewObject_ && !modelRenderer_.expired())
            return;

        // ComponentGroup::Add<T> はカレント MainWindow の LifeCycle にコールバックを登録するため、自分がカレントであることを保証する
        const auto self = Application::ApplicationBase::MainWindows().Catch<ModelViewWindow>();
        if (Application::ApplicationBase::GetMainWindow() != self)
            Application::ApplicationBase::OnChangeWindow(self);

        if (!previewObject_)
        {
            previewObject_ = std::make_shared<Scene::SceneGameObject>();
            previewObject_->InitGameObject(std::weak_ptr<Module::GameObject::IGameObject>(), previewObject_);
        }
        modelRenderer_ = previewObject_->Components().Add<Module::Component::ModelRenderer>();
    }

    void ModelViewWindow::ApplySelectedModel()
    {
        if (!selectedGuid_)
            return;

        const auto it = contents_.find(*selectedGuid_);
        if (it == contents_.end())
            return;

        EnsurePreviewObject();
        if (const auto renderer = modelRenderer_.lock())
            renderer->SetMv1File(it->second);
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

        // 最後の 1 件を閉じたらプレビュー用 GameObject ごと破棄する(ModelRenderer::OnDestroy で MV1DeleteModel される)
        if (previewObject_)
        {
            previewObject_->ImplementDestroy();
            Application::ApplicationBase::ObjectRegistry().Remove(previewObject_->GetGuid());
            previewObject_.reset();
        }
        modelRenderer_.reset();
    }

    void ModelViewWindow::FrameCamera()
    {
        const auto renderer = modelRenderer_.lock();
        if (!renderer || renderer->modelDxLibHandle_ == -1 || !previewObject_)
        {
            camera_ = Module::Component::Editor3DCamera();
            return;
        }

        const int handle = renderer->modelDxLibHandle_;
        // フレームのローカル→ワールド行列にプレビュー Transform を含めるため、先に行列を設定しておく
        MV1SetMatrix(handle, previewObject_->Transform().GetDxWorldMatrix());

        glm::vec3 minPos( FLT_MAX);
        glm::vec3 maxPos(-FLT_MAX);
        bool      hasVertex = false;
        const int frameNum  = MV1GetFrameNum(handle);
        for (int frame = 0; frame < frameNum; ++frame)
        {
            if (MV1GetFrameVertexNum(handle, frame) <= 0)
                continue;

            const VECTOR localMin     = MV1GetFrameMinVertexLocalPosition(handle, frame);
            const VECTOR localMax     = MV1GetFrameMaxVertexLocalPosition(handle, frame);
            const MATRIX localToWorld = MV1GetFrameLocalWorldMatrix(handle, frame);
            for (int corner = 0; corner < 8; ++corner)
            {
                const VECTOR local = VGet(
                    (corner & 1) ? localMax.x : localMin.x,
                    (corner & 2) ? localMax.y : localMin.y,
                    (corner & 4) ? localMax.z : localMin.z);
                const VECTOR world = VTransform(local, localToWorld);
                minPos    = glm::min(minPos, glm::vec3(world.x, world.y, world.z));
                maxPos    = glm::max(maxPos, glm::vec3(world.x, world.y, world.z));
                hasVertex = true;
            }
        }
        if (!hasVertex)
        {
            minPos = glm::vec3(-1.0f);
            maxPos = glm::vec3( 1.0f);
        }

        const glm::vec3 center = (minPos + maxPos) * 0.5f;
        const float     radius = (std::max)(glm::length(maxPos - minPos) * 0.5f, 0.01f);

        // Editor3DCamera は FOV 90° / near 5 固定。外接球が視錐台に内接する距離 (r / sin(fov/2)) に余裕を足す
        const float fovY      = glm::radians(90.0f);
        const float nearPlane = 5.0f;
        float distance = radius / std::sin(fovY * 0.5f) * 1.15f;
        distance = (std::max)(distance, radius + nearPlane + 1.0f);

        // 右上前方から見下ろす。Editor3DCamera の前方は rotation * (0,0,1) なので LH 版の lookAt を使う
        const glm::vec3 viewDir = glm::normalize(glm::vec3(-0.45f, -0.35f, -1.0f));
        camera_.SetPosition(center - viewDir * distance);
        camera_.SetRotation(glm::quatLookAtLH(viewDir, glm::vec3(0.0f, 1.0f, 0.0f)));

        // グリッド間隔はモデルの大きさに合わせて 10 のべき乗にする
        gridStep_ = std::pow(10.0f, std::floor(std::log10(radius)));
    }

    void ModelViewWindow::DrawGrid() const
    {
        constexpr int      lineCount = 10;
        const float        extent    = gridStep_ * static_cast<float>(lineCount);
        const unsigned int axisColor = GetColor(180, 180, 180);
        const unsigned int gridColor = GetColor( 80,  80,  80);

        for (int i = -lineCount; i <= lineCount; ++i)
        {
            const float        p     = gridStep_ * static_cast<float>(i);
            const unsigned int color = (i == 0) ? axisColor : gridColor;
            DrawLine3D(VGet(p, 0.0f, -extent), VGet(p, 0.0f, extent), color);
            DrawLine3D(VGet(-extent, 0.0f, p), VGet(extent, 0.0f, p), color);
        }
    }
}
