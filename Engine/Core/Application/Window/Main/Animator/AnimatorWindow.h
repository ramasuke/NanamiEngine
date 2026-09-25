#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../MainWindowBase.h"
#include "../../../../../Module/AnimationTree/AnimationTree.h"
#include "../Factory/MainWindowFactory.h"

namespace NanamiEngine::Core::MainWindow
{
    class NANAMI_API AnimatorWindow final : public MainWindowBase<AnimationTree::AnimationTree>
    {   
    public:
        explicit AnimatorWindow();
        
    private:
        void OnSave() override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnUpdate() override;
    };
    
    REGISTER_MAIN_WINDOW(AnimatorWindow, "Animation")
}
