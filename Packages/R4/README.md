# R4 — rxcpp を R3 風に包んだリアクティブ層

`namespace NanamiEngine::R4`。グローバルに `using namespace NanamiEngine;` があるので `R4::Subject<T>` と書ける。
ゲーム/エンジンのコードは rxcpp を直接使わず `Packages/R4/R4.h` を include する
（`rxcpp::` と書いてよいのは `Packages/R4/Core/` の中だけ。`stdafx.h` の `rx.hpp` はプリコンパイル用）。

## 型

| R4 | R3 | 中身 | 主な API |
|---|---|---|---|
| `Observable<T>` | `Observable<T>` | `rxcpp::observable<T>` | `Subscribe(onNext[, onCompleted])` → `Disposable`、`Never()` / `Empty()` / `Return(v)` |
| `Subject<T>` | `Subject<T>` | `rxcpp::subjects::subject<T>` | `OnNext` / `OnCompleted` / `AsObservable` / `Subscribe` |
| `ReactiveProperty<T>` | `ReactiveProperty<T>` | `rxcpp::subjects::behavior<T>` | `Value()` / `Value(v)` / `OnNext(v)` / `ForceNotify()` / `AsReadOnly()` / `Subscribe` |
| `ReadOnlyReactiveProperty<T>` | `ReadOnlyReactiveProperty<T>` | 同上（状態を共有） | `CurrentValue()` / `AsObservable()` / `Subscribe` |
| `SerializableReactiveProperty<T>` | `SerializableReactiveProperty<T>` | `ReactiveProperty<T>` + cereal | 保存形式は `{"value": ...}`、インスペクタで編集・`onNext` ボタン |
| `Unit` | `Unit` | 空の struct | 値なしの通知 |
| `Disposable` | `IDisposable` | `rxcpp::composite_subscription` | `Dispose()` / `AddTo(this)` / `AddTo(CompositeDisposable)` / `RegisterTo(token)` |
| `CompositeDisposable` | `CompositeDisposable` | 同上 | `Add` / `Dispose` / `Clear` |
| `SerialDisposable` | `SerialDisposable` | 同上 | `Set(d)` で差し替え（古い方を Dispose）、持ち主の破棄時にも Dispose |
| `CancellationToken` | `CancellationToken` | 同上 | `IsCancellationRequested()` / `Register(fn)` / `Subscription()` |
| `CancellationTokenSource` | `CancellationTokenSource` | 同上 | `Token()` / `Cancel()` |

オペレータ（`Observable<T>` のメンバ）: `Where` / `Select` / `Skip` / `Take` / `TakeUntil(Observable)` /
`TakeUntil(CancellationToken)` / `DistinctUntilChanged` / `Prepend` / `Do` / `Pairwise` / `Merge` / `CombineLatest`。
フレーム・時間系（`EveryUpdate` / `Timer` / `ThrottleFirst` など）はまだ無い。

## 購読の寿命

`Subscribe` の戻り値 `Disposable` は `[[nodiscard]]`。捨てると購読が解除されないまま残るので、必ず持ち主を決める。

```cpp
// Component: 破棄時（ComponentGroup::OnDestroy で OnDestroy() の後）に解除される
button->OnClick().Subscribe([this](NanamiUi::MouseState) { Decide(); }).AddTo(this);

// 破棄時の後始末だけ積む
boss.DestroyCancellationToken().Register([weakSelf] { ... });

// 張り替える購読（再 Initialize など）
subscription_.Set(model.OnChangeHealth().Subscribe([this](Health h) { ... }));   // R4::SerialDisposable

// Component でないもの: メンバに持ってデストラクタで Dispose()
newPeerSubscription_ = networkSystem_.OnConnectPlayer().Subscribe(...);         // R4::Disposable
```

## 挙動の注意

- `ReactiveProperty::Value(v)` は `==` で比較できる型なら同じ値のとき通知しない（R3 と同じ）。
  必ず通知したいときは `OnNext(v)`、今の値をもう一度流すときは `ForceNotify()`。
- `ReactiveProperty` / `ReadOnlyReactiveProperty` は購読した瞬間に今の値を流す。`Subject` は流さない。
- 持ち主のコピーで購読者が連動しないよう、`Subject` / `ReactiveProperty` のコピーは購読者なしの新しい実体
  （`ReactiveProperty` は値だけ複製）から始まり、代入しても自分の購読者はそのまま。
  `CancellationTokenSource` のコピーも新しいソース。`Disposable` / `CancellationToken` / `Observable` はハンドルで、
  コピーしても同じものを指す。
- rxcpp の購読はムーブ元が空になり、触ると `std::terminate` するので、R4 のハンドル型はムーブもコピーとして扱う。
- `AddTo` / `RegisterTo` は、先に終わった購読（`Take(1)` など）を親から外す処理も積む
  （rxcpp の `composite_subscription::add` は子が先に解除されても親に残し続けるため）。
- 購読中の例外は rxcpp のまま（`OnErrorResume` 相当は無い）。

## センサー

`SensorEnterableAsObservable` / `SensorExitableAsObservable` / `SensorStayableAsObservable` は物理センサーの
enter / exit / stay を `OnAction()` で流す Component。
