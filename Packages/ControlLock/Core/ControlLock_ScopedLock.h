#pragma once
#include "Engine/Core/Api/NanamiApi.h"

#include "ControlLock_Service.h"

namespace NanamiEngine::ControlLock
{
    // NOTE: スコープを抜けるとロックを返す。コルーチンのローカルや、Component でないクラスのメンバに使う
    // NOTE: R4::Disposable はデストラクタで Dispose しないので、寿命で返したいときはこれで包む
    class NANAMI_NO_API ScopedLock final
    {
    public:
        ScopedLock() = default;
        explicit ScopedLock(const R4::Disposable& token) { Set(token); }
        ScopedLock(const ScopedLock&)            = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;
        ScopedLock(ScopedLock&& other) noexcept
            : token_ (std::move(other.token_))
            , isHeld_(other.isHeld_)
        {
            other.isHeld_ = false;
        }
        ScopedLock& operator=(ScopedLock&& other) noexcept
        {
            if (this != &other)
            {
                token_        = std::move(other.token_);
                isHeld_       = other.isHeld_;
                other.isHeld_ = false;
            }
            return *this;
        }
        ~ScopedLock() = default;

        // NOTE: 持っているロックを返してから、新しいロックを持つ
        void Set(const R4::Disposable& token)
        {
            token_.Set(token);
            isHeld_ = true;
        }

        void Release()
        {
            token_.Dispose();
            isHeld_ = false;
        }

        [[nodiscard]] bool IsHeld() const { return isHeld_; }

    private:
        // NOTE: 破棄時に Dispose する
        R4::SerialDisposable token_;
        bool                 isHeld_ = false;
    };
}
