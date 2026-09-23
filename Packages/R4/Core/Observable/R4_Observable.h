#pragma once
#include <tuple>
#include <type_traits>

#include "rx.hpp"
#include "../CancellationToken/R4_CancellationToken.h"
#include "../Disposable/R4_Disposable.h"
#include "../Unit/R4_Unit.h"

namespace NanamiEngine::R4
{
    ///NOTE: 購読できる値の流れ（R3 の Observable<T>）。コピーしても同じ流れを指す
    template <typename T>
    class Observable final
    {
    public:
        using ValueType = T;

        Observable() : source_(rxcpp::observable<>::never<T>().as_dynamic()) { }
        explicit Observable(rxcpp::observable<T> source) : source_(std::move(source)) { }

        [[nodiscard]] static Observable Never()  { return Observable(rxcpp::observable<>::never<T>().as_dynamic()); }
        [[nodiscard]] static Observable Empty()  { return Observable(rxcpp::observable<>::empty<T>().as_dynamic()); }
        [[nodiscard]] static Observable Return(T value) { return Observable(rxcpp::observable<>::just(std::move(value)).as_dynamic()); }

#pragma region Subscribe
        template <typename OnNextF>
        [[nodiscard]] Disposable Subscribe(OnNextF&& onNext) const
        {
            const auto subscription = rxcpp::composite_subscription();
            source_.subscribe(subscription, std::forward<OnNextF>(onNext));
            return Disposable(subscription);
        }

        template <typename OnNextF, typename OnCompletedF>
        requires std::is_invocable_v<OnCompletedF&>
        [[nodiscard]] Disposable Subscribe(OnNextF&& onNext, OnCompletedF&& onCompleted) const
        {
            const auto subscription = rxcpp::composite_subscription();
            source_.subscribe(subscription, std::forward<OnNextF>(onNext), std::forward<OnCompletedF>(onCompleted));
            return Disposable(subscription);
        }
#pragma endregion

#pragma region Operators
        template <typename PredicateF>
        [[nodiscard]] Observable Where(PredicateF predicate) const
        {
            return Observable(source_.filter(std::move(predicate)).as_dynamic());
        }

        template <typename SelectorF, typename U = std::decay_t<std::invoke_result_t<SelectorF&, const T&>>>
        [[nodiscard]] Observable<U> Select(SelectorF selector) const
        {
            return Observable<U>(source_.map(std::move(selector)).as_dynamic());
        }

        [[nodiscard]] Observable Skip(const int count) const
        {
            return Observable(source_.skip(count).as_dynamic());
        }

        [[nodiscard]] Observable Take(const int count) const
        {
            return Observable(source_.take(count).as_dynamic());
        }

        template <typename U>
        [[nodiscard]] Observable TakeUntil(const Observable<U>& other) const
        {
            return Observable(source_.take_until(other.source_).as_dynamic());
        }

        //NOTE: token がキャンセルされたら完了する
        [[nodiscard]] Observable TakeUntil(const CancellationToken& token) const
        {
            const auto cancelled = rxcpp::observable<>::create<Unit>([token](rxcpp::subscriber<Unit> subscriber)
            {
                const auto registration = rxcpp::composite_subscription();
                // 購読側が先に解除された時もここが呼ばれるので、そのときは流さない
                registration.add([subscriber]
                {
                    if (subscriber.is_subscribed())
                    {
                        subscriber.on_next(Unit{ });
                        subscriber.on_completed();
                    }
                });
                subscriber.add(registration);
                Detail::Attach(token.Subscription(), registration);
            });
            return Observable(source_.take_until(cancelled).as_dynamic());
        }

        [[nodiscard]] Observable DistinctUntilChanged() const
        {
            return Observable(source_.distinct_until_changed().as_dynamic());
        }

        //NOTE: 購読した瞬間に value を先に流す
        [[nodiscard]] Observable Prepend(T value) const
        {
            return Observable(source_.start_with(std::move(value)).as_dynamic());
        }

        //NOTE: 値を流す前に action を挟む（値は変えない）
        template <typename ActionF>
        [[nodiscard]] Observable Do(ActionF action) const
        {
            return Observable(source_.tap(std::move(action)).as_dynamic());
        }

        //NOTE: (一つ前の値, 今の値)
        [[nodiscard]] Observable<std::tuple<T, T>> Pairwise() const
        {
            return Observable<std::tuple<T, T>>(source_.pairwise().as_dynamic());
        }

        template <typename... Others>
        requires (std::is_same_v<Others, Observable> && ...)
        [[nodiscard]] Observable Merge(const Others&... others) const
        {
            return Observable(source_.merge(others.source_...).as_dynamic());
        }

        template <typename U, typename SelectorF, typename R = std::decay_t<std::invoke_result_t<SelectorF&, const T&, const U&>>>
        [[nodiscard]] Observable<R> CombineLatest(const Observable<U>& other, SelectorF selector) const
        {
            return Observable<R>(source_.combine_latest(std::move(selector), other.source_).as_dynamic());
        }
#pragma endregion

    private:
        template <typename> friend class Observable;
        rxcpp::observable<T> source_;
    };
}
