#pragma once
#include "../../Libs/rxcpp/rx-observable.hpp"

namespace LibCore::Rx
{
    ///NOTE: 値オブジェクト推奨
    template <typename T>
    struct ReadOnlyReactiveContext final
    {
        explicit ReadOnlyReactiveContext(const T& value, rxcpp::observable<T> observable);
        const T& Value() const;
        template <typename... ArgN>
        rxcpp::composite_subscription Subscribe(ArgN&&... args) const;

    private:
        const T& value_;
        const rxcpp::observable<T> observable_;
    };

    template <typename T>
    ReadOnlyReactiveContext<
        T>::ReadOnlyReactiveContext(const T& value, rxcpp::observable<T> observable) : value_(value),
        observable_(observable)
    {
    }

    template <typename T>
    const T& ReadOnlyReactiveContext<T>::Value() const
    {
        return value_;
    }

    template <typename T>
    template <typename... ArgN>
    rxcpp::composite_subscription ReadOnlyReactiveContext<T>::Subscribe(ArgN&&... args) const
    {
        return observable_
               .start_with(value_)
               .subscribe(std::forward<ArgN>(args)...);
    }
}
