#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <concepts>
#include <functional>

#include "rx.hpp"
#include "../CancellationToken/R4_CancellationToken.h"

namespace NanamiEngine::R4
{
    class CompositeDisposable;

    namespace Detail
    {
        //NOTE: child を parent に積む。rxcpp の add は child が先に解除されても parent から外さないので、
        //      外す処理も child 側に積んでおく（Take(1) などで終わった購読が溜まり続けないように）
        NANAMI_API void Attach(const rxcpp::composite_subscription& parent, const rxcpp::composite_subscription& child);
    }

    ///NOTE: 購読の寿命。Dispose() で購読解除。コピーしても同じ購読を指す
    ///      Subscribe の戻り値は必ず AddTo / RegisterTo するか、メンバに持って自分で Dispose() する
    class NANAMI_API Disposable final
    {
    public:
        Disposable() = default;
        explicit Disposable(rxcpp::composite_subscription subscription);
        // rxcpp の購読はムーブ元が空になり、触ると std::terminate するので、ムーブもコピーとして扱う
        Disposable(const Disposable&) = default;
        Disposable(Disposable&& other) : subscription_(other.subscription_) { }
        Disposable& operator=(const Disposable&) = default;
        Disposable& operator=(Disposable&& other) { subscription_ = other.subscription_; return *this; }
        [[nodiscard]] static Disposable Create(std::function<void()> onDispose);

        void Dispose() const;
        [[nodiscard]] bool IsDisposed() const;

        const Disposable& AddTo(const CompositeDisposable& composite) const;
        const Disposable& RegisterTo(const CancellationToken& token) const;

        //NOTE: DestroyCancellationToken() を持つもの（Component）が破棄される時に解除する
        template <typename OwnerT>
        requires requires(const OwnerT& owner) { { owner.DestroyCancellationToken() } -> std::convertible_to<CancellationToken>; }
        const Disposable& AddTo(const OwnerT* owner) const
        {
            return RegisterTo(owner->DestroyCancellationToken());
        }

        template <typename OwnerT>
        requires requires(const OwnerT& owner) { { owner.DestroyCancellationToken() } -> std::convertible_to<CancellationToken>; }
        const Disposable& AddTo(const OwnerT& owner) const
        {
            return RegisterTo(owner.DestroyCancellationToken());
        }

        [[nodiscard]] const rxcpp::composite_subscription& Subscription() const { return subscription_; }

    private:
        rxcpp::composite_subscription subscription_;
    };

    ///NOTE: 複数の Disposable をまとめて Dispose する。コピーしても同じ入れ物を指す
    class NANAMI_API CompositeDisposable final
    {
    public:
        CompositeDisposable() = default;
        CompositeDisposable(const CompositeDisposable&) = default;
        CompositeDisposable(CompositeDisposable&& other) : subscription_(other.subscription_) { }
        CompositeDisposable& operator=(const CompositeDisposable&) = default;
        CompositeDisposable& operator=(CompositeDisposable&& other) { subscription_ = other.subscription_; return *this; }

        void Add(const Disposable& disposable) const;
        //NOTE: 中身をすべて Dispose する。以降に Add したものは即 Dispose される
        void Dispose() const;
        //NOTE: 中身をすべて Dispose するが、この入れ物は引き続き使える
        void Clear() const;
        [[nodiscard]] bool IsDisposed() const;

    private:
        friend class Disposable;
        rxcpp::composite_subscription subscription_;
    };

    ///NOTE: 持っている Disposable を差し替えると古い方を Dispose する（購読の張り替え用）
    ///      持ち主の寿命に合わせて、破棄時にも Dispose する。コピーは空から始まり、ムーブは中身を引き継ぐ
    class NANAMI_API SerialDisposable final
    {
    public:
        SerialDisposable() = default;
        SerialDisposable(const SerialDisposable&) { }
        SerialDisposable(SerialDisposable&& other);
        SerialDisposable& operator=(const SerialDisposable&) { return *this; }
        SerialDisposable& operator=(SerialDisposable&& other);
        ~SerialDisposable();

        void Set(Disposable disposable);
        void Dispose();

    private:
        Disposable current_;
    };
}
