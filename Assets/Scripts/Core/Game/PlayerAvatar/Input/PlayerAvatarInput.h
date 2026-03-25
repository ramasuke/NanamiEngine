#pragma once
#include <functional>
#include "IPlayerAvatarInput.h"

namespace GameCore::PlayerAvatar
{
    template<typename T>
    class PlayerAvatarInput;

    template<>
    class PlayerAvatarInput<void>;

    template<typename T>
    class PlayerAvatarInput : public IPlayerAvatarInput
    {
    public:
        PlayerAvatarInput(std::function<bool()> checkInput, std::function<T()> readValue);

        void OnUpdate() override;
        void SetActive(bool active);
        [[nodiscard]] bool IsPressed      () const;
        [[nodiscard]] bool IsUpdatePressed() const;
        [[nodiscard]] bool IsReleased     () const;
        [[nodiscard]] T    ReadValue      () const;

    private:
        const std::function<bool()> checkInput_;
        const std::function<T   ()> readValue_;
        bool isPressing_     = false;
        bool previewIsPress_ = false;
        bool isActive_       = true ;
    };
    
    template<typename T>
    PlayerAvatarInput<T>::PlayerAvatarInput(std::function<bool()> checkInput, std::function<T()> readValue)
        : checkInput_(std::move(checkInput)), readValue_(std::move(readValue)) {}

    template<typename T>
    void PlayerAvatarInput<T>::OnUpdate()
    {
        previewIsPress_ = isPressing_;
        isPressing_     = checkInput_();
    }

    template<typename T>
    bool PlayerAvatarInput<T>::IsPressed() const
    {
        return !previewIsPress_ && isPressing_ && isActive_;
    }

    template<typename T>
    bool PlayerAvatarInput<T>::IsUpdatePressed() const
    {
        return isPressing_ && isActive_;
    }

    template<typename T>
    bool PlayerAvatarInput<T>::IsReleased() const
    {
        return previewIsPress_ && !isPressing_ && isActive_;
    }

    template<typename T>
    T PlayerAvatarInput<T>::ReadValue() const
    {
        return readValue_();
    }

    template<typename T>
    void PlayerAvatarInput<T>::SetActive(const bool active)
    {
        isActive_ = active;
    }
}
