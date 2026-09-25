# ゲームコードのホットリロード (案C) 計画書

エディタを起動したまま `Assets/Scripts` を再ビルドして差し替えるために、現在の
「NanamiEngine (静的 lib) + EnviroHunter (exe)」を

```
Host exe (WinMain のみ)  ->  NanamiEngine.dll  <-  Game.dll (Assets/Scripts)
                                                  ^ エディタが LoadLibrary / FreeLibrary で差し替える
```

の 3 モジュール構成に分ける計画。§1〜§9 は 2026-09-25 の実現可能性調査 (grep / ファイル読解で確認した事実)、
§10 が段階と進捗、§11 が未検証事項。未検証の箇所は「要 PoC」と明記する。

改訂履歴
- 2026-09-25: 初版 (実現可能性調査)。
- 2026-09-25: 段階 0 完了 (Effekseer /MD 再ビルド、`NanamiUseDynamicCrt` 既定 `true`)。段階 1 (ゲームコードの DxLib 排除) 実装、Windows 4 構成ビルド済み。
- 2026-09-25: 段階 A 実装 (MSVC ビルド・エディタ確認済み)。PoC を `tools/hotreload_poc/` に実装、Linux (g++ + dlopen) で 10 サイクル PASS。
- 2026-09-25: 段階 A (多相登録のラップ) を追加。登録解除を「cereal の表の差分方式」から「登録記録方式」に変更。
  レビューで出た検討事項 (開いているシーンの保持、バージョン運用、期限切れ weak_ptr、ゲーム製ウィンドウ、
  取り残し検出、FreeLibrary しない保険モード) を §5 に追加。

---

## 0. 結論

**実現可能。ただし 2 つの前提作業と、1 つの PoC 必須項目がある。**

| # | 項目 | 判定 | 根拠 (詳細は各節) |
|---|---|---|---|
| 1 | CRT を /MT から /MD へ | 可 (作業あり) | DxLib は `_DLL` 定義で MD 版を自動選択。**Effekseer 系 8 lib は MT 版しか無く、/MD で再ビルドが必須** |
| 2 | サードパーティ lib のグローバル状態の二重化 | 可 (作業あり) | ゲームからの DxLib 直接呼び出し 34 ファイルを排除する必要。ImGui は `IMGUI_API` で import 可 |
| 3 | エンジン内 static の二重化 | 可 | `SingletonBase` 利用 7 クラスを `.cpp` 化。cereal の `StaticObject` は **vendored 版へのパッチが必要 (要 PoC)** |
| 4 | 登録解除の欠如 | 可 (作業あり) | ゲームからエンジンへ登録する経路 7 種、いずれも Unregister 無し。cereal 分は **段階 A の登録記録**から消す |
| 5 | アンロード前の参照掃除 | 可 | `GameWindow::End()` が土台。開いているシーンの保持、期限切れ `weak_ptr` の purge、ゲーム製ウィンドウの破棄を足す |
| 6 | export 面の規模 | 可 | 非テンプレートクラス 382、メンバ定義 2,911 で MSVC の 65,535 export 上限に余裕。`.def` 全出力方式は不可 |
| 7 | DLL 境界の言語機能 (RTTI / 例外 / type_index) | 可 | MSVC は `type_info` を名前で比較するので `dynamic_cast` / `std::type_index` は跨げる |
| 8 | 差し替え手順の挿入点 | 可 | `ApplicationBase::Run` の `ScreenFlip` 直後 (`WindowDisplayModeController::OnFrameEnd` と同じ位置) |
| 9 | 配布 (engine_dist / NanamiHub) への影響 | 可 | 出荷ビルド (`NanamiApplicationMode=Game`) は静的リンクのまま残せる |

**事前に潰すべき最大のリスクは 3 の cereal パッチ**で、これが成立しないと Scene / Prefab のロードでゲーム側 Component を復元できない。
その前提として、**段階 A で多相登録の入口をエンジンのマクロに統一し、どのモジュールが何を登録したかを記録する**。
登録解除はこの記録から引く (§3.2, §4)。

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
可。Effekseer の再ビルドが唯一の追加作業 (`tools/effekseer_md/README.md`)。/MD 化は静的リンクのまま先に行い、4 構成 (Editor/Game × Debug/Release) が動くことを確認してから DLL 化へ進める。
切り替えは `NanamiEngine.props` の `NanamiUseDynamicCrt` (MD 版 lib が揃うまで既定 `false`)。/MD のゲーム版は VC++ ランタイム DLL が要るので、
`NanamiEngine.Game.props` の `NanamiCopyCrtRedist` (Game × Release) が `$(VCToolsRedistDir)` から出力先へコピーし、`GameBuilder::Package` が exe の隣の DLL を配布フォルダへ持っていく。

