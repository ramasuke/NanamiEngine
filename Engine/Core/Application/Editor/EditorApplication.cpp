#include "EditorApplication.h"

#include <DxLib.h>
#include <memory>

#include "../../../../Libs/LibCore/ImGui/Wrapper/ImGuiWrapper.h"
#include "../../../../Libs/ImGui/ImGuiHelper.h"
#include "../Window/Toolbar/EditorToolbarWindow.h"
#include "EffekseerForDXLib.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "../LifeCycle/ApplicationLifeCycle.h"
#include "../Window/Popup/Group/PopupWindowGroup.h"

Core::Application::EditorApplication::EditorApplication()
{
    SetDrawScreen   (DX_SCREEN_BACK);
    SetMouseDispFlag(true          );

    ImGuiWrapper::CreateInstance();
    OnCompileEditorCompiler();
}

void Core::Application::EditorApplication::Run()
{
    while (ProcessMessage() >= 0)
    {
        ClearDrawScreen();
        
        ApplicationBase::Run();
        ImGuiWrapper::Instance().Update();
        ApplicationLifeCycle_().OnUpdate();
        GetMainWindow()->OnUpdate();
        
        OnDrawGui();
        ImGui::EndFrame();

        RenderVertex();
        ImGuiWrapper::Instance().Draw();
        ScreenFlip();
    }
}

void Core::Application::EditorApplication::OnExit()
{
}

void Core::Application::EditorApplication::OnDrawGui()
{
    EditorToolbarWindow::OnDraw(PopupWindows());
    GetMainWindow()->OnDrawGui(MainWindow::MainWindowDrawGuiContext(FileDraggingHand()));
    PopupWindows_().OnDraw(FileDraggingHand());
}
    
void Core::Application::EditorApplication::OnCompileEditorCompiler()
{
    // Compiler::EngineCompiler::ProcessHeaderFiles("Assets/Scripts"  , true);
    // Compiler::EngineCompiler::ProcessHeaderFiles("Engine/Module/Component"  , true);
    // Compiler::EngineCompiler::ProcessHeaderFiles("Packages", true);
}

Core::FileSystem::EditorDraggingHand& Core::Application::EditorApplication::FileDraggingHand()
{
    static FileSystem::EditorDraggingHand fileDraggingHand;
    return fileDraggingHand;
}