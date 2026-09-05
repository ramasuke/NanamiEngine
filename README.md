# NanamiEngine

DxLib / ImGui / Jolt Physics / cereal / enet をベースにした自作 C++ ゲームエンジン＋ゲーム本体です。
単一の Visual Studio プロジェクト `NanamiEngine.vcxproj`（toolset v143, C++20）で構成されており、
エディタもゲーム本体も同じ実行ファイルの中で動きます（ImGui によるインエディタ編集）。

ビルドシステムに glob 機構はなく、ソースファイル一覧を `.vcxproj` / `.vcxproj.filters` に
**手動で** 追加する必要があります（後述のツール群は、この手作業を安全に自動化するために存在します）。

## 特徴

- **エディタ内蔵**: ImGui ベースの Hierarchy / Project / Prefab / Inspector などのウィンドウ群で、
  シーン・Prefab・BehaviourTree・AnimationTree をゲームを止めずに編集できます。GameObject の
  Transform は ImGuizmo でハンドル操作できます。
- **コンポーネント指向の GameObject システム**: `IGameObject` を実装する `PrefabGameObject` /
  `SceneGameObject` / `CopiedPrefabGameObject` の3種類の GameObject に、`ComponentBase` 派生の
  コンポーネント（約65種類）を組み合わせて構築します。
- **物理演算**: Jolt Physics を統合。Box / Capsule / Cylinder / Sphere / StaticMesh の各種
  Collider に摩擦係数などのパラメータを持たせられます。
- **シリアライズ**: シーン・Prefab・BehaviourTree・AnimationTree・パーティクル効果はすべて cereal
  ベースの独自 JSON フォーマットで保存されます（フォーマットの詳細は `docs/` を参照）。
- **クライアント/サーバー型ネットワーク同期**: enet(UDP) を用い、Relay / Authoritative の2方式、
  LAN / Localhost の接続先切り替えに対応。任意の `SyncParameter<T>` 型やオブジェクトの Spawn、
  Transform・Animation・BehaviourTree の同期を扱えます。
- **敵 AI**: BehaviourTree による行動制御（Selector / Sequence / RandomSelector(重み付き) /
  Once系 / Action ノード）と、高さサンプリング付き2DグリッドをマルチスレッドでTheta*探索する
  経路探索を組み合わせています。
- **パーティクル**: Effekseer によるエフェクト（Sprite / Ring / Ribbon）。
- **ビルド時間対策**: プリコンパイル済みヘッダ（PCH）と Unity Build を導入済み。

## リポジトリ構成

### ルート

| パス | 内容 |
|---|---|
| `NanamiEngine.sln` / `NanamiEngine.vcxproj(.filters)` | 唯一のVisual Studioプロジェクト。ファイル一覧は手動管理 |
| `Main.cpp` / `stdafx.h` / `stdafx.cpp` | エントリポイントとプリコンパイル済みヘッダ |
| `ProjectConfig/` | アプリ・ネットワーク・物理演算の各種チューニング値をJSONで外出しした設定ツリー |
| `Engine/` | エンジン本体 |
| `Assets/` | ゲーム側のスクリプト・シーン・Prefab・データ・アート等 |
| `Libs/` / `Packages/` | サードパーティ／自社製の補助ライブラリ |
| `tools/` | シーン/Prefab・BehaviourTree・AnimationTree・Effekseer エフェクトを CLI から編集する Python 製オーサリングツール群 |
| `docs/` | BehaviourTree / AnimationTree のファイルフォーマットと運用ドキュメント |
| `.clangd` | MSVC専用ビルドだが、エディタ側の補完・定義ジャンプのために clangd 用のインクルードパスを別途定義 |

### Engine/

`Engine/Core`（アプリケーション／エディタ基盤・ネットワーク・ファイルシステムなど）と
`Engine/Module`（GameObject・Component・Asset・Physics などのドメインモジュール群）に分かれています。
（`Engine/Compiler`、`Engine/Render` はディレクトリのみ存在し、現状未使用です。）

`Engine/Module` 配下の主なサブシステム:

- **GameObject** — `IGameObject` インターフェースと `Transform`、階層のドラッグ&ドロップ並び替えを
  助ける `TreeDropZone`。GameObjectは用途別に3種類あります: Prefabのルートである
  `PrefabGameObject`、シーンに直接置かれる `SceneGameObject`、そして「シーンに配置された
  Prefabインスタンスの独立コピー」である `CopiedPrefabGameObject`。この3分類のおかげでシーン／
  Prefabファイルは常に「純粋な木構造（共有参照なし）」になります。
