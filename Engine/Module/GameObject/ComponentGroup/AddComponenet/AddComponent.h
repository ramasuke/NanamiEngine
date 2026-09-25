#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::GameObject
{
    class NANAMI_API AddComponent final
    {
    public:
        using DrawMenuFunc = void (*)(std::shared_ptr<Component::ComponentBase>& addComponent);

        static std::shared_ptr<Component::ComponentBase> OnDrawGui();

        // NOTE: 標準メニューの後ろに order 昇順で並ぶ
        static bool RegisterMenu(DrawMenuFunc draw, int order = 0);

        ///addされる場合はoutComponentにComponentBaseのポインタを入れる
        ///addされない場合はnullptrを入れる
        template <typename T>
        static void OnDrawTryAddComponentGui(std::shared_ptr<Component::ComponentBase>& outComponent)
        {
            static_assert(std::is_base_of_v<Component::ComponentBase, T>, "T must inherit from ComponentBase");

            if (ImGui::Button(StripNamespace(typeid(T).name()).c_str()))
            {
                outComponent = std::make_shared<T>();
            }
        }

    private:
        static void OnDrawRendererGui   (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawSoundGui      (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawUiRendererGui (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawColliderGui   (std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawCinemachineGui(std::shared_ptr<Component::ComponentBase>& addComponent);
        static void OnDrawNetworkGui    (std::shared_ptr<Component::ComponentBase>& addComponent);
        static std::string StripNamespace(const std::string& name);
    };
}
