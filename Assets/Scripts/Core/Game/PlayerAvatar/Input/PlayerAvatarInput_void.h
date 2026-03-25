#pragma once
#include <functional>

#include "IPlayerAvatarInput.h"

namespace GameCore::PlayerAvatar
{
    template<>
    class PlayerAvatarInput<void> final : public IPlayerAvatarInput
    {
    public:
        explicit PlayerAvatarInput(std::function<bool()> checkInput)
            : checkInput_(std::move(checkInput)) {}

        void OnUpdate() override;
        [[nodiscard]] bool IsPressed()       const;
        [[nodiscard]] bool IsUpdatePressed() const;
        [[nodiscard]] bool IsReleased()      const;
        void SetActive(bool active);

    private:
        const std::function<bool()> checkInput_;
        bool isPressing_     = false;
        bool previewIsPress_ = false;
        bool isActive_       = true ;
    };

    inline void PlayerAvatarInput<void>::OnUpdate()
    {
        previewIsPress_ = isPressing_;
        isPressing_     = checkInput_();        
    }

    inline bool PlayerAvatarInput<void>::IsPressed() const
    {
        return !previewIsPress_ && isPressing_ && isActive_;
    }

    inline bool PlayerAvatarInput<void>::IsUpdatePressed() const
    {
        return isPressing_ && isActive_;
    }

    inline bool PlayerAvatarInput<void>::IsReleased() const
    {
        return previewIsPress_ && !isPressing_ && isActive_;
    }

    inline void PlayerAvatarInput<void>::SetActive(const bool active)
    {
        isActive_ = active;
    }
}