- **Component** — `ComponentBase` 派生の組み込みコンポーネント: `Animator`、`AudioSource`、
  `BlendImageRenderer`、`DirectionLight`、`ImageRenderer`（+`ImageAnimationRenderer`）、
  `ModelRenderer`、`ParticleSystem`（+`ParticleSystem_PlayMode`）、`SkyDome3D`、`SphereRenderer` など。
- **Physics** — Jolt Physics のラッパー（`Engine_Physics_Physics`、`ContactListener`、
  `BroadPhaseLayer`/`Layer`/`LayerFilter`、`RaycastHit` 等）。Collider は
  `ICollider`/`ColliderBase` を共通基底に持つ `BoxCollider` / `CapsuleCollider` /
  `CylinderCollider` / `SphereCollider` / `StaticMeshCollider`。Collider系は中間C++基底クラスが
  フィールドを持つ構造のため、`tools/scene add-component` では新規インスタンスを1から構築できない
  既知の制限があります。
- **Asset** — `AssetBase`/`Asset` 基底と `AssetFactory`。具体的なアセット種別として
  `HlslFile`(+`HlslPsFile`/`HlslVsFile`)、`MV1File`（3Dモデル）、`SoundFile`、`MovieFile`、
  `ParticleFile`（Effekseer）、`SpriteFile`(+`SpriteAnimationFile`)、`TtfFontFile`、`SceneFile`、
  `PrefabGameObjectFile`、`AnimationTreeFile` などがあります。アセットは大きく2系統に分かれ、
  `.meta` にguidとパスのみを持ち実体データは別ファイルに持つ「薄いプロキシ」型（`SceneFile` /
  `PrefabGameObjectFile` / `EnemyBehaviourFile` など）と、ペイロードを `.meta` に丸ごとインライン
  保存する `ScriptableObject` 派生の「厚い」型（例: `SwordManInitStatus`）があります。
- **AnimationTree** — `AnimationTree` 実行時クラス、ノード基底 `IAnimationNode`（現状具象実装は
  `AnimationClipNode` のみ）、遷移を表す `AnimationNodePath`。グラフエディタは
  `Engine/Core/Application/Window/Main/Animator/AnimatorWindow`。`Animator` コンポーネントの
  `animationTreeFile_` フィールドでGameObjectに紐付けます。
- **Scene** — `Scene` クラス、`ShadowMapSetting`。
- **Network**（Module層） — `Engine_Network_NetworkRunner` のみ。実体の大半は
  `Engine/Core/Network` にあります（後述）。
- **Gui / NanamiUI** — `Gui` はBehaviourTree/AnimationTreeエディタ双方が使う汎用ノードグラフ描画
  `GraphGui` とインスペクタ用のリフレクションヘルパー。`NanamiUI` はエディタではなく**ゲーム内UI**の
  ウィジェット群: `NanamiUi_Button`、`NanamiUi_Slider`、`TextRenderer`（Align変更機能あり）、
  `MovieRenderer`、`DrawBillboard3D`、`BlendAnmiationRenderer`、共通インターフェース
  `NanamiUi_IInteractivableRenderer`。
- **Guid** — `Guid`値型と、ハッシュマップキーとして使うための `GuidHash`。ObjectRegistryやネットワーク
  オブジェクトID、アセット参照などで全面的に使われています。
- **LifeCycleCallback** — オブジェクトが任意にオプトインするインターフェース群
  (`IAwakable`、`IBeginPhysics`、`IEnablableAsset`、`IFixedUpdatable`、`IPreFixedUpdate`、
  `IRenderable`、`IDebugRenderable`、`IInitRenderable`、`LateUpdate`) と、それらを
  Awake/FixedUpdate/Update/LateUpdate/Renderの各フェーズでディスパッチする
  `LifeCycleCallbackGroup`。
- **LocalPrefs / ProjectConfig / Log / Namespace / Color / ScriptableObject / 3DRender** —
  それぞれ、エディタ側のローカル設定（`Editor_ToolBar`含む）、`ProjectConfig/`配下のJSON設定読み込み、
  ログ出力（`Log.txt`）、名前空間ヘルパー、`Color32`値型、「厚い」アセット型の基底、デバッグ用の
  ギズモ形状描画 (`Shapes`) を担当します。

