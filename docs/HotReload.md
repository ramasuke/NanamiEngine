# ゲームコードのホットリロード (案C) 実現可能性調査

エディタを起動したまま `Assets/Scripts` を再ビルドして差し替えるために、現在の
「NanamiEngine (静的 lib) + EnviroHunter (exe)」を

```
Host exe (WinMain のみ)  ->  NanamiEngine.dll  <-  Game.dll (Assets/Scripts)
                                                  ^ エディタが LoadLibrary / FreeLibrary で差し替える
```

の 3 モジュール構成に分けられるかを、2026-09-25 時点のソースで調査した結果。
ここに書いた事実はすべて `grep` / ファイル読解で確認したもの。未検証の箇所は「要 PoC」と明記する。

**進捗**: 段階 A (多相登録の入口を `NANAMI_REGISTER_TYPE` に統一し、`SerializationTypeRegistry` に記録) は実装済み (§3.4)。
それ以外 (PoC / 段階 0-3) は未着手。レビューで出た追加の検討事項は §12。

---

## 0. 結論

**実現可能。ただし 2 つの前提作業と、1 つの PoC 必須項目がある。**

| # | 項目 | 判定 | 根拠 (詳細は各節) |
|---|---|---|---|
| 1 | CRT を /MT から /MD へ | 可 (作業あり) | DxLib は `_DLL` 定義で MD 版を自動選択。**Effekseer 系 8 lib は MT 版しか無く、/MD で再ビルドが必須** |
| 2 | サードパーティ lib のグローバル状態の二重化 | 可 (作業あり) | ゲームからの DxLib 直接呼び出し 34 ファイルを排除する必要。ImGui は `IMGUI_API` で import 可 |
| 3 | エンジン内 static の二重化 | 可 | `SingletonBase` 利用 7 クラスを `.cpp` 化。cereal の `StaticObject` は **vendored 版へのパッチが必要 (要 PoC)** |
| 4 | 登録解除の欠如 | 可 (作業あり) | ゲームからエンジンへ登録する経路 7 種、いずれも Unregister 無し。全て追加が必要 |
| 5 | アンロード前の参照掃除 | 可 | `GameWindow::End()` が土台。残りは ScriptableObject アセットと期限切れ `weak_ptr` の purge |
| 6 | export 面の規模 | 可 | 非テンプレートクラス 382、メンバ定義 2,911 で MSVC の 65,535 export 上限に余裕。`.def` 全出力方式は不可 |
| 7 | DLL 境界の言語機能 (RTTI / 例外 / type_index) | 可 | MSVC は `type_info` を名前で比較するので `dynamic_cast` / `std::type_index` は跨げる |
| 8 | 差し替え手順の挿入点 | 可 | `ApplicationBase::Run` の `ScreenFlip` 直後 (`WindowDisplayModeController::OnFrameEnd` と同じ位置) |
| 9 | 配布 (engine_dist / NanamiHub) への影響 | 可 | 出荷ビルド (`NanamiApplicationMode=Game`) は静的リンクのまま残せる |

**事前に潰すべき最大のリスクは 3 の cereal パッチ**で、これが成立しないと Scene / Prefab のロードでゲーム側 Component を復元できない。
設計案は §3.2 にあり、cereal の公開データ構造だけで実装できる見込みだが、動く実物で確認してから他の作業に着手すべき。

---

## 1. CRT: /MT -> /MD

### 現状
- `NanamiEngine.props:46,52` で `RuntimeLibrary` が `MultiThreadedDebug` / `MultiThreaded` (静的 CRT)。
- 2 モジュール間で `std::string` / `std::vector` / `shared_ptr` を値渡しし、相手側で破棄する箇所が
  `ComponentBase` / `Field<T>` / `Scene` など至る所にあるため、**ヒープを共有する /MD は必須**。/MT のままでは
  片方の CRT が確保したメモリをもう片方が解放して即クラッシュする。

### サードパーティ lib の CRT 種別 (lib 内の `/DEFAULTLIB` 指示を `strings` で確認)

