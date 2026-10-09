#pragma once
#include <concepts>

#include "rx.hpp"
#include "../Observable/R4_Observable.h"

namespace NanamiEngine::R4
{
    template <typename T>
    class ReactiveProperty;

    // NOTE: 読み取り専用の側。状態を共有するので元が破棄された後も安全に読める
    template <typename T>
    class ReadOnlyReactiveProperty final
    {
    public:
        // NOTE: 変化しない値として作る（Null オブジェクト用）
        explicit ReadOnlyReactiveProperty(T value) : behavior_(std::move(value)) { }

        [[nodiscard]] T CurrentValue() const { return behavior_.get_value(); }
        // NOTE: 購読した瞬間に今の値も流れる
        [[nodiscard]] Observable<T> AsObservable() const { return Observable<T>(behavior_.get_observable()); }

        template <typename... ArgN>
        [[nodiscard]] Disposable Subscribe(ArgN&&... args) const
        {
            return AsObservable().Subscribe(std::forward<ArgN>(args)...);
        }

    private:
        friend class ReactiveProperty<T>;
        explicit ReadOnlyReactiveProperty(rxcpp::subjects::behavior<T> behavior) : behavior_(std::move(behavior)) { }

        rxcpp::subjects::behavior<T> behavior_;
    };

    // NOTE: 今の値を持ち、変わった時に通知する。同じ値では通知しない (必ず通知するのは ForceNotify)
    // NOTE: コピーは値だけ複製した新しい実体になり、購読者は引き継がない
    template <typename T>
    class ReactiveProperty
    {
    public:
        ReactiveProperty() requires std::default_initializable<T> : behavior_(T()) { }
        explicit ReactiveProperty(T value) : behavior_(std::move(value)) { }
        ReactiveProperty(const ReactiveProperty& other) : behavior_(other.Value()) { }
        // NOTE: rxcpp の behavior はムーブ元が空になるので、ムーブは同じ実体を共有する
        ReactiveProperty(ReactiveProperty&& other) : behavior_(other.behavior_) { }
        ReactiveProperty& operator=(const ReactiveProperty& other) { Value(other.Value()); return *this; }
        ReactiveProperty& operator=(ReactiveProperty&& other)      { Value(other.Value()); return *this; }
        ~ReactiveProperty() = default;

        [[nodiscard]] T Value() const        { return behavior_.get_value(); }
        [[nodiscard]] T CurrentValue() const { return behavior_.get_value(); }

        void Value(const T& value)
        {
            if constexpr (std::equality_comparable<T>)
            {
                if (behavior_.get_value() == value)
                    return;
            }
            OnNext(value);
        }

        // NOTE: 同じ値でも必ず通知する
        void OnNext(const T& value) { behavior_.get_subscriber().on_next(value); }
        // NOTE: 今の値をもう一度通知する
        void ForceNotify() { OnNext(Value()); }

        [[nodiscard]] Observable<T> AsObservable() const { return Observable<T>(behavior_.get_observable()); }
        [[nodiscard]] ReadOnlyReactiveProperty<T> AsReadOnly() const { return ReadOnlyReactiveProperty<T>(behavior_); }

        template <typename... ArgN>
        [[nodiscard]] Disposable Subscribe(ArgN&&... args) const
        {
            return AsObservable().Subscribe(std::forward<ArgN>(args)...);
        }

    private:
        rxcpp::subjects::behavior<T> behavior_;
    };
}