### Assets/Scripts

`Core/`（ゲームロジック本体）・`Editor/`（インゲームエディタ拡張）・`GamePlay/`（Coreの上に乗る
具体的なコンテンツ）・`Network/`（ゲーム固有パケット）に分かれています。

- **`Core/Game/PlayerAvatar/`** — `IPlayerAvatar`/`PlayerAvatar` を中心に、`Animator`、
  `AttackArea`、`CameraGroup`、`Chattable`、`Input`/`InputAction`、`Quest`、`State`/`StateMachine`
  （テンプレートベースからenum管理へリファクタ済み）、`Status`、`SwordMan` などのサブフォルダ。
- **`Core/Game/Npc/Enemy/Behaviour/Action/Content/`** — BehaviourTreeのActionリーフをカテゴリ別に
  整理: `Basic`（`ChasePlayerForPathFinding`、`ToPlayerDistance`、`ToPlayerRaycast`、
  `WanderMove`）、`Camera`、`EnemyStatus`（`Attack`、`Condition`、`OnDamage`、`OnDeath`、
  `PlayAnimation`）、`GameObject`（`RadiateProjectile`）、`Other`（`Random`、`ReadBlackBoard`、
  `WriteBlackBoard`）、`Particle`、`PlayerStatus`、`RigidBody`（`ChangeColliderEmotionType`、
  `SetVelocity`、`ShootDownAirShip` 等）、`Scene`、`Sound`（`PlayBGM`、`PlaySE`）、`Transform`、
  `Wait`。このカテゴリ分けがそのまま `tools/bt add-action --category` の値になります。
- **`Core/Game/PathFinding/HeightGridAstar/Multithread/`** — 高さサンプリング付き2Dグリッドを使う
  マルチスレッドTheta*経路探索。
- **`Editor/BehaviourTree/`** / **`Editor/Npc/{Enemy,Friendly}/Behaviour`** — インゲームの
  BehaviourTreeグラフエディタ本体（`tools/bt` はこれと同じデータへの、自動化向け代替インターフェース）。
- **`GamePlay/Npc/Enemy/`** — `FirstEventDragon`、`Hyena`、`NetworkBehaviourTree`
  （BehaviourTreeのネットワーク共有ラッパー）、`Projectile`、`TrainingDummy`、`Tyrannosaurus`。
- **`GamePlay/PlayerAvatar/`**（`Bullet`、`ChattableArea`、`SwordMan`）、
  **`GamePlay/Prop/`**（`AirShip`、`Canon`、`DestructibleObject`、`IslandPedestial`、
  `ProximityReveal`）、**`GamePlay/Ui/`**（`ActionInstructTutorial`、`BillBoardNpcChatIcon`、
  `DealDamageTextBillBoard`、`NpcChatting`、`OtherPlayerStatusUIGroup`、`PlayerStatus`、
  `StageSelect` 等）。
- **`Network/Packet/`** — エンジン共通のパケット基盤に乗る、ゲーム固有のパケット群
  (`Custom_PacketType`、`CustomPacketDispatcherBase/Group`) と、その上の
  `SpawnPlayer`/`SyncAvatarState`/`SyncBehaviourTree` ディスパッチモジュール。

### Assets/Data・Assets/Prefab（一例）

- `Assets/Data/EnemyBehaviour/*.enemyBehaviourData` — `DemoWolf`、`FirstEventDragon`、
  `HyenaBehaviour`、`T-Rex`、`TrainingDummy` など。
- `Assets/Data/HeightGridMap/` — 経路探索用の高さサンプリング付き2Dグリッド（ScriptableObject）。
- `Assets/Data/{EventNpcWalkingRoute, FriendlyNpcBehviour, FriendlyNpcStatus, FriendlyNpcWalkingRoute, NpcChatText, PlayerAvatar}/` —
  NPCの移動ルート・行動データ・ステータス初期値・会話テキストなど。
- `Assets/Prefab/` — `Bullet/CanonBullet`、`Npc/Enemy/{FirstEventDragon, Hyena}`、
  `Particle/`（`ElectricDust`、`ExplosionParticle`、`FireBallParticle`、`FootstepDust` 等）、
  `PlayerAvatar/Swordman/{CameraGroup, Swordman, SwordManStatusPresenter}`、
  `UI/`（`ActionInstructTutorialUI`、`ChattingUI`、`KnightStatusUI` 等）。