| lib | ビルド形態 | MD 版 | 備考 |
|---|---|---|---|
| DxLib (`DxLibW_vs2015_x64_MD(d).lib` ほか 20 種) | 同梱 lib | **あり** | `DxDataTypeWin.h` の `#pragma comment(lib)` が `_DLL` 定義時 (= /MD) に `_MD` 版を自動選択する。設定変更だけで切り替わる |
| Effekseer / EffekseerRendererDX9 / DX11 / EffekseerForDXLib (`*_vs2019_x64(_d).lib`, 計 8) | 同梱 lib | **なし (`LIBCMT` のみ)** | /MD 混在は `LNK2038 RuntimeLibrary mismatch` になる。EffekseerForDXLib はソース公開 (MIT) なので `RuntimeLibrary` を変えて再ビルドする。`EffekseerForDXLib.h:18-24` の自動リンクも MD 版名に合わせて差し替える |
| Jolt | **ソースビルド** (`NanamiEngine.vcxproj` に 133 `.cpp`) | 不要 | `Libs/JoltPhysics/Jolt.lib` (71 MB) は `<Content Include>` のみで **リンクされていない** |
| ImGui / ImGuizmo / enet / glm / meshoptimizer | ソースビルド | 不要 | props 変更で追従 |
| cereal / rxcpp / tweeny | ヘッダのみ | 不要 | |

### 判定
可。Effekseer の再ビルドが唯一の追加作業。/MD 化は静的リンクのまま先に行い、4 構成 (Editor/Game × Debug/Release) が動くことを確認してから DLL 化へ進める。

---

## 2. サードパーティ lib のグローバル状態が 2 モジュールに分裂する問題

静的 lib を Game.dll にもリンクすると、その lib のグローバル状態 (DxLib のハンドル表、ImGui の `GImGui`、
Effekseer のマネージャ) が **エンジン側と別物として 2 つできる**。ゲーム側の呼び出しは初期化されていない
方のコピーを叩くことになり、リンクは通るのに実行時に壊れる。ゲームコードからの直接利用を洗った結果:

| lib | ゲーム側の直接利用 | 対策 |
|---|---|---|
| DxLib | **34 ファイル、約 150 呼び出し** (`CheckHitKey` 60、`VGet` 15、`SetDrawBlendMode` 14、`GetJoypadXInputState` 14、`GetNowCount` 13、`DrawRotaGraphF` 10、ほか `PlaySoundMem` / `DrawLine3D` / `ConvWorldPosToScreenPos` / `MV1SearchFrame` など) | ゲームコードから DxLib を **完全に排除**し、エンジンのラッパー経由にする (入力 / 時間 / 2D 描画 / サウンド / 座標変換)。CLAUDE.md の「エンジンヘッダは DxLib を露出しない」の延長。Game.dll 側は `DX_LIB_NOT_DEFAULTPATH` を定義して DxLib の自動リンクを切れば (`DxDataTypeWin.h:29`)、取り残しは **リンクエラーとして検出できる** |
| ImGui | **121 ファイル** | ImGui は公式に DLL 境界対応 (`IMGUI_API`)。エンジン側 `IMGUI_API=__declspec(dllexport)`、ゲーム側 `dllimport` をプリプロセッサ定義で与える (`Libs/ImGui/ImGuiHelper.h:76` に `#ifndef IMGUI_API` ガードあり)。`imgui_internal.h` の `GImGui` も `IMGUI_API` 付き |
| ImGuizmo | 1 ファイル (`Assets/Data/GrassField/Data_GrassField.cpp`) | 全 API が `IMGUI_API` 付き (40 箇所) なので ImGui と同じ扱いで解決 |
| enet | 1 ファイル (`GamePlay/Network/Relay/EnetRelayNetworkSystem.cpp`) | enet は C でグローバル状態が小さいが、`enet_initialize` の時刻基準を共有するためエンジン側から re-export するか、リレー通信の enet 呼び出しをエンジン側 (`Engine/Module/Network`) に寄せる |
| Effekseer | 0 ファイル | Game.dll の PCH から `EffekseerForDXLib.h` を外す (自動リンク pragma を巻き込まないため) |
| Jolt | 0 ファイル (Jolt を露出するエンジンヘッダを include するのは `Prop_StoryMovieParts.cpp` の 1 件のみ) | Jolt は `JPH_SHARED_LIBRARY` / `JPH_BUILD_SHARED_LIBRARY` で export 対応済み (`Jolt/Core/Core.h:245-278`)。ゲームから非 inline の Jolt シンボルを使えばリンクエラーになるので検出可能 |

**検出の仕組みが重要**: Game.dll が「エンジンの import lib **だけ**」をリンクし、DxLib / Effekseer の自動リンクを切っておけば、
非 export シンボルの利用は必ずリンクエラーになる。サイレントな二重化は自動リンク pragma 経由でしか起きない。

