#pragma once
#include "rx.hpp"
#include "../Observable/R4_Observable.h"

namespace NanamiEngine::R4
{
    ///NOTE: OnNext した値を購読者へ流す（R3 の Subject<T>）
    ///      持ち主がコピーされた時に購読者が連動しないよう、コピー先は購読者なしの新しい Subject から始まり、
    ///      代入しても自分の購読者はそのまま
    template <typename T>
    class Subject final
    {
    public:
        Subject() = default;
        Subject(const Subject&) { }
        // rxcpp の subject はムーブ元が空になるので、ムーブは同じ実体を共有する
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
