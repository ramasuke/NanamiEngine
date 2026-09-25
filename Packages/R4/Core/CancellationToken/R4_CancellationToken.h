#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <functional>

#include "rx.hpp"

namespace NanamiEngine::R4
{
    ///NOTE: キャンセルされたかを見る/キャンセル時の処理を積むだけの側。キャンセルするのは CancellationTokenSource
    ///      コピーしても同じトークンを指す
    class NANAMI_API CancellationToken final
    {
    public:
        CancellationToken() = default;
        explicit CancellationToken(rxcpp::composite_subscription subscription);
        // ムーブもコピーとして扱う
        CancellationToken(const CancellationToken&) = default;
        CancellationToken(CancellationToken&& other) : subscription_(other.subscription_) { }
        CancellationToken& operator=(const CancellationToken&) = default;
        CancellationToken& operator=(CancellationToken&& other) { subscription_ = other.subscription_; return *this; }

        //NOTE: キャンセル時に unsubscribe される rxcpp の購読。R4 の外からは基本的に使わない
        [[nodiscard]] const rxcpp::composite_subscription& Subscription() const { return subscription_; }
        [[nodiscard]] bool IsCancellationRequested() const;
        //NOTE: キャンセル時に呼ばれる。既にキャンセル済みならその場で呼ばれる
        void Register(std::function<void()> callback) const;

    private:
        rxcpp::composite_subscription subscription_;
    };

    ///NOTE: 持ち主ごとに別のトークンを持つよう、コピー/ムーブ先は新しいソースから始まる
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
