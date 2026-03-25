#pragma once
#include "../MainWindowBase.h"
#include "../../../Editor/Camera/Free/Editor3DCamera.h"
#include "../../../../../Module/GameObject/PrefabGameObject/PrefabGameObject.h"
#include "../Factory/MainWindowFactory.h"
#include "../../../../../Module/Scene/Scene.h"

namespace NanamiEngine::Core::MainWindow
{
    class PrefabViewWindow final : public virtual MainWindowBase<Module::GameObject::PrefabGameObject>
    {
    public:
        PrefabViewWindow();

    private:
        void OnUpdate()     override;
        void OnDrawGui(MainWindowDrawGuiContext context)    override;
        void OnDrawAddChildGameObjectButton() const;
        void OnSave()       override;
        
        Component::Editor3DCamera camera_;
    };

    REGISTER_MAIN_WINDOW(PrefabViewWindow)
}
