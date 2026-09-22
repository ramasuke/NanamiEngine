#include "R4_Disposable.h"

namespace NanamiEngine::R4
{
    void Detail::Attach(const rxcpp::composite_subscription& parent, const rxcpp::composite_subscription& child)
    {
        if (!child.is_subscribed())
            return;

        if (!parent.is_subscribed())
        {
            child.unsubscribe();
            return;
        }

        const auto weak = parent.add(child);
        child.add([parent, weak]
        {
            parent.remove(weak);
        });
    }

    Disposable::Disposable(rxcpp::composite_subscription subscription)
        : subscription_(std::move(subscription))
    {
    }

    Disposable Disposable::Create(std::function<void()> onDispose)
    {
        const auto subscription = rxcpp::composite_subscription();
        subscription.add([onDispose = std::move(onDispose)]
        {
            onDispose();
        });
        return Disposable(subscription);
    }

    void Disposable::Dispose() const
    {
        if (subscription_.is_subscribed())
            subscription_.unsubscribe();
    }

    bool Disposable::IsDisposed() const
    {
        return !subscription_.is_subscribed();
    }

    const Disposable& Disposable::AddTo(const CompositeDisposable& composite) const
    {
        Detail::Attach(composite.subscription_, subscription_);
        return *this;
    }

    const Disposable& Disposable::RegisterTo(const CancellationToken& token) const
    {
        Detail::Attach(token.Subscription(), subscription_);
        return *this;
    }

    void CompositeDisposable::Add(const Disposable& disposable) const
    {
        disposable.AddTo(*this);
    }

    void CompositeDisposable::Dispose() const
    {
        if (subscription_.is_subscribed())
            subscription_.unsubscribe();
    }

    void CompositeDisposable::Clear() const
    {
        subscription_.clear();
    }

    bool CompositeDisposable::IsDisposed() const
    {
        return !subscription_.is_subscribed();
    }

    SerialDisposable::SerialDisposable(SerialDisposable&& other)
        : current_(other.current_)
    {
        other.current_ = Disposable();
    }

    SerialDisposable& SerialDisposable::operator=(SerialDisposable&& other)
    {
        if (this != &other)
        {
            Set(other.current_);
            other.current_ = Disposable();
        }
        return *this;
    }

    SerialDisposable::~SerialDisposable()
    {
        Dispose();
    }

    void SerialDisposable::Set(Disposable disposable)
    {
        const auto old = current_;
        current_ = disposable;
        old.Dispose();
    }

    void SerialDisposable::Dispose()
    {
        current_.Dispose();
    }
}
