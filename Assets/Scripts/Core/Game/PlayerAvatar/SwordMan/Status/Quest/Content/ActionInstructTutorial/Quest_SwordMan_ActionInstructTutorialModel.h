#pragma once

namespace GameCore::PlayerAvatar::SwordMan
{
    class IObservableStatusEvent;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class ActionInstructTutorialModel final
    {
    public:
        explicit ActionInstructTutorialModel(const IObservableStatusEvent& statusEvent);

        [[nodiscard]] const IObservableStatusEvent& StatusEvent() const { return _statusEvent; }
        
    private:
        const IObservableStatusEvent& _statusEvent;
    };
}
