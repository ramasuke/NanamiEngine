#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <cstdio>
#include <memory>

#include "../../Weather/Sandstorm.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GamePlay::Debug
{
    namespace
    {
        // NOTE: 今のメインシーンの Sandstorm。砂漠以外では無い
        std::shared_ptr<Weather::Sandstorm> FindSandstorm()
        {
            std::shared_ptr<Weather::Sandstorm> found;
            NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
                [&found](const std::shared_ptr<GameObject::IGameObject>& gameObject)
                {
                    if (!found)
                        found = gameObject->Components().Catch<Weather::Sandstorm>().lock();
                });
            return found;
        }

        void DrawSandstorm()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            const auto gameWindow = NanamiEngine::Core::Application::ApplicationBase::GameWindow();
            if (!gameWindow || !gameWindow->IsPlaying())
            {
                Widgets::Note("プレイ中だけ使える。");
                return;
            }

            const auto sandstorm = FindSandstorm();
            if (!sandstorm)
            {
                Widgets::Note("このシーンには砂嵐が無い (砂漠だけ)。");
                return;
            }

            char intensity[16];
            std::snprintf(intensity, sizeof(intensity), "%.2f", Weather::Sandstorm::GetIntensity01());
            Widgets::Label("強さ", intensity);

            if (Widgets::Button("砂嵐にする"))
                sandstorm->StartStorm();
            if (Widgets::Button("砂嵐を止める"))
                sandstorm->StopStorm();
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(Sandstorm, "天候/砂嵐", 50, GamePlay::Debug::DrawSandstorm)
#endif