---

## 2. サードパーティ lib のグローバル状態が 2 モジュールに分裂する問題

静的 lib を Game.dll にもリンクすると、その lib のグローバル状態 (DxLib のハンドル表、ImGui の `GImGui`、
Effekseer のマネージャ) が **エンジン側と別物として 2 つできる**。ゲーム側の呼び出しは初期化されていない
方のコピーを叩くことになり、リンクは通るのに実行時に壊れる。ゲームコードからの直接利用を洗った結果:

| lib | ゲーム側の直接利用 | 対策 |
|---|---|---|
| DxLib | **42 ファイル、302 箇所** (初版の 34 / 150 は入力系だけの数) (`CheckHitKey` 60、`VGet` 15、`SetDrawBlendMode` 14、`GetJoypadXInputState` 14、`GetNowCount` 13、`DrawRotaGraphF` 10、ほか `PlaySoundMem` / `DrawLine3D` / `ConvWorldPosToScreenPos` / `MV1SearchFrame` など) | ゲームコードから DxLib を **完全に排除**し、エンジンのラッパー経由にする (入力 / 時間 / 2D 描画 / サウンド / 座標変換)。CLAUDE.md の「エンジンヘッダは DxLib を露出しない」の延長。Game.dll 側は `DX_LIB_NOT_DEFAULTPATH` を定義して DxLib の自動リンクを切れば (`DxDataTypeWin.h:29`)、取り残しは **リンクエラーとして検出できる** |
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
**段階 A で新設する `SerializationTypeRegistry` も、同じ理由で `Instance()` を `.cpp` に定義する。**

### 3.2 cereal の多相登録: 段階 A のラップと `StaticObject<T>` の単一化 — **要 PoC**

#### 現状 (2026-09-25)

多相登録はすべて cereal のマクロを直接呼んでいて、エンジンには何の記録も残らない。

| 書き方 | エンジン側 | ゲーム側 | 備考 |
|---|---|---|---|
| `CEREAL_REGISTER_TYPE(T)` | 32 (`Libs/LibCore/BlackBoard/AnimationParameter.cpp` の 3 型を含む) | 156 | |
| `CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)` | 53 | 160 | 型数より多い分 (エンジン 21、ゲーム 4 行以上) は 2 つ目以降の基底 (`IUpdatable` / `IAwakable` など) と中間基底の関係のみ。関係が型より多いファイルは 13 |
| `ENGINE_REGISTER_COMPONENT(T)` (`ComponentBase.h:85-95`) | - | 114 | 上の 2 つに展開するだけの省略記法 |
| `REGISTER_ATTACK_AREA_TYPE(T)` (`AttackArea.h:243`) | - | 4 | 同上 (ゲーム側ヘッダで定義) |
| `REGISTER_PLAYER_AVATAR_BASE(T)` (`PlayerAvatarBase.h:347`) | - | 4 | 同上 (ゲーム側ヘッダで定義、複数行) |

書式のばらつき (置き換えスクリプトが扱う 4 パターン):
1. `TYPE` と `RELATION` の 2 行組 (大半)。
2. 複数行に分かれた `RELATION` (`AnimationNodePathAdditionCondition.cpp:8-19`、`REGISTER_PLAYER_AVATAR_BASE` の定義)。
3. `TYPE` を 3 つ並べた後に `RELATION` を 3 つ並べる形 (`AnimationNodePathAdditionCondition.cpp:5-7`、
   `Libs/LibCore/BlackBoard/AnimationParameter.cpp:5-7`)。
4. `RELATION` だけの行 (`ENGINE_REGISTER_COMPONENT` の後に 2 つ目の基底を足すもの。例: `Game.cpp:164`、`Rotator.cpp:24`)。

行末の `;` は 235 行にあり 133 行に無い。BOM の無い `.cpp` が 4 つある (`HlslFile.cpp`、`Enemy_Behaviour_Action_ToPlayerRaycast.cpp`、
`Enemy_Behaviour_Action_ChasePlayerForPathFinding.cpp`、`Enemy_Behaviour_Action_ToPlayerDistance.cpp`)。

#### 段階 A: 登録の入口をエンジンのマクロに統一し、登録元モジュールを記録する

作るもの (置き場所は `Engine/Module/Serialization/Engine_Module_SerializationRegistration.h`。登録を書く `.cpp` は全部これを include している):

- `NANAMI_REGISTER_TYPE(T, Base)`
  = `CEREAL_REGISTER_TYPE(T)` + `CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)` + 記録。
- `NANAMI_REGISTER_POLYMORPHIC_RELATION(Base, T)`
  = 関係だけ + 記録 (2 つ目以降の基底、中間基底用)。
