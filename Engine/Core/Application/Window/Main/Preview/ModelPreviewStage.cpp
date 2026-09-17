#include "ModelPreviewStage.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "../../../ApplicationBase.h"
#include "../../../../Object/Registry/ObjectRegistry.h"
#include "../../../../../Module/Asset/Asset.h"
#include "../../../../../Module/Component/ModelRenderer/ModelRenderer.h"
#include "../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"

namespace NanamiEngine::Core::MainWindow
{
    void ModelPreviewStage::SetModel(const std::shared_ptr<IMainWindow>& owner, const std::shared_ptr<Module::Asset::Mv1File>& model)
    {
        if (!model)
            return;

        model_ = model;
        EnsurePreviewObject(owner);
        if (const auto renderer = modelRenderer_.lock())
            renderer->SetMv1File(model_);
        pendingFrame_ = true;
    }

    void ModelPreviewStage::ClearModel()
    {
        if (previewObject_)
        {
            previewObject_->ImplementDestroy();
            Application::ApplicationBase::ObjectRegistry().Remove(previewObject_->GetGuid());
            previewObject_.reset();
        }
        modelRenderer_.reset();
        model_.reset();
    }

    void ModelPreviewStage::PollModelLoad()
    {
        const auto renderer = modelRenderer_.lock();
        if (!renderer || !model_)
            return;

        // 非同期ロード中に SetMv1File すると -1 になるので、ロード完了後に取り直す
        if (renderer->modelDxLibHandle_ == -1 && model_->IsLoadCompleted())
            renderer->SetMv1File(model_);
    }

    void ModelPreviewStage::UpdateViewport()
    {
        if (pendingFrame_ && model_ && ModelHandle() != -1)
        {
            FrameCamera();
            pendingFrame_ = false;
        }

        // 描画より先にカメラを更新し、FrameCamera の結果をこのフレームの描画に反映させる
        camera_.OnUpdate();
        if (showGrid_)
            DrawGrid();
    }

    void ModelPreviewStage::DrawViewportGui()
    {
        if (ImGui::Button("Reset Camera"))
        {
            frameViewDirection_ = DefaultFrameViewDirection();
            pendingFrame_       = true;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &showGrid_);

        const int handle = ModelHandle();
        if (model_ && handle == -1)
            ImGui::Text("handle: -1 (not loaded)");
        else
            ImGui::Text("handle: %d", handle);
        ImGui::Text("LoadingResource Count: %d", Module::Asset::Asset::GetLoadingResourceCount());
        ImGui::TextDisabled("Right-drag: look / WASD+Space: move / LShift: fast");
    }

    void ModelPreviewStage::DrawPreviewObjectGui() const
    {
        if (!previewObject_)
            return;

        if (ImGui::TreeNodeEx("Preview Object", ImGuiTreeNodeFlags_DefaultOpen))
        {
            previewObject_->OnDrawGui();
            ImGui::TreePop();
        }
    }

    int ModelPreviewStage::ModelHandle() const
    {
        const auto renderer = modelRenderer_.lock();
        return renderer ? renderer->modelDxLibHandle_ : -1;
    }

    MATRIX ModelPreviewStage::PreviewWorldMatrix() const
    {
        return previewObject_ ? previewObject_->Transform().GetDxWorldMatrix() : MGetIdent();
    }

    void ModelPreviewStage::EnsurePreviewObject(const std::shared_ptr<IMainWindow>& owner)
    {
        if (previewObject_ && !modelRenderer_.expired())
            return;

        // ComponentGroup::Add<T> はカレント MainWindow の LifeCycle にコールバックを登録するため、owner がカレントであることを保証する
        if (Application::ApplicationBase::GetMainWindow() != owner)
            Application::ApplicationBase::OnChangeWindow(owner);

        if (!previewObject_)
        {
            previewObject_ = std::make_shared<Scene::SceneGameObject>();
            previewObject_->InitGameObject(std::weak_ptr<Module::GameObject::IGameObject>(), previewObject_);
        }
        modelRenderer_ = previewObject_->Components().Add<Module::Component::ModelRenderer>();
    }

    void ModelPreviewStage::FrameCamera()
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

        // Editor3DCamera の前方は rotation * (0,0,1) なので LH 版の lookAt を使う
        const glm::vec3 viewDir = frameViewDirection_;
        camera_.SetPosition(center - viewDir * distance);
        camera_.SetRotation(glm::quatLookAtLH(viewDir, glm::vec3(0.0f, 1.0f, 0.0f)));

        // グリッド間隔はモデルの大きさに合わせて 10 のべき乗にする
        gridStep_ = std::pow(10.0f, std::floor(std::log10(radius)));
    }

    glm::vec3 ModelPreviewStage::DefaultFrameViewDirection()
    {
        // 右上前方から見下ろす
        return glm::normalize(glm::vec3(-0.45f, -0.35f, -1.0f));
    }

    void ModelPreviewStage::DrawGrid() const
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
