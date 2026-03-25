#pragma once
#include "../../../Compiler/EngineCompiler.h"
#include "../ApplicationBase.h"

namespace NanamiEngine::Core::Application
{
    class EditorApplication final : public ApplicationBase
    {
    public:
        ///初期設定
        EditorApplication();
        static FileSystem::EditorDraggingHand& FileDraggingHand();
        
    private:
        void Run      () override;
        void OnExit   () override;
        void OnDrawGui();
        static void OnCompileEditorCompiler();

        Compiler::EngineCompiler compiler_;
    };
}