- 記録先 `Serialization::SerializationTypeRegistry` (新規 `.h` / `.cpp`、`NanamiEngine.vcxproj` と `.filters` に手で追加):
  - 1 件 = `type` (`std::type_index`) / `base` (`std::type_index`) / `name` (`cereal::detail::binding_name<T>::name()`。
    保存ファイルの `polymorphic_name` と同じ文字列。関係のみの登録では空) / `module` (`HMODULE`)。
  - `Instance()` は `.cpp` に定義する (§3.1)。
  - `module` は、マクロから実体化されるテンプレート関数の中の static 変数のアドレスを
    `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | UNCHANGED_REFCOUNT)` に渡して求める。
    `Windows.h` は `.cpp` だけで include する (ヘッダには `void*` で渡す)。
  - マクロ内の登録用変数は `namespace { const bool <__COUNTER__ で一意な名前> = Record<T, Base, IsType>(); }` の形。
    呼び出し行の末尾に `;` があってもなくても通る形にする。
- 既存の 3 つのラッパーマクロ (`ENGINE_REGISTER_COMPONENT` / `REGISTER_ATTACK_AREA_TYPE` / `REGISTER_PLAYER_AVATAR_BASE`) の中身も
  `NANAMI_REGISTER_TYPE` にする。呼び出し側は変えない。
- 保存ファイルの形式は変えない。第 1 引数のトークン列がそのまま `polymorphic_name` になるので、**綴りは 1 文字も変えない**
  (部分修飾の名前、例 `Network::NetworkAnimator`、`CineMachine::...` も `using namespace` 頼みのまま同じ綴りで残す)。

#### `StaticObject<T>` の単一化 (段階 4 で適用、PoC で先に検証)

cereal は多相ポインタの保存・復元に、モジュールごとの関数ローカル static (`static_object.hpp:66`) を使う:

| StaticObject の中身 | キー | 値 | Game.dll のアンロードで dangling になるか |
|---|---|---|---|
| `InputBindingMap<Archive>::map` (JSON / PortableBinary の 2 つ) | `std::string` = `name` | 関数ポインタ | **なる** |
| `OutputBindingMap<Archive>::map` (同 2 つ) | `std::type_index` = `type` | 関数ポインタ | **なる** |
| `PolymorphicCasters::map` / `reverseMap` | `std::type_index` = `type` と `base` | `PolymorphicVirtualCaster const*` (静的オブジェクト、vtable あり) | **なる** |
| `Versions::mapping` | 型名ハッシュ | `uint32_t` | ならない (§3.2 の「登録解除」参照) |

`CEREAL_DLL_EXPORT` は `dllexport` を付けるだけで、**import 側は無い**。そのため現状のままだと:
- エンジン側 (`Scene` / `PrefabGameObjectFile` のロード) がゲーム Component を復元しようとしても、エンジンの map にゲーム型が無い。
- ゲーム側がエンジン型 (`shared_ptr<IGameObject>` など) を保存しようとしても、ゲームの map にエンジン型が無い。
つまり **双方向にマージが必要**で、片方向のコピーでは足りない。

実装 (`Libs/cereal/include/cereal/details/static_object.hpp` のパッチ、`CEREAL_NANAMI_SHARED_STATIC_OBJECT` 定義時のみ有効。
未定義なら元の cereal のまま = 今の静的 lib ビルドは無変更):
`StaticObject<T>::create()` が、エンジン DLL の export する `cereal::detail::nanami_shared_static_object(typeid(T).name(), create, destroy)`
から実体を取る。表は `Engine/Module/Serialization/Engine_Module_SharedStaticObject.cpp` にあり、実体を作ったモジュール
(create 関数のアドレスから求める) を覚えておく。これで全モジュールが同じ map を見る。
登録は Game.dll の静的初期化 (DllMain のローダーロック中) で走るが、既にロード済みのエンジン DLL の export を呼ぶだけなので問題ない。
**注意**: `create()` はロックの外で呼ぶ (`OutputBindingCreator` のコンストラクタが `OutputBindingMap` を取りに再入する。
ロック中に作ると `std::mutex` でデッドロックする。PoC で判明)。
export 指定は `Engine/Core/Api/NanamiApi.h` の `NANAMI_API` (`NANAMI_ENGINE_BUILD_DLL` / `NANAMI_ENGINE_USE_DLL` / どちらも無し = 空)。
段階 2 で全公開クラスに付けるものの先行分として、Serialization の 3 クラスに付けてある。

#### 登録解除 (記録方式)