## ネットワークアーキテクチャ

enet(UDP) 上に構築されたクライアント/サーバー型モデルです（純粋なP2Pではありません）。

- **接続方式**（`ProjectConfig/Network/*.json` で設定）:
  - `NetworkMode`: `Server` / `Client`
  - `ServerType`: `Relay`（受信をそのままブロードキャスト） / `Authoritative`
    （サーバーが `OnServerReceive` をオーバーライドして再送信内容を判断）
  - `ConnectionTarget`: `Localhost` / `LAN`（`LanAddress` で接続先指定）
  - `MaxClients`、`UnreliableSendRate`（信頼性なし送信の間引きレート）
- **中核クラス**: `Engine/Core/Network/EnetUDPNetworkSystem` が `INetworkSystem` を実装し、
  `enet::ENetHost`/`ENetPeer` をラップ。信頼性なし送信用のアキュムレータと送信間隔制御、
  接続時に発火する rxcpp の `onConnectPlayer_` サブジェクトを持ちます。ローカル/オフライン用に
  `NullNetworkSystem` も存在します。
- **オブジェクト同期**: `NetworkObjectBase`/`INetworkObject` が同期用サブオブジェクトを
  `CreateSyncObject<T>`/`RegisterSyncObject<T>` で登録する仕組み。任意の型を同期できる
  `Network_SyncParameter`（`SyncParameter<T>`）、`NetworkObjectId`/`PlayerId`、Prefab GUIDから
  スポーンする `NetworkPrefabObjectRegistry`、生成済みオブジェクトを管理する
  `NetworkObjectInstanceRegistry`、毎フレーム処理対象を管理する `NetworkTickableRegistry`。
- **パケット**: `Packet_ByteBuffer`/`Packet_Codec`を基盤に、`Packet_Dispatch_PacketDispatcherBase`/
  `Packet_PacketDispatcherGroup` 経由でディスパッチ。エンジン組み込みの
  `AssignPlayerId`/`SpawnNetworkObject`/`SyncAnimation`/`SyncParameter`/`SyncTransform` に加え、
  ゲーム側で `SpawnPlayer`/`SyncAvatarState`/`SyncBehaviourTree` を追加しています。

## ビルド方法

**ユーザーから明示的に指示されない限り、自発的にビルドを実行しないこと。**

```
MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m
```

`-p:PreferredToolArchitecture=x64` は必須です。32bitコンパイラだと cereal の深いテンプレート
展開でヒープ不足になり `C1060` が発生します。MSBuild本体は
`C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe` です。

ビルド時間短縮のため、プリコンパイル済みヘッダ (`stdafx.h`/`stdafx.cpp`) と Unity Build を
導入しています。

### エディタツール向けの補助設定

MSVC専用ビルドですが、`.clangd` に `--driver-mode=cl` と `/std:c++20`、および
`Libs/{glm, プロジェクトに追加すべきファイル_VC用, cereal/include, ImGui, ImGuizmo, rxcpp,
JoltPhysics, tweeny}` のインクルードパスが設定されており、MSVC以外のエディタでも補完・
定義ジャンプが効くようになっています。

## ソースコードの文字コード

`.h`/`.cpp` は **UTF-8 with BOM** です。普通に読み書きしてよく、特別な変換は不要です
（MSVC v143 toolset はUTF-8-with-BOMをネイティブに読めます）。もしCP932(Shift-JIS)としては
デコードできるがUTF-8としてはデコードできないファイルを見つけたら、それは以前存在した
自動変換フックの名残りです。UTF-8 with BOMとして保存し直してください。

## コンテンツ制作ツール群 (`tools/`)

シーン/Prefab、BehaviourTree、AnimationTree、Effekseerエフェクトはcereal JSONを直接手編集せず、
以下のPythonツールキットから編集します。4つのツールは `tools/common/` の基盤を共有しています。

### `tools/scene`（シーン・Prefab・GameObject・Component）

