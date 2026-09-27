#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <functional>

#include "rx.hpp"

namespace NanamiEngine::R4
{
    class NANAMI_API CancellationToken final
    {
    public:
        CancellationToken() = default;
        explicit CancellationToken(rxcpp::composite_subscription subscription);
        CancellationToken(const CancellationToken&) = default;
        CancellationToken(CancellationToken&& other) : subscription_(other.subscription_) { }
        CancellationToken& operator=(const CancellationToken&) = default;
        CancellationToken& operator=(CancellationToken&& other) { subscription_ = other.subscription_; return *this; }

        [[nodiscard]] const rxcpp::composite_subscription& Subscription() const { return subscription_; }
        [[nodiscard]] bool IsCancellationRequested() const;
        //NOTE: キャンセル時に呼ばれる。既にキャンセル済みならその場で呼ばれる
        void Register(std::function<void()> callback) const;

    private:
        rxcpp::composite_subscription subscription_;
    };
    
    class NANAMI_API CancellationTokenSource final
    {
    public:
        CancellationTokenSource() = default;
        CancellationTokenSource(const CancellationTokenSource&) { }
        CancellationTokenSource& operator=(const CancellationTokenSource&) { return *this; }

        [[nodiscard]] CancellationToken Token() const { return CancellationToken(subscription_); }
        [[nodiscard]] bool IsCancellationRequested() const;
        void Cancel() const;

    private:
        rxcpp::composite_subscription subscription_;
    };
}
