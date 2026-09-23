#pragma once
#include "rx.hpp"
#include "../Observable/R4_Observable.h"

namespace NanamiEngine::R4
{
    ///NOTE: OnNext した値を購読者へ流す（R3 の Subject<T>）
    template <typename T>
    class Subject final
    {
    public:
        Subject() = default;
        Subject(const Subject&) { }
        // rxcpp の subject はムーブ元が空になるので、ムーブは共有
        Subject(Subject&& other) : subject_(other.subject_) { }
        Subject& operator=(const Subject&) { return *this; }
        Subject& operator=(Subject&&)      { return *this; }

        void OnNext(const T& value) const { subject_.get_subscriber().on_next(value); }
        void OnCompleted() const          { subject_.get_subscriber().on_completed(); }
        [[nodiscard]] bool HasObservers() const { return subject_.has_observers(); }

        [[nodiscard]] Observable<T> AsObservable() const { return Observable<T>(subject_.get_observable()); }

        template <typename... ArgN>
        [[nodiscard]] Disposable Subscribe(ArgN&&... args) const
        {
            return AsObservable().Subscribe(std::forward<ArgN>(args)...);
        }

    private:
        rxcpp::subjects::subject<T> subject_;
    };
}