```
python -m tools.scene new-scene NAME [--dir]
python -m tools.scene new-prefab NAME [--dir]
python -m tools.scene copy-prefab SOURCE [--name] [--dir]   # 新規guidで複製
python -m tools.scene show|validate FILE
python -m tools.scene add-gameobject|remove-gameobject|move-gameobject FILE ...  # move はデフォルトでワールド変形を維持
python -m tools.scene set-transform|set-active|rename-gameobject FILE ...
python -m tools.scene add-component|remove-component|set-component-params FILE ...
python -m tools.scene instantiate-prefab PREFAB --into FILE [--parent]
python -m tools.scene apply FILE OPS.json     # 複数操作をアトミックに適用する主インターフェース
python -m tools.scene regen-catalog [--check]
python tools/scene/selftest.py
```

すべての変更系コマンドは `--dry-run` に対応。既知のv1制限: 対象ファイルは常に「純粋な木構造」の
前提（共有参照は表現できない）、カタログは `ComponentBase` の直接派生（約65種）のみ対応、
設定可能なパラメータ型は `int|float|bool|string|vec2|vec3|field` に限定、Collider等の中間C++基底
を持つコンポーネントは新規追加不可、`add-component-type` のような雛形生成コマンドはまだない。

### `tools/bt`（敵BehaviourTree・Action）

```
python -m tools.bt new-tree NAME
python -m tools.bt show|validate FILE
python -m tools.bt add-node|remove-node|move-node FILE ...   # デフォルトで自動レイアウト（--no-layoutで無効化）
python -m tools.bt layout FILE [--dx] [--dy]
python -m tools.bt set-params|set-weight FILE ...
python -m tools.bt add-bb-param|remove-bb-param FILE ...
python -m tools.bt apply FILE OPS.json
python -m tools.bt add-action --name X --category "<Cat>" [--param n:type=default ...]
python -m tools.bt remove-action ...
python -m tools.bt regen-catalog [--check]
python tools/bt/selftest.py
```

既知のv1制限: 純粋な木構造前提、ブラックボードは `AnimationParameter<int>` のみ、
`nested:*`/`unknown` 形状のパラメータはCLIから設定不可。

### `tools/animtree`（AnimationTree）

```
python -m tools.animtree new-tree NAME        # Entry + AnyState のみを持つ最小構成
python -m tools.animtree show|validate FILE
python -m tools.animtree add-clip-node|remove-node|set-node-params|move-node FILE ...
python -m tools.animtree add-transition|remove-transition|set-transition-params FILE ...  # 遷移はguidを持たず位置で指定
python -m tools.animtree add-condition|remove-condition FILE ...
python -m tools.animtree add-param|remove-param|set-param FILE ...
python -m tools.animtree apply FILE OPS.json
python -m tools.animtree regen-catalog [--check]
python tools/animtree/selftest.py
```

既知のv1制限: 一般グラフ構造のためクロスノード自動レイアウトはなし、`add-node-type` に相当する
雛形生成コマンドはなし（新しい`IAnimationNode`派生の追加はエンジン側ImGuiメニューの分岐も含め
手作業。`docs/AnimationTree.md` §4参照）。

### `tools/effect`（Effekseerパーティクル）

```
python -m tools.effect new-project NAME
python -m tools.effect show FILE              # ノードをドット区切りの子インデックスパスで表示
python -m tools.effect validate FILE
python -m tools.effect add-node --kind sprite|ring|ribbon|group FILE ...
python -m tools.effect set-params FILE ...
python -m tools.effect apply FILE OPS.json
python -m tools.effect compile FILE           # .efkproj -> .efkefc（Effekseer 1.7.3.0 CUI、環境固有）
python -m tools.effect install EFKEFC --dest Assets/Art/Effect/<Sub>/<Name>.efkefc [--project]
python tools/effect/selftest.py
```

モデル化されているノード種別は `Sprite`/`Ring`/`Ribbon` のみ。`install` は
`tools/common/meta_base.py` 経由で新規guidの `.efkefc.meta`（`ParticleFile`）を発行し、
`--project` 指定時は `.efkproj` ソースを `Assets/Art/Effect/_Source/` にコミットします。

### `tools/common`（4ツール共通基盤）

- `cereal_json.py` — cerealの `JSONOutputArchive` 方言（RapidJSON PrettyWriter、4スペースインデント、
  UTF-8 no-BOM、CRLF、末尾改行なし、Grisu2の数値表現の癖を含む）をバイト単位で再現するリーダー/
  プリンター。
- `blob.py` — cerealオブジェクトの部分木を `Ptr`/`Ver` でタグ付けした表現。polymorphic_idや
  バージョン管理のカウンタを、構造編集後も位置に依存せず再生成できます。