### 判定
可。DxLib 排除 (34 ファイル) が主作業。既存ラッパーの有無は実装時に棚卸しする (`CheckHitKey` / `GetJoypadXInputState` はエンジン側では
`WindowDisplayMode.cpp` と `Editor3DCamera.cpp` しか使っておらず、ゲーム向け入力ラッパーは現状無い)。

---

## 3. エンジン内の static がモジュールごとに二重化する問題

### 3.1 `SingletonBase<T>` (`Libs/Singleton/LibCore_SingletonBase.h:11`)

関数ローカル static をテンプレートのヘッダで定義しているため、exe と DLL で **別インスタンス**になる。
ゲーム側の `REGISTER_ASSET` などが Game.dll 側のコピーへ登録し、エンジンから見えない。利用箇所は 7 クラス:

`EditorToolbarWidgetRegistry`, `PopupWindowFactory`, `MainWindowFactory`, `AssetFactory`,
`RpcHandlerRegistry`, `PacketTypeNameRegistry`, `DebugSheet`

対策: 各クラスの `Instance()` を `.cpp` に出して export する (テンプレート基底をやめる)。
`LocalPrefsRegistry::GetInstance()` / `AutoMcpServer::Instance()` / `GameBuilder::Instance()` は既に `.cpp` 定義で問題なし。
クラス static データメンバ (`ApplicationConfiguration` 群、`ApplicationBase::physics_` など約 40 個) も `.cpp` 定義なので
クラス単位の `dllexport` で自動的に単一化される。

### 3.2 cereal の `StaticObject<T>` (`Libs/cereal/include/cereal/details/static_object.hpp:66`) — **要 PoC**

cereal は多相ポインタの保存・復元に、モジュールごとの関数ローカル static を使う:

| StaticObject の中身 | キー | 値 | ゲームのアンロードで dangling になるか |
|---|---|---|---|
| `InputBindingMap<Archive>::map` (JSON / PortableBinary の 2 つ) | `std::string` (型名) | 関数ポインタ | **なる** |
| `OutputBindingMap<Archive>::map` (同 2 つ) | `std::type_index` | 関数ポインタ | **なる** |
| `PolymorphicCasters::map` / `reverseMap` | `std::type_index` | `PolymorphicVirtualCaster const*` (静的オブジェクト、vtable あり) | **なる** |
| `Versions::mapping` | 型名ハッシュ | `uint32_t` | ならない (ただし後述) |

`CEREAL_DLL_EXPORT` は `dllexport` を付けるだけで、**import 側は無い**。そのため現状のままだと:
- エンジン側 (`Scene` / `PrefabGameObjectFile` のロード) がゲーム Component を復元しようとしても、エンジンの map にゲーム型が無い。
- ゲーム側がエンジン型 (`shared_ptr<IGameObject>` など) を保存しようとしても、ゲームの map にエンジン型が無い。
つまり **双方向にマージが必要**で、片方向のコピーでは足りない。

設計案 (cereal は vendored なので改変できる):
1. `StaticObject<T>::create()` を、エンジン DLL が export する `void*& NanamiSharedStaticSlot(const char* typeName)` 経由で
   1 つの実体を返すように書き換える (約 20 行)。これで全モジュールが同じ map を見る。
2. `FreeLibrary` 前に、`SerializationTypeRegistry` (§3.4) の Game.dll のモジュールの記録を引いて、cereal の map から消す
   (`StaticObject<...>::getInstance().map` は public)。キーは記録にそのまま入っている:
   `InputBindingMap` は `name`、`OutputBindingMap` は `type`、`PolymorphicCasters` の `map` / `reverseMap` は `type` と `base`。
   (当初案の「`LoadLibrary` 前後のキー集合の差分」は不要になった。記録漏れの保険として、`PolymorphicCasters` の値のポインタが
   Game.dll のアドレス範囲にあるものを消すチェックは残してよい)
3. `Versions::find` は `emplace` なので (`helpers.hpp:415`)、ホットリロードの合間に `CEREAL_CLASS_VERSION` を上げても
   古い値が残る。`Versions::mapping` は型ハッシュ -> 番号だけで、保存時に `find(hash, Version<T>::version)` が
   無ければ入れ直す (`cereal.hpp:602`) ので、`FreeLibrary` の後・`LoadLibrary` の前に **mapping を丸ごと clear** すればよい。
   多相でないゲーム型 (`Field<T>` の中身の構造体など) のバージョンも `SerializationTypeRegistry` には載らないので、
   型ごとに消すより丸ごと clear のほうが漏れがない。

登録は Game.dll の静的初期化 (DllMain のローダーロック中) で走るが、既にロード済みのエンジン DLL の export を呼ぶだけなので問題ない。

