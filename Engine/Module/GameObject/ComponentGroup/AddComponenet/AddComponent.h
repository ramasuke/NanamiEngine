#pragma once
#include "../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::GameObject
{
    class AddComponent final
    {
    public:
        static std::shared_ptr<Component::ComponentBase> OnDrawGui();

    private:
        static void OnDrawRendererGui   (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawSoundGui      (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawUiRendererGui (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawColliderGui   (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawCinemachineGui(std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawGameCoreGui   (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawGamePlayGui   (std::shared_ptr<Component::ComponentBase>& addComponent);
        
        ///addされる場合はoutComponentにComponentBaseのポインタを入れる
        ///addされない場合はnullptrを入れる
        template <typename T>
        [[nodiscard]] static void OnDrawTryAddComponentGui(std::shared_ptr<Component::ComponentBase>& outComponent)
        {
            static_assert(std::is_base_of_v<Component::ComponentBase, T>, "T must inherit from ComponentBase");

            if (ImGui::Button(StripNamespace(typeid(T).name()).c_str()))
            {
                outComponent = std::make_shared<T>();
            }
        }

        static std::string StripNamespace(const std::string& name);
    };
}
