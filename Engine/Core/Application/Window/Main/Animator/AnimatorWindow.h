#pragma once
#include "../MainWindowBase.h"
#include "../../../../../Module/AnimationTree/AnimationTree.h"
#include "../Factory/MainWindowFactory.h"

namespace NanamiEngine::Core::MainWindow
{
    class AnimatorWindow final : public MainWindowBase<AnimationTree::AnimationTree>
    {   
    public:
        explicit AnimatorWindow() = default;
        
    private:
        void OnSave() override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnUpdate() override;
    };
    
    REGISTER_MAIN_WINDOW(AnimatorWindow)
}