**cereal の表のキー集合を LoadLibrary 前後で比べて増えた分を消す「差分方式」は使わない。**
`SerializationTypeRegistry` から Game.dll の `module` の記録を引き、その `name` / `type` / `base` で cereal の表から消す
(実装: `Engine/Module/Serialization/Engine_Module_SerializationModuleUnloader.cpp` の `SerializationModuleUnloader::Unregister(module)`。
`ClearClassVersions()` と、Debug の取り残し確認用 `CountLeftoverCasters(module)` も同じクラス):

| 表 | 消すキー |
|---|---|
| `InputBindingMap<JSON>::map`, `InputBindingMap<PortableBinary>::map` | 記録の `name` (空でないもの) |
| `OutputBindingMap<JSON>::map`, `OutputBindingMap<PortableBinary>::map` | 記録の `type` |
| `PolymorphicCasters::map` | `map[base]` から `type` の項目を消す。`base` の項目が空になれば `base` ごと消す |
| `PolymorphicCasters::reverseMap` | `type` |

- 推移的な項目: cereal は (Mid, Derived) の登録時に `map[祖先][Derived]` と `reverseMap[Derived] = 祖先` も足すので、派生をキーに
  全部の基底から消す。その型を基底とする `map[型]` も丸ごと消す (派生は同じ DLL の型)。
- 保険として、vtable が Game.dll にある caster を含む項目も消す (実体はヒープにあるので、アドレスではなく vtable で
  `GetModuleHandleExW(FROM_ADDRESS)` を引く)。PoC では記録だけで足りていて、この掃除で消えるものは 0。
- `SerializationTypeRegistry` の Game.dll 分の記録も、Game.dll を外すときに消す。
- `Versions::mapping` は型名ハッシュ → 番号だけで、無ければ保存時に入れ直される (`cereal.hpp:596` `registerClassVersion`)。
  `emplace` なので古い値が残る (`helpers.hpp:415`)。**FreeLibrary の後、LoadLibrary の前に丸ごと `clear()` する**。

### 3.3 その他
- rxcpp: `Packages/R4/Core` はスケジューラ (`observe_on` / `current_thread`) を一切使っていない。`rx-scheduler.hpp:32` の `shared_empty`
  が二重化するだけで無害。
- `REGISTER_LOCAL_PREF_WITH_PATH` の `inline static` 登録子 (`Engine_Module_LocalPrefs_Editor_ToolBar.h:136,176`) はモジュールごとに
  1 個ずつ走るのが正しい挙動なので問題なし (登録先は §3.1 で単一化される)。
- `ApplicationLifeCycle.cpp:6` の `thread_local` はエンジン `.cpp` 内なので単一。

### 判定
可。段階 A → PoC の順で進める。

---

## 4. 登録はあるが登録解除がない

`/WHOLEARCHIVE` 前提のグローバル static 自己登録が全てで、**Unregister はどのレジストリにも存在しない**
(`EditorToolbarWidgetRegistry.h`, `AssetFactory.h`, `PacketTypeNameRegistry.h`, `DebugSheet.h`, `LocalPrefs_Editor_ToolBar.h`,
`AddComponent.h`, `MainWindowFactory.h` を grep)。アンロード後に残る関数ポインタ / `std::function` / vtable が次フレームで dangling になる。

| ゲーム -> エンジンの登録経路 | Assets 内の件数 | 保持するもの | 対応 |
|---|---|---|---|
| 多相登録 (段階 A 後は `NANAMI_REGISTER_TYPE` / `NANAMI_REGISTER_POLYMORPHIC_RELATION`。`ENGINE_REGISTER_COMPONENT` 等の経由含む) | 156 型 / 160 関係 | cereal の表の関数ポインタ、caster オブジェクト | `SerializationTypeRegistry` の記録から消す (§3.2 の記録方式) |
| `REGISTER_SCRIPTABLE_OBJECT` (= `REGISTER_ASSET` + `REGISTER_CREATABLE_ASSET_EXTENSION`) | 26 | `AssetFactory` のローダー `std::function` | モジュール ID 付き登録 + `UnregisterModule(id)` |
| `REGISTER_MAIN_WINDOW` / `REGISTER_POPUP_WINDOW` | 2 / 2 | ファクトリ `std::function` + 生成済みウィンドウ実体 | 同上。実体は §5 で破棄。**呼び出しがヘッダに書かれている 4 件は `.cpp` へ移す** (ヘッダだと include したファイルごとに登録子ができる) |
| `REGISTER_LOCAL_PREF_WITH_PATH` | 6 | 編集 GUI ラムダ (`shared_ptr<optional<T>>` を捕捉) | 同上 |
| `AddComponent::RegisterMenu` | 1 | 関数ポインタ | 同上 |
| `PacketTypeNameRegistry::Register` (`Custom_PacketType.cpp`) | 1 | 文字列のみ、`names_[type] = name` で冪等 | 対応不要 (再登録で上書き) |
| `REGISTER_EDITOR_TOOLBAR_WIDGET` / `REGISTER_DEBUG_SHEET_PAGE` | 0 / 0 | - | 現状ゲーム側利用なし。同じ仕組みだけ入れておく |