### 3.3 その他
- rxcpp: `Packages/R4/Core` はスケジューラ (`observe_on` / `current_thread`) を一切使っていない。`rx-scheduler.hpp:32` の `shared_empty`
  が二重化するだけで無害。
- `REGISTER_LOCAL_PREF_WITH_PATH` の `inline static` 登録子 (`Engine_Module_LocalPrefs_Editor_ToolBar.h:136,176`) はモジュールごとに
  1 個ずつ走るのが正しい挙動なので問題なし (登録先は §3.1 で単一化される)。
- `ApplicationLifeCycle.cpp:6` の `thread_local` はエンジン `.cpp` 内なので単一。

### 3.4 段階 A: 多相登録の記録 (実装済み)

`CEREAL_REGISTER_TYPE` / `CEREAL_REGISTER_POLYMORPHIC_RELATION` を直接書いていた箇所 (ゲーム 154 + エンジン 32 の型、関係だけの登録 25) を、
`Engine/Module/Serialization/Engine_Module_SerializationRegistration.h` の次のマクロに置き換えた。

| マクロ | 展開 | 記録 |
|---|---|---|
| `NANAMI_REGISTER_TYPE(T, Base)` | `CEREAL_REGISTER_TYPE(T)` + `CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)` | `type` / `base` / `name` (= `binding_name<T>::name()`、保存ファイルの `polymorphic_name`) / `module` |
| `NANAMI_REGISTER_POLYMORPHIC_RELATION(Base, T)` | `CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)` | `type` / `base` / `module` (`name` は空) |

`ENGINE_REGISTER_COMPONENT` / `REGISTER_ATTACK_AREA_TYPE` / `REGISTER_PLAYER_AVATAR_BASE` も中身を `NANAMI_REGISTER_TYPE` にした。
記録先は `Serialization::SerializationTypeRegistry` (`Engine_Module_SerializationTypeRegistry.h/.cpp`)。`Instance()` は `.cpp` 定義なので
DLL 化しても実体はエンジンに 1 つ (§3.1 の問題が起きない)。`module` は、登録マクロから実体化されるテンプレート関数内の static 変数の
アドレスを `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)` に渡して求める (今の exe 1 つの構成では全部同じ値)。

- 保存形式は変わらない: cereal に渡す `T` のトークン列は置き換え前と同じ (綴りは変えていない)。マクロが 1 段増えても文字列化の結果が
  同じことは GCC / clang (`-fms-compatibility`) で確認済み。**MSVC での確認はビルド後に既存シーンを開いて行う**。
- `tools/bt` / `tools/animtree` の catalog scanner は `NANAMI_REGISTER_TYPE` も読むようにした (新旧どちらも可)。置き換え前後で
  `regen-catalog` の結果が同一なことを確認済み。`tools/bt add-action` の雛形も `NANAMI_REGISTER_TYPE` を出す。
- 今の静的リンク構成では記録を読む処理はまだ無い。段階 3 の `UnregisterModule` で使う。

### 判定
可。3.2 の PoC を最初に行う。

---

## 4. 登録はあるが登録解除がない

`/WHOLEARCHIVE` 前提のグローバル static 自己登録が全てで、**Unregister はどのレジストリにも存在しない**
(`EditorToolbarWidgetRegistry.h`, `AssetFactory.h`, `PacketTypeNameRegistry.h`, `DebugSheet.h`, `LocalPrefs_Editor_ToolBar.h`,
`AddComponent.h`, `MainWindowFactory.h` を grep)。アンロード後に残る関数ポインタ / `std::function` / vtable が次フレームで dangling になる。

| ゲーム -> エンジンの登録経路 | Assets 内の件数 | 保持するもの | 対応 |
|---|---|---|---|
| `NANAMI_REGISTER_TYPE` / `_POLYMORPHIC_RELATION` (段階 A で置き換え済み。`ENGINE_REGISTER_COMPONENT` 経由含む) | 156 / 160 | 関数ポインタ、caster オブジェクト | §3.2 の手順 2 (`SerializationTypeRegistry` の記録で削除) |
| `REGISTER_SCRIPTABLE_OBJECT` (= `REGISTER_ASSET` + `REGISTER_CREATABLE_ASSET_EXTENSION`) | 26 | `AssetFactory` のローダー `std::function` | モジュール ID 付き登録 + `UnregisterModule(id)` |
| `REGISTER_MAIN_WINDOW` / `REGISTER_POPUP_WINDOW` | 2 / 2 | ファクトリ `std::function` + 生成済みウィンドウ実体 | 同上。実体は §5 で破棄 |
| `REGISTER_LOCAL_PREF_WITH_PATH` | 6 | 編集 GUI ラムダ (`shared_ptr<optional<T>>` を捕捉) | 同上 |
| `AddComponent::RegisterMenu` | 1 | 関数ポインタ | 同上 |
| `PacketTypeNameRegistry::Register` (`Custom_PacketType.cpp`) | 1 | 文字列のみ、`names_[type] = name` で冪等 | 対応不要 (再登録で上書き) |
| `REGISTER_EDITOR_TOOLBAR_WIDGET` / `REGISTER_DEBUG_SHEET_PAGE` | 0 / 0 | - | 現状ゲーム側利用なし。同じ仕組みだけ入れておく |