- `meta_base.py` — 「薄いプロキシ」型 `AssetBase` 用の汎用 `.meta` サイドカーコーデック
  (`MetaSpec`)。
- `vcxproj.py` — 約1MBある手動管理の `.vcxproj`/`.vcxproj.filters` へのバイト安全なテキスト挿入
  （アンカーベースの正規表現挿入、BOM/CRLF維持、要素数差分アサーションと不一致時のロールバック）。
- `diffcheck.py` — ラウンドトリップのセルフテスト用に、cerealオブジェクトを構造的に比較
  （Transformの繰り返し `"child"` キーのような同名兄弟キーにも対応）。

## ドキュメント (`docs/`)

- **`docs/BehaviourTree.md`** — データの置き場所（`.enemyBehaviourData`、実行時クラス
  `Enemy_BehaviourTree`、Action基底 `Enemy_Behaviour_ActionBase`、エディタ側集約ヘッダ
  `Enemy_Behaviour_ActionHeaders.h`）、`.enemyBehaviourData` の完全なフォーマット仕様
  （polymorphic_id / ptr_wrapper / cereal_class_version の記法、`SelectorNode`/`SequenceNode`/
  `RandomSelectorNode`（重み付き選択にフォールバックがない点の注意）/`OnceExecute`/
  `OnceSuccessNode`/`ActionNode` の各ノード型、ブラックボードパラメータの書式）、
  ディスク上のフォーマット（UTF-8・BOMなし・CRLF・末尾改行なし・4スペースインデント）。
- **`docs/AnimationTree.md`** — データの置き場所（`Assets/Animations/*.animTree`、実行時
  `AnimationTree.{h,cpp}`、`AnimationTree/Node/**` のノード型、`NodePath/**` の遷移、エディタ
  `AnimatorWindow`）、`Animator` コンポーネントの `animationTreeFile_` を介した紐付け方法、
  トップレベル6キーの完全なフォーマット仕様（`nodes_0..N-1` の順序は非意味的で `unordered_map`
  裏付け、`fromNodeNodePath_*`/`fromAnyStateNodeNodePath_*` 等）、ノード型一覧
  (`AnimatorEntryNode`、`AnimationVisualAnyStateNode`、`AnimationClipNode`（cereal version 2、
  v1/v2で追加されたフィールドを含む）)。

## サードパーティ依存

**`Libs/`**: `ImGui`、`ImGuizmo`（Transformのギズモ操作）、`JoltPhysics`、`LibCore`（自社製補助
ライブラリ: `BlackBoard`、DxLibラッパー、`FilePathHelper`、ImGuiヘルパー、rxcppの
`ReadOnlyReactiveContext`/`SerializableSubject`、Tween(`Ease`含む)、cereal用glmアダプタ等）、
`Singleton`、`cereal`、`enet`、`glm`、`rxcpp`、`tweeny`。

**`Packages/`**（Unityのパッケージ命名を意識した自社製の任意追加モジュール）:
- `Cinemachine/` — `CinemachineCameraBrain`/`CineMachineVirtualCamera` によるカメラブレイン＋
  バーチャルカメラシステム。
- `R4/` — `SensorEnterableAsObservable`/`SensorExitableAsObservable`/`SensorStayableAsObservable`。
  物理センサー/トリガーのenter/stay/exitイベントをrxcppのobservableでラップ。

`stdafx.h` から実際に使われているのは DxLib、EffekseerForDXLib、ImGuiHelper、rxcpp (`rx.hpp`)、
Jolt (`Jolt/Jolt.h`)、glm (`vec2.hpp`/`vec3.hpp`/`fwd.hpp`)、および C++20 標準ライブラリ
（`<coroutine>`/`<ranges>` を含む）です。

## 開発状況

現在活発に開発が進んでいる領域: BehaviourTree/AnimationTree/シーン編集をAIエージェントが
`tools/` 経由で直接操作できるようにする仕組み、ImGuizmoによるTransform編集やHierarchy/Project/
Prefabウィンドウの検索機能などエディタUXの強化、ネットワーク同期（`SyncParameter<T>`・
オブジェクトIDの割り当て・BehaviourTreeのネットワーク共有）、高さサンプリング付き2Dグリッドと
Theta*を使った経路探索まわりのバグ修正です。詳細は `git log` を参照してください。

## ライセンス

現時点でこのリポジトリにライセンスファイルは存在しません。