ゲーム内部で閉じる登録 (`REGISTER_ENEMY_ACTION_WITH_NAME` 54、`REGISTER_FRIENDLY_ACTION_WITH_NAME` 26、`REGISTER_MAGIC_SPELL_EFFECT`、
`REGISTER_QUEST_*`、`REGISTER_ITEM_EFFECT`、`REGISTER_PLAYER_AVATAR_BASE`、`REGISTER_ATTACK_AREA_TYPE` など) は
Game.dll 内の static に登録されるので、DLL と一緒に消える。対応不要。

共通の実装方針: 各レジストリの `Register` に「現在ロード中のモジュール ID」(段階 A と同じく登録子のアドレスから `HMODULE` を求める) を
記録させ、`UnregisterModule(HMODULE)` を 1 つ用意する。個々のマクロは変えずに済む。

### 判定
可。機械的な作業。

---

## 5. `FreeLibrary` 前に消さなければならない参照

Game.dll 由来のコードやオブジェクトへの参照が 1 つでも残ると、次に触った瞬間に落ちる。
**危ないのはデータではなく、Game.dll のコードを指すもの** (vtable、関数ポインタ、ラムダ、`shared_ptr` の制御ブロック)。
エンジンが長く持つものを洗い出す。既存の破棄経路と不足分:

| 参照の持ち主 | 現状 | 不足 |
|---|---|---|
| Scene 上の GameObject / Component | `GameWindow::End()` (`GameWindow.cpp:130`) が `RemoveImplementAllGameObject` + `StartScenePath` の再ロード + `ResetPhysics` | 「破棄」と「シーン復元」を分け、復元は DLL ロード後に回す (下の「戻すシーン」) |
| コルーチン | 同 End 内の `LifeCycle().Coroutine()->AllClear()` | なし |
| 非同期シーンロード | 同 End 内の `sceneLoader_.Cancel()` | なし |
| ScriptableObject アセット (ゲーム型、`AssetsDirectory` に常駐) | `ApplicationBase::ResetAssetsDirectory()` (`ApplicationBase.cpp:156`) | ホットリロード手順から呼ぶだけ |
| `ObjectRegistry` の `weak_ptr` | Component は `Unregister` (`ComponentGroup.cpp:76,109`)、アセットは `RemoveIfExpired` | **`PurgeExpired()` を足し、FreeLibrary の前に呼ぶ**。`weak_ptr` の制御ブロックはゲーム側の `make_shared` が作ったもので、期限切れでも最後の `weak_ptr` が消える時にゲーム側コードを呼ぶ |
| `Network::PrefabObjectRegistry` (`NetworkPrefabObjectRegistry.h:17-22`、`Add` / `Catch` のみ) | **削除手段がまったく無い** | 同じく `PurgeExpired()` を足す |
| `ApplicationLifeCycle` の `LifeCycleOnceCallbackGroup` (`Field<T>` の初期化待ち `weak_ptr`) | 1 回 Invoke すると pop される | アンロード前に 1 回 `OnUpdate` を回すか `Clear` を追加 |
| R4 購読 | `.Subscribe(...).AddTo(this)` で Component 破棄時に dispose | なし (規約通りに書かれていれば) |
| ゲーム側の `std::thread` | `HeightGridAstar` の 1 本のみ、デストラクタで `join` (`PathFinding_HeightGridAstar_Multithread.cpp:18-21`) | なし。Component 破棄で止まる |
| エンジンの `FutureTask` (`Coroutine_FutureTask.h:26` の `std::thread`) | ゲーム側の利用 0 | なし |
| `NetworkRunnerBase::s_instance_` (raw ポインタ、`CustomNetworkRunner` が継承) | Component | 破棄時に null になることを確認する |
| ゲーム製の Main/Popup ウィンドウ実体 (`EnemyNpcBehaviourWindow` / `FriendlyNpcBehaviourWindow` / `RunningEnemyBehaviourTreeWindow` / `RunningFriendlyBehaviourTreeWindow`) | `MainWindowGroup` (`MainWindowGroup.h`) に **削除関数が無い** (`MakeWindow` / `Catch` / `OnSave` のみ) | 下の「ゲーム製ウィンドウ」 |
| ImGui のウィンドウ設定 / ID スタック | 文字列キーのみ | なし |
| Jolt Body の userData | `ResetPhysics()` で全消去 | なし |