ゲーム内部で閉じる登録 (`REGISTER_ENEMY_ACTION_WITH_NAME` 54、`REGISTER_FRIENDLY_ACTION_WITH_NAME` 26、`REGISTER_MAGIC_SPELL_EFFECT`、
`REGISTER_QUEST_*`、`REGISTER_ITEM_EFFECT`、`REGISTER_PLAYER_AVATAR_BASE`、`REGISTER_ATTACK_AREA_TYPE` など) は
Game.dll 内の static に登録されるので、DLL と一緒に消える。対応不要。

共通の実装方針: 各レジストリの `Register` に「現在ロード中のモジュール ID」(エンジンが `LoadLibrary` 前後で設定する thread_local か
グローバル) を記録させ、`UnregisterModule(id)` を 1 つ用意する。個々のマクロは変えずに済む。

### 判定
可。機械的な作業。

---

## 5. `FreeLibrary` 前に消さなければならない参照

Game.dll 由来のコードやオブジェクトへの参照が 1 つでも残ると、次に触った瞬間に落ちる。既存の破棄経路と不足分:

| 参照の持ち主 | 現状 | 不足 |
|---|---|---|
| Scene 上の GameObject / Component | `GameWindow::End()` (`GameWindow.cpp:130`) が `RemoveImplementAllGameObject` + 初期シーン再ロード + `ResetPhysics` | そのまま使える。再ロードは DLL ロード後に回す必要があるので、End を「破棄」と「初期シーン再ロード」に分割する |
| コルーチン | 同 End 内の `LifeCycle().Coroutine()->AllClear()` | なし |
| 非同期シーンロード | 同 End 内の `sceneLoader_.Cancel()` | なし |
| ScriptableObject アセット (ゲーム型、`AssetsDirectory` に常駐) | `ApplicationBase::ResetAssetsDirectory()` (`ApplicationBase.cpp:156`) | ホットリロード手順から呼ぶだけ |
| `ObjectRegistry` の `weak_ptr` | Component は `Unregister` (`ComponentGroup.cpp:76,109`)、アセットは `RemoveIfExpired` | **期限切れ `weak_ptr` の一括 purge が必要**。`weak_ptr` の制御ブロックはゲーム側の `make_shared` が作ったもので、期限切れでも最後の `weak_ptr` が消える時にゲーム側コードを呼ぶ。アンロード後に残っていると exit 時などに落ちる |
| `ApplicationLifeCycle` の `LifeCycleOnceCallbackGroup` (`Field<T>` の初期化待ち `weak_ptr`) | 1 回 Invoke すると pop される | アンロード前に 1 回 `OnUpdate` を回すか `Clear` を追加 |
| R4 購読 | `.Subscribe(...).AddTo(this)` で Component 破棄時に dispose | なし (規約通りに書かれていれば) |
| ゲーム側の `std::thread` | `HeightGridAstar` の 1 本のみ、デストラクタで `join` (`PathFinding_HeightGridAstar_Multithread.cpp:18-21`) | なし。Component 破棄で止まる |
| エンジンの `FutureTask` (`Coroutine_FutureTask.h:26` の `std::thread`) | ゲーム側の利用 0 | なし |
| `NetworkRunnerBase::s_instance_` (raw ポインタ、`CustomNetworkRunner` が継承) | Component | 破棄時に null になることを確認する |
| ゲーム製の Main/Popup ウィンドウ実体 (4 種、`MainWindows()` / `PopupWindows()` が保持) | - | **破棄が必要** (§4 のモジュール ID で識別) |
| ImGui のウィンドウ設定 / ID スタック | 文字列キーのみ | なし |
| Jolt Body の userData | `ResetPhysics()` で全消去 | なし |

