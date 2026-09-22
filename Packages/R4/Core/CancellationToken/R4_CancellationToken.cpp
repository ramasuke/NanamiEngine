#include "R4_CancellationToken.h"

namespace NanamiEngine::R4
{
    CancellationToken::CancellationToken(rxcpp::composite_subscription subscription)
        : subscription_(std::move(subscription))
    {
    }

    bool CancellationToken::IsCancellationRequested() const
    {
        return !subscription_.is_subscribed();
    }

    void CancellationToken::Register(std::function<void()> callback) const
    {
        // 解除済みの composite に add すると、その場で解除（= callback 呼び出し）される
        subscription_.add([callback = std::move(callback)]
        {
            callback();
        });
    }

    bool CancellationTokenSource::IsCancellationRequested() const
    {
        return !subscription_.is_subscribed();
    }

    void CancellationTokenSource::Cancel() const
    {
        if (subscription_.is_subscribed())
            subscription_.unsubscribe();
    }
}