### 検討事項 (レビューで追加)

- **戻すシーン**: `GameWindow::End()` は `StartScenePath` をディスクから読み直すので、そのままでは毎回スタートシーンに戻り、
  未保存の編集も消える。差し替えの前に開いているシーン (全 `contents_` と `mainScene_`) をメモリに保存 (PortableBinary) し、
  新しい DLL を読んだ後にそれを復元する (Unity の Domain Reload と同じ順番)。Play 中の状態は保持しない (Play 中は差し替え前に Stop する)。
- **バージョン**: cereal の JSON は名前が見つからないと例外を投げる。メンバを足したら `CEREAL_CLASS_VERSION` を上げ、
  `load` で `if (version >= N)` にする。名前を変えるときは `load` で旧名 / 新名を読み分ける。
  メモリ上のスナップショットも同じ規則で復元されるので、差し替え前後でこの規則を守れば編集中のシーンは失われない。
- **期限切れ weak_ptr**: `ObjectRegistry` と `Network::PrefabObjectRegistry` に `PurgeExpired()` を足し、FreeLibrary の前に呼ぶ。
- **ゲーム製ウィンドウ 4 つ**: `MainWindowGroup` / `PopupWindowGroup` にモジュール単位の削除関数を足す。
  `CurrentMainWindow()` がゲーム製なら先に `GameWindow` に切り替える。Behaviour Tree エディタの未保存の編集は
  `OnSave` か確認ダイアログで守る。開いていたウィンドウ (型名で記録) は読み込み後に開き直す。
- **取り残しの検出**: vtable のアドレス (`*reinterpret_cast<void* const*>(obj)`) を `GetModuleHandleExW(FROM_ADDRESS)` に渡せば
  どのモジュールのクラスか分かる。FreeLibrary 直前に、`ObjectRegistry` / ウィンドウ群 / レジストリ群に Game.dll のものが
  残っていないかを Debug ビルドで `assert` する。
- **保険**: DLL は `Game_<n>.dll` の一意名で読むので、**開発中は古い DLL を FreeLibrary しないモード**を用意する
  (取り残しがあっても落ちない)。掃除漏れを探すときだけ FreeLibrary するモードに切り替える。

### 差し替え手順 (エディタのみ)