差し替え手順 (エディタのみ、Play 中の状態は保持しない = Unity の Domain Reload 相当):

```
ScreenFlip 後 (ApplicationBase::Run, WindowDisplayModeController::OnFrameEnd と同じ位置)
  0. 開いているシーンをメモリに保存、開いているゲーム製 Window の名前を記録 (§12.1, §12.4)
  1. GameWindow: 全シーン破棄, コルーチン AllClear, sceneLoader Cancel, ResetPhysics
  2. ResetAssetsDirectory, ObjectRegistry / PrefabObjectRegistry の期限切れ purge, LifeCycle の残りを flush (§12.3)
  3. ゲーム製 Main/Popup ウィンドウを破棄 (表示中なら GameWindow に切り替え、未保存の編集は保存か確認) (§12.4)
  4. UnregisterModule(gameModuleId) : §4 のレジストリ + cereal の map から SerializationTypeRegistry の記録分を削除 (§3.2 の 2)
  5. FreeLibrary、cereal の Versions::mapping を clear (§3.2 の 3)
  6. Game_<n>.dll / .pdb をコピーして LoadLibrary (リンカが元ファイルを上書きできるように一意名で)
  7. Reload Assets (ScriptableObject を新しい型で読み直す), 0 で保存したシーンを復元, Window を開き直す
```

ビルドの起動は `GameBuilder` (`Engine/Core/Application/Build/GameBuilder.h`) が既に MSBuild を `AsyncProcess` で回しているので流用できる。

### 判定
可。

---

## 6. export 面の規模

現状 `dllexport` / `dllimport` は 0 件。エンジン側の規模:

| 対象 | 数 |
|---|---|
| 非テンプレートの class / struct 宣言 (Engine + Packages + LibCore) | 382 |
| テンプレート class / struct | 23 |
| namespace スコープの自由関数宣言 (概算) | 83 |
| `.cpp` にある out-of-line メンバ定義 (概算) | 2,911 |
| ヘッダ / `.cpp` | 349 / 468 (Jolt・ImGui・enet 込み) |

- MSVC の DLL export 上限 65,535 に対して十分小さい。**クラス単位の `NANAMI_API` マクロ方式**で十分。
- `.def` を全シンボルから自動生成する方式 (CMake の `WINDOWS_EXPORT_ALL_SYMBOLS` 相当) は、cereal のテンプレート実体化 (COMDAT) を
  拾って上限を超えるおそれがあるので採らない。
- テンプレート (`Field<T>`, `ComponentGroup::GetComponent<T>`, R4, cereal) は呼び出し側モジュールで実体化される。§3 の static 以外に
  テンプレート内 static は見つからなかった (`Packages/Cinemachine/.../ShakeCameraBehaviour.h:36` はクラス static で `.cpp` 定義)。
- 出荷ビルドを静的リンクのまま残すなら `NANAMI_API` は `NanamiApplicationMode=Game` で空定義にする。

### 判定
可。382 クラスへの付与が主作業 (機械的、スクリプト化可)。

---

## 7. DLL 境界を越える言語機能

| 機能 | 状況 |
|---|---|
| `dynamic_cast` / `dynamic_pointer_cast` (`ComponentGroup::Catch<T>`, `AssetFactory` のローダー) | MSVC は `type_info` をアドレスが違えば **名前文字列で比較**するので DLL 境界を越えて一致する |
| `std::type_index` をキーにした map (cereal `OutputBindingMap` / `PolymorphicCasters`) | `hash_code` も名前から計算。一致する |
| 例外 (`NanamiException` をゲームで投げてエンジンの `SafeExecutor` / `Main.cpp` で捕まえる) | /MD + 同一コンパイラなら問題なし。`__try/__except` (`Engine_Module_SafeExecute.cpp:130`) も DLL を跨げる |
| `constexpr APPLICATION_MODE` (`ApplicationConfiguration.h:15,17`)、`NANAMI_GAME_BUILD`、`NANAMI_DEBUG_SHEET_ENABLED` | **コンパイル時分岐なので全モジュールで一致が必須** (`GameCore::Game` のクラスレイアウトが `NANAMI_DEBUG_SHEET_ENABLED` で変わる)。既に `NanamiEngine.props` が全プロジェクトに同じ定義を配っているので、それを維持するだけ |
| `_ITERATOR_DEBUG_LEVEL` / Debug-Release の混在 | 不可。Game.dll はエンジンと同じ構成でビルドする |
| PCH (`stdafx.h` を `ForcedIncludeFiles`) | Game.dll 側は自前の PCH にし、`DxLib.h` / `EffekseerForDXLib.h` を外す (§2) |

