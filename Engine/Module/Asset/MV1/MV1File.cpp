#include "MV1File.h"
#include "DxLib.h"

#include "../../../Core/Application/Window/Main/ModelView/ModelViewWindow.h"
#include "../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Module::Asset
{
    Mv1File::Mv1File(const std::string& contentPath)
        : contentPath_(contentPath)
    {
    }

    void Mv1File::OnEnableAsset() { dxLibHandle_ = MV1LoadModel(contentPath_.c_str()); }

    void Mv1File::OnDoubleClick()
    {
        // AddContent には自身の shared_ptr が必要なので ObjectRegistry から引く(.meta の有無に関わらず AssetFactory が登録している)
        const auto self = Core::Application::ApplicationBase::ObjectRegistry().Catch<Mv1File>(guid_).lock();
        if (!self)
        {
            LogError("Mv1File: ObjectRegistry に未登録のため開けません: " + contentPath_);
            return;
        }

        const auto window = Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::ModelViewWindow>();
        // ComponentGroup::Add<T> はカレント MainWindow の LifeCycle に登録するため、AddContent より先に切り替える
        Core::Application::ApplicationBase::OnChangeWindow(window);

        if (window->Contains(guid_))
        {
            window->Select(guid_);
            return;
        }
        window->AddContent(self);
    }

    bool Mv1File::IsLoadCompleted() const
    {
        // 非同期ロード中は dxLibHandle_ が -1 ではないので CheckHandleASyncLoad で完了を判定する(TRUE: まだロード中)
        return dxLibHandle_ != -1 && CheckHandleASyncLoad(dxLibHandle_) == FALSE;
    }

    void Mv1File::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
        LibCore::ImGuiHelper::OnDrawInputField("dxLibHandle_", dxLibHandle_);
    }

    const Guid& Mv1File::GetGuid        () const { return guid_; }
    int         Mv1File::LoadDxLibHandle() const { return MV1DuplicateModel(dxLibHandle_); }
    std::string Mv1File::GetContentPath () const { return contentPath_; }
}