```
ScreenFlip 後 (ApplicationBase::Run, WindowDisplayModeController::OnFrameEnd と同じ位置)
  0. Play 中なら Stop。開いているウィンドウの型名と、開いている全シーンを PortableBinary でメモリに保存
  1. GameWindow: 全シーン破棄, コルーチン AllClear, sceneLoader Cancel, ResetPhysics
  2. ResetAssetsDirectory, ObjectRegistry / PrefabObjectRegistry の PurgeExpired, LifeCycle の残りを flush
  3. CurrentMainWindow を GameWindow に切り替え、ゲーム製 Main/Popup ウィンドウを破棄
  4. UnregisterModule(gameModule): §4 のレジストリ + SerializationTypeRegistry の記録から cereal の表を掃除 (§3.2)
  5. Debug では取り残し assert。FreeLibrary (保険モードでは呼ばない)。Versions::mapping を clear
  6. Game_<n>.dll / .pdb をコピーして LoadLibrary (リンカが元ファイルを上書きできるように一意名で)
  7. Reload Assets (ScriptableObject を新しい型で読み直す), 保存したシーンを復元, ウィンドウを開き直す
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
| `std::type_index` をキーにした map (cereal `OutputBindingMap` / `PolymorphicCasters`、`SerializationTypeRegistry`) | `hash_code` も名前から計算。一致する |
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
- **Unity ビルド** (`EnableUnitySupport`, 32 ファイル/バッチ) は DLL でもそのまま使える。段階 A の `namespace { }` 内の `__COUNTER__` 変数名は
  Unity バッチ内でも衝突しない (`__COUNTER__` は翻訳単位ごとに単調増加)。
- **`NvOptimusEnablement`** (`Main.cpp:15`) はドライバが exe から読むので Host exe に移す。

---

## 9. 配布 (engine_dist / NanamiHub) への影響

- `tools/engine_dist/template/__PROJECT__.vcxproj` は `ConfigurationType=Application` でエンジン lib を `/WHOLEARCHIVE` リンクする
  (`NanamiEngine.Game.props:15-17`)。
- 推奨: **Editor モードのみ 3 モジュール構成、Game モードは今の静的リンクを維持**。`NanamiEngine.Game.props` で `NanamiApplicationMode` によって
  `ConfigurationType` (`DynamicLibrary` / `Application`) と `NANAMI_API` の定義を切り替えれば、出荷物と `GameBuilder` の手順は変わらない。
- 配布パッケージには `NanamiEngine.dll` + import lib + Host exe が増える。`engine_dist/selftest.py` の期待ファイル一覧を更新する。
- 段階 A の新マクロは配布ヘッダに含まれるので、テンプレートプロジェクト (`tools/engine_dist/template/Assets/Scripts`) の雛形も新マクロにする。

---

## 10. 段階と進捗

各段階は単独でビルド・動作確認できる粒度にしてあり、**段階ごとにコミットを分ける** (後の段階と混ぜない)。
ビルドと AutoMCP による確認は CLAUDE.md のとおり、都度ユーザーの許可を取る。

| 段階 | 内容 | 主な作業 | 完了条件 | 進捗 |
|---|---|---|---|---|
| **A** | 多相登録のラップ (§3.2) | `NANAMI_REGISTER_TYPE` / `NANAMI_REGISTER_POLYMORPHIC_RELATION` と `SerializationTypeRegistry` を追加。既存の cereal 直接呼び出し (エンジン 32 型 / 53 関係、ゲーム 156 型 / 160 関係) と 3 つのラッパーマクロをスクリプトで置き換え。ツール・ドキュメント更新 (下記) | 型名の文字列が変わらないこと、regen-catalog の結果が同一、MSVC ビルド、既存のシーン・プレハブ・BT・AnimTree がエディタで開くこと | **実装済み** (2026-09-25)。置き換え 195 ファイル / 397 箇所、型 186・関係 211 の集合が前後で一致、データ内の `polymorphic_name` 246 種すべてが登録名に含まれることを確認。**MSVC ビルドとエディタでの確認は未実施** (Windows 環境で行う) |
| **PoC** | §3.2 の共有スロットパッチ + 記録方式の登録解除を、最小の Host exe + Engine.dll + Game.dll で検証 | `static_object.hpp` 改変、`SharedStaticObjects` / `SerializationModuleUnloader` の追加、`tools/hotreload_poc/` (sln + Linux 用スクリプト、README 参照) | ゲーム型を含む多相ポインタが JSON・PortableBinary 双方でエンジン側から復元でき、ゲーム側からエンジン型も保存・復元でき、10 回繰り返しても表がベースラインに戻る。不成立なら止めて報告 | **実装済み**。Linux (g++ + `dlopen(RTLD_DEEPBIND)`、`-fno-gnu-unique`) で 10 サイクル PASS、valgrind でエラー 0・definite leak 0。**Windows (MSVC) での実行は未実施**: `tools/hotreload_poc/HotReloadPoc.sln` をビルドして `HotReloadPocHost.exe` を実行する |
| 0 | /MD 化 | props 変更、Effekseer 8 lib の /MD 再ビルド | 4 構成 (Editor/Game × Debug/Release) が動く | **完了** (2026-09-25)。Effekseer 170e + EffekseerForDXLib 17x@796064f1 を /MD で再ビルドして `*_vs2019_x64_MD(d).lib` を同梱 (`tools/effekseer_md/`)。`-p:NanamiUseDynamicCrt=true` で 4 構成がビルドでき、エディタ起動を確認。`NanamiUseDynamicCrt` の既定を `true` に変更 (`false` で /MT に戻る)。CRT ランタイム DLL は `$(VCToolsRedistInstallDir)` からコピー (MSBuild 単体では `$(VCToolsRedistDir)` が空) |
| 1 | ゲームコードから DxLib を排除 | 42 ファイル、302 箇所 (再棚卸しで判明。当初の 34 / 150 は入力系だけの数) をエンジンのファサードへ: `Engine/Core/Platform/{Input,Draw2D,Render,AsyncLoad}` (新規 7 組)、`Time::NowMilliseconds`、`SoundFile` の再生 API、`Render3D::Shapes::DrawLine3D`、`ApplicationBase::RequestClose`。enet 1 ファイルは段階 2 で扱う | `python tools/dxlib_guard/check_game_dxlib.py` が 0 (`DX_LIB_NOT_DEFAULTPATH` によるリンク検出は Game.dll 化後) | **実装済み** (2026-09-25)。guard 0 件。MSVC ビルド (Editor Debug / Release、/MD) は通り、エディタ起動とタイトル画面の描画・ログにエラー無しを確認 (`XInput()` の呼び残し 2 ヘッダを `Gamepad().thumbRX` に修正)。**プレイヤー操作 (キーボード / パッド)、ショップ・イベントボード・ポーズ・キャラ選択・ステージ選択の部屋番号、大砲ゲージ演出、草・木・雲・格子バリアのシェーダー、天候フォグ、ロード画面の BGM フェードは入力が要るため手動確認待ち**。挙動が変わる箇所: `StageSelectPresenter` の部屋番号入力 (`KEY_INPUT_0 + digit` は DirectInput のキーコードが連番でないため上段 1〜9 とテンキーが効いていなかった。`Keyboard::IsDigitDown` で両方効く) |
| 2 | エンジン DLL 化 (Game は exe のまま) | Host exe へ WinMain 移動、`NANAMI_API` 付与 (382 クラス)、`SingletonBase` 7 クラスの `.cpp` 化、`IMGUI_API`、cereal パッチ適用、engine_dist 更新 | 4 構成が動き、Game モードの成果物と `GameBuilder` の手順が変わらない | 未着手 |
| 3 | Game.dll 化 + ホットリロード | `ConfigurationType` 切替、§4 のモジュール ID 付き Unregister、`PurgeExpired`、ウィンドウ群の削除関数、§5 の差し替え手順と保険モード、ビルド起動 UI | エディタ上で Game.dll を差し替え、開いていたシーンとウィンドウが戻る | 未着手 |

### 段階 A の実装メモ

- 置き換えは `tools/serialization/migrate_register_macros.py` で行う (手作業にしない)。既定の対象は `Engine` / `Packages` / `Libs/LibCore` / `Assets` の `.cpp`。
  再実行しても変更なし (冪等) で、古いエンジンから作ったプロジェクトの移行にも使える。§3.2 の 4 パターンと、行末 `;` の有無、複数行の引数を扱う。
  BOM は元の状態を保つ (BOM の無い 4 ファイルに BOM を足さない。無関係な差分を出さない)。
- 新しい `.h` / `.cpp` は `NanamiEngine.vcxproj` と `.filters` に手で追加する。
- ツール:
  - `tools/bt/catalog_scan.py:47` と `tools/animtree/catalog_scan.py:38` の `RE_REGISTER_TYPE` は `CEREAL_REGISTER_TYPE` しか読まない。
    `(?:NANAMI|CEREAL)_REGISTER_TYPE\s*\(\s*([\w:]+)\s*[,)]` のように新旧両方を読めるようにする。
  - `tools/bt/scaffold.py:179-180` の add-action の雛形も新マクロを出す。
  - `docs/BehaviourTree.md`、`docs/AnimationTree.md`、`.claude/skills/build/SKILL.md`、CLAUDE.md の「cereal registration goes in the .cpp」節を
    新マクロに合わせる。
- 確認:
  - マクロを 1 段挟んでも cereal の型名の文字列が変わらないことを確かめる (`binding_name<T>::name()` の値を置き換え前後で比較する小さなテスト、
    または既存の `.scene` / `.prefab` の `polymorphic_name` 一覧と突き合わせる)。
  - `regen-catalog` (bt / bt friendly / animtree) の結果が置き換え前と同じことを確かめる。
  - `tools/bt` と `tools/scene` の selftest には元から落ちる項目がある (bt の add-action 2 件、scene の `StageLoadingScene`・catalog fresh・
    `mark_` versioning)。変更前のツリーでも同じか比べて判断する。
  - 最終確認は MSVC のビルドと、既存のシーン・プレハブ・Behaviour Tree・AnimationTree をエディタで開くこと (許可を取ってから)。

---

## 11. 未検証事項 (実装前に PoC で確認すること)

1. §3.2 の cereal パッチで、エンジン側からゲーム型を含む多相ポインタが JSON・PortableBinary 双方で復元できること。
   → PoC で成立 (Linux)。Windows 実行が残り。実エンジンの `.scene` / `.prefab` での確認は段階 3。
2. `SerializationTypeRegistry` の記録だけで cereal の表から Game.dll 分を漏れなく消せること。
   → PoC で成立 (記録だけで表がベースラインに戻り、vtable による保険の掃除は 0 件)。
3. アンロード -> 再ロードを 10 回以上繰り返してもリークや dangling が無いこと (Application Verifier / `_CrtDumpMemoryLeaks` で確認)。
   → PoC で 10 サイクル、valgrind エラー 0 (Linux)。Windows の Debug ビルドは `_CRTDBG_LEAK_CHECK_DF` を有効にしてある。
4. Effekseer /MD 再ビルド物で、既存の全エフェクトが従来通り描けること。
5. `IMGUI_API` dllimport で `ImGuiHelper.h` (LibCore) と ImGuizmo が問題なく動くこと。
6. VS デバッガをアタッチしたまま Game.dll を差し替えてブレークポイントが効くこと (PDB コピー運用)。
7. 開いているシーンのメモリ上スナップショットが、Component のメンバ追加 (`CEREAL_CLASS_VERSION` を上げた場合) をまたいで復元できること。