### 判定
可。

---

## 8. ビルド・デバッグ運用

- **ファイルロック**: ロードした DLL / PDB はリンカが上書きできない。一意名 (`Game_<n>.dll`) にコピーしてからロードする。PDB はコピーで
  追従できる (デバッガはコピー側を読む)。
- **エディタ起動中のエンジン再ビルド**は不可 (NanamiEngine.dll がロード中)。ホットリロード対象は Game.dll のみ、と割り切る。
- **Debug 構成の `DebugInformationFormat`** は未指定 (`/Z7` は Release のみ)。DLL 化とは独立に、`/ZI` を付ければ VS の Edit and Continue も併用できる。
- **Unity ビルド** (`EnableUnitySupport`, 32 ファイル/バッチ) は DLL でもそのまま使える。
- **`NvOptimusEnablement`** (`Main.cpp:15`) はドライバが exe から読むので Host exe に移す。

---

## 9. 配布 (engine_dist / NanamiHub) への影響

- `tools/engine_dist/template/__PROJECT__.vcxproj` は `ConfigurationType=Application` でエンジン lib を `/WHOLEARCHIVE` リンクする
  (`NanamiEngine.Game.props:15-17`)。
- 推奨: **Editor モードのみ 3 モジュール構成、Game モードは今の静的リンクを維持**。`NanamiEngine.Game.props` で `NanamiApplicationMode` によって
  `ConfigurationType` (`DynamicLibrary` / `Application`) と `NANAMI_API` の定義を切り替えれば、出荷物と `GameBuilder` の手順は変わらない。
- 配布パッケージには `NanamiEngine.dll` + import lib + Host exe が増える。`engine_dist/selftest.py` の期待ファイル一覧を更新する。

---

## 10. 進め方 (提案)

各段階は単独でビルド・動作確認できる粒度にしてある。

| 段階 | 内容 | 主な作業 | 単独で得られる価値 |
|---|---|---|---|
| **A (済)** | 多相登録の入口を統一し、登録元モジュールを記録 (§3.4) | 登録 211 箇所の置き換え、`SerializationTypeRegistry` | 登録の一覧がエンジンから見える。保存形式は不変 |
| **PoC** | §3.2 の cereal 共有スロットパッチを、最小の Host exe + Engine.dll + Game.dll で検証 | `static_object.hpp` 改変、`SerializationTypeRegistry` の記録による cereal map からの削除、ロード -> シーン復元 -> アンロード -> 再ロードの 1 サイクル | 実現可否の最終確認 |
| 0 | /MD 化 | props 変更、Effekseer 8 lib の /MD 再ビルド、4 構成の動作確認 | なし (前提) |
| 1 | ゲームコードから DxLib を排除 | 34 ファイル、約 150 箇所をエンジンラッパーへ (入力・時間・2D 描画・サウンド・座標変換のラッパー整備を含む)。enet 1 ファイル | エンジン / ゲームの境界が明確になる |
| 2 | エンジン DLL 化 (Game は exe のまま) | Host exe へ WinMain 移動、`NANAMI_API` 付与 (382 クラス)、`SingletonBase` 7 クラスの `.cpp` 化、`IMGUI_API`、cereal パッチ適用、engine_dist 更新 | ビルド時間短縮、配布物のバイナリ互換 |
| 3 | Game.dll 化 + ホットリロード | `ConfigurationType` 切替、§4 のモジュール ID 付き Unregister、§5 の差し替え手順、ビルド起動 UI | 目的のホットリロード |

段階 2 まで進めば、ゲーム exe はエンジン DLL に依存する普通の構成になり、そこまでで止めても NanamiHub の「エンジン更新だけ差し替える」運用が可能になる。

---

## 11. 未検証事項 (実装前に PoC で確認すること)

1. §3.2 の cereal パッチで、エンジン側からゲーム Component を含む `.scene` / `.prefab` が JSON・PortableBinary 双方で復元できること。
2. アンロード -> 再ロードを 10 回以上繰り返してもリークや dangling が無いこと (Application Verifier / `_CrtDumpMemoryLeaks` で確認)。
3. Effekseer /MD 再ビルド物で、既存の全エフェクトが従来通り描けること。
4. `IMGUI_API` dllimport で `ImGuiHelper.h` (LibCore) と ImGuizmo が問題なく動くこと。
5. VS デバッガをアタッチしたまま Game.dll を差し替えてブレークポイントが効くこと (PDB コピー運用)。

