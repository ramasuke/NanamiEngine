# ControlLock — 操作ロックの集約

`namespace NanamiEngine::ControlLock`。使う側は `Packages/ControlLock/ControlLock.h` だけ include する。

「プレイヤーの操作を止めたい」という要求を集めて数える。誰かが持っている間はロック中で、全員が返すと解ける。
画面・演出・BT・RPC が別々に止めても、先に終わった側がほかの分まで解いてしまうことがない。

ここは要求を集めるだけで、プレイヤーを知らない（`Packages/` は `Assets/` を include できない）。実際に止めるのは
ゲーム側で、`IsLocked()` を見て行う。

## ロックを取る

```cpp
#include "Packages/ControlLock/ControlLock.h"

// Component: 自分が破棄されるときに返る
NanamiEngine::ControlLock::Service::Instance().Acquire().AddTo(this);

// コルーチンのローカル / Component でないクラスのメンバ: スコープを抜けるか、破棄で返る
NanamiEngine::ControlLock::ScopedLock lock(NanamiEngine::ControlLock::Service::Instance().Acquire());
lock.Release();   // 途中で返すとき

// Lock と Unlock が別々に呼ばれる相手 (BT のノード、RPC)
service.AcquireKeyed(key).AddTo(owner);   // 同じ key で重ねて呼んでも 1 つ
service.ReleaseKeyed(key);
```

- 戻り値は `R4::Disposable`。**`Dispose` するまで持ち続ける**ので、必ず寿命を決める（`AddTo` / `ScopedLock` /
  `SerialDisposable`）。`R4::Disposable` 自体はデストラクタで返さない。
- `AcquireKeyed` も `AddTo` で持ち主の寿命に結ぶ。Unlock まで届かずに持ち主が消えても返る。
- メニュー画面は自分で取らない。`UiFlow::UiScreen` の `locksPlayerControl_` が開閉に合わせて取る。

## 取得元の記録

`Acquire()` / `AcquireKeyed()` は呼び出し元を `std::source_location` で記録する。`Holders()` で、いま誰が止めているかを
ファイル・関数・行で確かめられる（`UiScreen` が取ったものは `label` に `screenId_`）。

`NANAMI_CONTROL_LOCK_TRACE_ENABLED`（`ControlLockConfig.h`）はエディタと **Debug** 構成のゲームビルドで 1、Release の
ゲームビルドで 0。0 のときは引数ごと無くなり、ソースのフルパスが exe に入らない。呼ぶ側は `Acquire()` と書くだけで、
`#if` は要らない。

## IsLocked() と HasHolder()

| | 返すもの |
|---|---|
| `HasHolder()` | いま持ち主がいるか |
| `IsLocked()` | 持ち主がいるか、**最後の持ち主が返したフレームの間** |

画面を閉じるのに使ったキー（B など）を、解けた直後のゲーム操作が拾わないよう、`IsLocked()` は返したフレームの間だけ
ロック中のままにする。止める側は `IsLocked()` を見る。

## ゲーム側で止める

このプロジェクトでは `PlayerAvatarBase` が手元のアバターについて毎フレーム確かめ、`PlayerAvatarStateMachineBase::
ApplyControlLock` が State を差し替える。

- ロック中は、操作を受ける State（`ControlAcceptance()` が `None` 以外）を Disable State へ替える。
- 倒れている・演出で動かしているなど、もともと操作を受けない State（Death / Down / WarpIn など）には触らない。
  演出はロックを持ったまま `GetEventSceneStateMachine().OnChangeState(...)` で動きを差し替えられる。
- 解けたら、Disable State にいるときだけ初期 State へ戻す。
- 解くのは `OnUpdate` で入力を更新した後だけ。State の遷移は `OnFixedUpdate` で判定されるので、先に解くと
  古い「押した瞬間」を拾う。

**State を直接 Disable に替えない。** 止めたいときはロックを取る。

## プレイ終了とホットリロード

`Service::Clear()` が全てのロックを捨てる。`GameCore::Game::OnDestroy` と `GameModule::Reload` が呼ぶ。捨てた後に、
残っていた `Disposable` が `Dispose` されても何も起きない。

エンジン側に残るのは id と文字列だけで、Game.dll のコードを指すものは持たない。
