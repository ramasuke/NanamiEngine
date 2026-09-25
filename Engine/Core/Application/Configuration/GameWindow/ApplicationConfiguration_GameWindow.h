#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <array>

#include "../../../../Module/GameObject/Mark/GameObjectMark.h"

namespace NanamiEngine::Core::Application::Configuration
{
    /** @brief GameWindow 上に GameObject のマークを描くかの設定 */
    class NANAMI_API GameWindowConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static bool ShouldDrawMark(Module::GameObject::GameObjectMark mark, bool isPlayMode);

        static void DrawConfigGUI();

    private:
        using MarkFlags = std::array<bool, static_cast<size_t>(Module::GameObject::GameObjectMark::Count)>;

        static bool      showMarks_;
        static bool      showMarksInPlayMode_;
        static MarkFlags showMarkFlags_;
    };
}