---

## 12. レビューで追加した検討事項

### 12.1 差し替え後に戻すシーン
§5 の手順 1 / 7 は `GameWindow::End()` (`GameWindow.cpp:130`) を土台にしているが、End は
`BuildConfiguration::StartScenePath()` を**ディスクから**読み直す。そのままだと、リロードのたびにスタートシーンへ戻り、
未保存の編集も消える (Unity の Domain Reload は直前の状態を保存して戻す)。

- 手順 1 の前に、開いているシーン (`GameWindow::contents_`) を cereal でメモリに保存し、手順 7 ではスタートシーンではなくそれを復元する。
- 保存は古い Game.dll があるうちに、復元は新しい Game.dll を読んでから行う。PoC (§11 の 1) でこの往復も確認する。

### 12.2 型の変更とバージョン
cereal の JSON 読み込みは、名前が見つからないと例外を投げる (`json.hpp:555`)。§12.1 の保存 -> 復元を挟むと:

- メンバを足したら `CEREAL_CLASS_VERSION` を上げ、`load` で `if (version >= N)` にする (今の規約どおり)。
  古い DLL で保存したデータは古い番号なので、足したメンバは既定値のまま読める。上げ忘れると復元が例外で失敗する。
- メンバ名の変更 (Unity の `[FormerlySerializedAs]` 相当) は、バージョンを上げて `load` で旧名 / 新名を読み分ける。
- §3.2 の手順 3 (`Versions::mapping` の clear) をしないと、上げた番号が反映されない。

### 12.3 期限切れ `weak_ptr` の掃除 (§5 の補足)
`make_shared` した場所のモジュールに制御ブロックの破棄処理ができるので、Game.dll で作られたオブジェクトの `weak_ptr` が
`FreeLibrary` 後に消えると落ちる。

- `ObjectRegistry` に `PurgeExpired()` (`std::erase_if(assets_, 期限切れ)`) を足し、手順 2 で呼ぶ (Game.dll がまだあるうちに破棄処理を走らせる)。
- `Network::PrefabObjectRegistry` (`NetworkPrefabObjectRegistry.h`) も同じ形の `weak_ptr` 表で、**削除手段が無い**。同じく `PurgeExpired()` を足す。
- ほかに、エンジン側が長く持つ `weak_ptr` / `shared_ptr` / `std::function` / R4 の購読で、ゲームが作ったものを指しうる箇所を実装時に洗い出す。
  危ないのはデータ (/MD でヒープ共有) ではなく **Game.dll のコードを指すもの** (vtable、関数ポインタ、ラムダ、制御ブロック)。

### 12.4 ゲーム製ウィンドウ (§5 の補足)
対象: `EnemyNpcBehaviourWindow` / `FriendlyNpcBehaviourWindow` (Main)、`RunningEnemyBehaviourTreeWindow` / `RunningFriendlyBehaviourTreeWindow` (Popup)。

- **`MainWindowGroup` に削除関数が無い** (一度作ったメイン Window は終了まで残る)。削除を足す。
- `CurrentMainWindow()` がゲーム製 Window を指していたら、外す前に `GameWindow` へ切り替える。
- Behaviour Tree エディタの未保存の編集は Window の中にある。破棄の前に `MainWindowGroup::OnSave()` を呼ぶか、確認を出す。
- 開いていた Window の名前を覚えておき、読み込み後に開き直す (Unity と同じ使い心地にするため)。
- `REGISTER_MAIN_WINDOW` / `REGISTER_POPUP_WINDOW` の 4 つはヘッダに書かれている (登録は map への代入なので今は無害)。
  モジュール ID で管理するなら `.cpp` へ移す。

### 12.5 取り残しの検出と保険
- 取り残しの検出: オブジェクトの vtable のアドレス (`*reinterpret_cast<void**>(obj)`) を `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)`
  に渡すと、どのモジュールのクラスか分かる。`FreeLibrary` 直前に、エンジンが持つ Window / アセット / 登録表を走査して
  Game.dll のものが残っていないかを Debug ビルドで assert できる。
- 保険: DLL は `Game_<n>.dll` の一意名でロードするので、開発中は**古い DLL を `FreeLibrary` しない**モードを用意できる。
  取り残しがあっても古いコードが動くだけで落ちない (リロードごとにメモリは増えるが、エディタ再起動で戻る)。
  普段はこのモード、掃除漏れを探すときだけ `FreeLibrary` するモード (§11 の 2) にする。
