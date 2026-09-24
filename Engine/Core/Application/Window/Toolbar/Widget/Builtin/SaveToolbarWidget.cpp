#include "SaveToolbarWidget.h"

#include <string>

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../../ApplicationBase.h"
#include "../../../../../FileSystem/Directory/Directory.h"
#include "../../../../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Main/Game/GameWindow.h"

namespace NanamiEngine::Core::Toolbar
{
    bool SaveToolbarWidget::IsVisible() const
    {
        return !Application::ApplicationBase::GameWindow()->IsPlaying();
    }

    void SaveToolbarWidget::OnDraw(EditorToolbarWidgetContext&)
    {
        if (ImGui::Button("Save"))
        {
            try
            {
                Application::ApplicationBase::MainWindows    ().OnSave();
                Application::ApplicationBase::AssetsDirectory().OnSave();
            }
            catch (const Module::Exception::NanamiException& exception)
            {
                Module::LogError("EditorToolbar: 保存に失敗しました: " + std::string(exception.what()));
            }
        }
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(SaveToolbarWidget, 200)
}
