# NanamiEngine

DxLib / ImGui / Jolt Physics / cereal / enet をベースにした自作 C++ ゲームエンジン＋ゲーム本体です。
単一の Visual Studio プロジェクト `NanamiEngine.vcxproj`（toolset v143, C++20）で構成されており、
エディタもゲーム本体も同じ実行ファイルの中で動きます（ImGui によるインエディタ編集）。

ビルドシステムに glob 機構はなく、ソースファイル一覧を `.vcxproj` / `.vcxproj.filters` に
**手動で** 追加する必要があります（後述のツール群は、この手作業を安全に自動化するために存在します）。

## 特徴

- **エディタ内蔵**: ImGui ベースの Hierarchy / Project / Prefab / Inspector / Console などの
  ウィンドウ群で、シーン・Prefab・BehaviourTree・AnimationTree をゲームを止めずに編集できます。
  GameObject の Transform は ImGuizmo でハンドル操作でき、Hierarchy / Project / Prefab ウィンドウには
  検索ボックスがあります。`.mv1` モデル／アニメーションのプレビュー（`ModelViewWindow` /
  `AnimationViewWindow`）、実行中の AnimationTree・BehaviourTree（Enemy / FriendlyNpc）の可視化、
  ネットワークパケットの送受信ログ（`NetworkLoggerWindow`）も備えています。
- **コンポーネント指向の GameObject システム**: `IGameObject` を実装する `PrefabGameObject` /
  `SceneGameObject` / `CopiedPrefabGameObject` の3種類の GameObject に、`ComponentBase` 派生の
  コンポーネント（`ENGINE_REGISTER_COMPONENT` で登録されたもの約85種類）を組み合わせて構築します。
- **物理演算**: Jolt Physics を統合。Box / Capsule / Cylinder / Sphere / StaticMesh の各種
  Collider に摩擦係数などのパラメータを持たせられ、Collision / Sensor の Enter・Exit(・Stay)
  コールバックを受け取れます。
- **シリアライズ**: シーン・Prefab・BehaviourTree・AnimationTree はすべて cereal ベースの独自 JSON
  フォーマットで保存されます（フォーマットの詳細は `docs/` を参照）。cereal / iostream 由来の例外は
  エンジン共通の例外階層（`Engine/Module/Exception`）に変換されます。
- **クラッシュ耐性**: `SafeExecute` がライフサイクルコールバック（Update / Render など）を
  オブジェクト単位で C++例外・SEH例外（nullptr参照など）から保護し、1つのコンポーネントの不具合で
  エディタ全体が落ちないようにしています（`ProjectConfig/Application/CrashRecoveryEnabled.json` /
  `DebuggerFailFastEnabled.json` / `BreakOnLogErrorEnabled.json` で挙動を切り替え可能）。
- **クライアント/サーバー型ネットワーク同期**: enet(UDP) を用い、Relay / Authoritative の2方式、
  LAN / Localhost の接続先切り替えに対応。任意の `SyncParameter<T>` 型やオブジェクトの Spawn、
  Transform・Animation の同期、所有者(権威)管理とクライアント離脱時の所有権移譲、汎用RPCを扱えます。
- **NPC AI**: 敵（Enemy）と味方NPC（FriendlyNpc）の2系統の BehaviourTree による行動制御
  （Selector / Sequence / RandomSelector(重み付き) / Once系 / Action ノード）と、高さサンプリング付き
  2DグリッドをマルチスレッドでTheta*探索する経路探索を組み合わせています。
- **コルーチン**: C++20 コルーチンによる `Task` / `CoroutineScheduler` と、`WaitForSeconds` /
  `WaitUntil` / `WaitForTween` / `WaitForObservable` などの Awaitable 群（`Engine/Core/Coroutine`）。
- **パーティクル**: Effekseer によるエフェクト（`tools/effect` で Sprite / Ring / Ribbon / Model /
  Track ノードを CLI から作成可能）。
- **ビルド時間対策**: プリコンパイル済みヘッダ（PCH）と Unity Build を導入済み。

## リポジトリ構成

### ルート

| パス | 内容 |
|---|---|
| `NanamiEngine.sln` / `NanamiEngine.vcxproj(.filters)` | 唯一のVisual Studioプロジェクト。ファイル一覧は手動管理 |
| `Main.cpp` / `stdafx.h` / `stdafx.cpp` | エントリポイントとプリコンパイル済みヘッダ |
| `ProjectConfig/` | `Application`（ウィンドウサイズ・固定更新レート・ライト・シャドウマップ・クラッシュ回復設定など）/ `DebugDraw`（Collider表示）/ `Network` / `Physics` の各種設定値をJSONで外出しした設定ツリー |
| `Engine/` | エンジン本体 |
| `Assets/` | ゲーム側のスクリプト・シーン・Prefab・データ・アート等 |
| `Libs/` / `Packages/` | サードパーティ／自社製の補助ライブラリ |
| `tools/` | シーン/Prefab・BehaviourTree・AnimationTree・Effekseer エフェクト・MV1モデル変換を CLI から扱う Python 製オーサリングツール群と、UI素材を生成する `tools/art` |
| `docs/` | BehaviourTree / AnimationTree のファイルフォーマットと運用ドキュメント |
| `Logs/` | Console / NetworkLogger ウィンドウから保存したログの出力先（実行時ログ本体はルートの `EngineLog.txt`） |
| `.clangd` | MSVC専用ビルドだが、エディタ側の補完・定義ジャンプのために clangd 用のインクルードパスを別途定義 |

### Engine/

`Engine/Core`（アプリケーション／エディタ基盤・ネットワーク・ファイルシステムなど）と
`Engine/Module`（GameObject・Component・Asset・Physics などのドメインモジュール群）に分かれています。
（`Engine/Compiler`、`Engine/Render` はディレクトリのみ存在し、現状未使用です。）

`Engine/Core` 配下の主なサブシステム:

- **Application** — `ApplicationBase` と `EditorApplication` / `GameApplication`、設定読み込み
  （`Configuration/{DebugDraw,Network,Physics}`）、エディタ用フリーカメラ、ウィンドウ群。
  ウィンドウは「メインウィンドウ」（独自のライフサイクルを持ち、編集対象の型ごとに開く `MainWindowBase<T>` 派生:
  `GameWindow`（Hierarchy を含むシーン編集）、`PrefabViewWindow`、`AnimatorWindow`、
  `ModelViewWindow`、`AnimationViewWindow`）と「ポップアップウィンドウ」（`IPopupWindow` 派生:
  `InspectorWindow`、`ProjectWindow`、`ConsoleWindow`、`NetworkLoggerWindow`、
  `RunningAnimationTreeWindow`）、`EditorToolbarWindow` に分かれます。
- **Coroutine** — C++20 コルーチンの `Task<T>`、`CoroutineScheduler`、非同期実行用 `FutureTask`、
  Awaitable群（`WaitForSeconds`、`WaitUntil`、`WaitForTween`/`WaitForTweenV`、`WaitForObservable`、
  `WaitForSubscription`、`Yield`）。
- **FileSystem** — `Directory`/`File` と、Projectウィンドウからのドラッグ&ドロップを扱う
  `EditorDraggingHand`。
- **Object** — 全オブジェクトの基底 `IObject`、guid→オブジェクトの弱参照テーブル `ObjectRegistry`、
  アセット・GameObject・Componentをguidで参照するシリアライズ可能な参照型 `Field<T>`（`FIELD(T)` マクロ）。
  Prefab内の子オブジェクトもこれで参照する（Instantiate 時は `GuidRemap` が複製先の guid に貼り替える）。
- **Network** — enet ラッパーとネットワークオブジェクト基盤（詳細は「ネットワークアーキテクチャ」）。

`Engine/Module` 配下の主なサブシステム:

- **GameObject** — `IGameObject` インターフェースと `Transform`、階層のドラッグ&ドロップ並び替えを
  助ける `TreeDropZone`。GameObjectは用途別に3種類あります: Prefabのルートである
  `PrefabGameObject`、シーンに直接置かれる `SceneGameObject`、そして「シーンに配置された
  Prefabインスタンスの独立コピー」である `CopiedPrefabGameObject`。この3分類のおかげでシーン／
  Prefabファイルは常に「純粋な木構造（共有参照なし）」になります。
- **Component** — `ComponentBase` 派生の組み込みコンポーネント: `Animator`、`AudioSource`、
  `BlendImageRenderer`、`DirectionLight`、`ImageRenderer`（+`ImageAnimationRenderer`）、
  `ModelRenderer`、`ParticleSystem`（+`ParticleSystem_PlayMode`、`Component/ParticleRenderer/` 配下）、
  `QuadRenderer`（`.mv1` を使わずシェーダー付き板ポリゴンを描く軽量レンダラー）、`Rotator`（汎用回転）、
  `ScreenColorGradeRenderer`（UI描画フェーズで画面を取り込み彩度・明度フィルタをかける）、`SkyDome3D`、
  `SphereRenderer` など。
  カスタムシェーダー用の定数バッファを持つレンダラーは `IShaderConstantBufferHost` を実装します。
- **Physics** — Jolt Physics のラッパー（`Engine_Physics_Physics`、`ContactListener`、
  `BroadPhaseLayer`/`Layer`/`LayerFilter`、`RaycastHit`、`UserData` 等）。Collider は
  `ICollider`/`ColliderBase` を共通基底に持つ `BoxCollider` / `CapsuleCollider` /
  `CylinderCollider` / `SphereCollider` / `StaticMeshCollider`。接触イベントは
  `ICollisionEnterable`/`ICollisionExitable`/`ISensorEnterable`/`ISensorExitable`/`ISensorStayable`
  を実装するか、`CollisionListener` コンポーネントの rxcpp observable（`OnCollisionEnterAsObservable`
  等）で受け取ります。Collider系は中間C++基底クラスがフィールドを持つ構造のため、
  `tools/scene add-component` では新規インスタンスを1から構築できない既知の制限があります。
- **Asset** — `AssetBase`/`Asset` 基底と `AssetFactory`。具体的なアセット種別として
  `HlslFile`(+`HlslPsFile`/`HlslVsFile`)、`MV1File`（3Dモデル）、`SoundFile`、`MovieFile`、
  `ParticleFile`（Effekseer）、`SpriteFile`(+`SpriteAnimationFile`)、`TtfFontFile`、`SceneFile`、
  `PrefabGameObjectFile`、`AnimationTreeFile` などがあります。アセットは大きく2系統に分かれ、
  `.meta` にguidとパスのみを持ち実体データは別ファイルに持つ「薄いプロキシ」型（`SceneFile` /
  `PrefabGameObjectFile` / `EnemyBehaviourFile` など）と、ペイロードを `.meta` に丸ごとインライン
  保存する `ScriptableObject` 派生の「厚い」型（例: `SwordManInitStatus`）があります。
- **AnimationTree** — `AnimationTree` 実行時クラス、ノード基底 `IAnimationNode`（特殊ノードの
  `AnimatorEntryNode`/`AnimationVisualAnyStateNode` を除く具象実装は現状 `AnimationClipNode` のみ）、
  遷移を表す `AnimationNodePath` と遷移条件 `AnimationNodePathAdditionCondition(Group)`。グラフエディタは
  `Engine/Core/Application/Window/Main/Animator/AnimatorWindow`、実行中の状態は
  `RunningAnimationTreeWindow` で確認できます。`Animator` コンポーネントの `animationTreeFile_`
  フィールドでGameObjectに紐付けます。
- **Scene** — `Scene` クラス、`ShadowMapSetting`、シーン直置き用の `SceneGameObject` /
  `CopiedPrefabGameObject`。
- **Network**（Module層） — `NetworkRunnerBase`（`Engine_Network_NetworkRunner`）、同期処理を持つ
  コンポーネントの基底 `NetworkComponent`、組み込みのネットワークコンポーネント `NetworkGameObject` /
  `NetworkTransform` / `NetworkAnimator`、汎用RPC（`Rpc/`）、パケットログ（`PacketLog` /
  `PacketTypeNameRegistry`）。下位層の実体は `Engine/Core/Network` にあります（後述）。
- **Gui / NanamiUI** — `Gui` はBehaviourTree/AnimationTreeエディタ双方が使う汎用ノードグラフ描画
  `GraphGui` とインスペクタ用のリフレクションヘルパー。`NanamiUI` はエディタではなく**ゲーム内UI**の
  ウィジェット群: `NanamiUi_Button`、`NanamiUi_Slider`、`TextRenderer`（Align変更機能あり）、
  `MovieRenderer`、`Billboard3D`/`BillboardAnimation3D`、`BlendAnimationRenderer`、
  `HorizontalLayoutGroup`/`VerticalLayoutGroup`/`GridLayoutGroup`、マウス状態 `MouseState`、
  共通インターフェース `NanamiUi_IInteractivableRenderer`。
- **Guid** — `Guid`値型と、ハッシュマップキーとして使うための `GuidHash`。ObjectRegistryやネットワーク
  オブジェクトID、アセット参照などで全面的に使われています。
- **LifeCycleCallback** — オブジェクトが任意にオプトインするインターフェース群
  (`IAwakable`、`IStartable`、`IBeginPhysics`、`IEndPhysics`、`IEnablableAsset`、`IFixedUpdatable`、
  `IPreFixedUpdate`、`IUpdatable`、`LateUpdate`、`IRenderable`、`IShadowRenderable`、
  `IUserInterfaceRenderable`、`IDebugRenderable`、`IInitRenderable`) と、それらを
  Awake/Start/FixedUpdate/Update/LateUpdate/Renderの各フェーズでディスパッチする
  `LifeCycleCallbackGroup`（優先度順の `SortCallbackGroup`、1回だけ呼ぶ `OnceCallbackGroup` を含む）。
  各コールバックは `SafeExecute` 経由で呼ばれます。
- **Exception / SafeExecute / Serialization** — エンジン共通の例外階層（`NanamiException` を基底に
  `SerializationException` 等）、C++例外・SEH例外から処理を保護する `SafeExecute`、cereal / iostream の
  例外をエンジン例外へ変換するシリアライズヘルパー。
- **LocalPrefs / ProjectConfig / Log / Namespace / Color / ScriptableObject / 3DRender** —
  それぞれ、エディタ側のローカル設定（`Editor_ToolBar`含む）、`ProjectConfig/`配下のJSON設定読み込み、
  ログ出力（`EngineLog.txt`。ConsoleWindowにも表示）、名前空間ヘルパー、`Color32`値型、「厚い」アセット型の
  基底、デバッグ用のギズモ形状描画 (`Shapes`) を担当します。

### Assets/Scripts

`Core/`（ゲームロジック本体。ゲーム固有のネットワークパケット/RPCは `Core/Network/`）・
`Editor/`（インゲームエディタ拡張）・`GamePlay/`（Coreの上に乗る具体的なコンテンツ）に分かれています。

- **`Core/Game/PlayerAvatar/`** — `IPlayerAvatar`/`PlayerAvatar` を中心に、`Animator`、
  `AttackArea`、`CameraGroup`、`Chattable`、`Input`/`InputAction`、`LockOnTarget`（ロックオン対象
  `ILockOnTarget`/`LockOnPoint`）、`Quest`、`State`/`StateMachine`（テンプレートベースからenum管理へ
  リファクタ済み）、`Status`、`Wakeable`（ダウン状態からの蘇生 `IPlayerWakeable`）、`SwordMan` などの
  サブフォルダ。`SwordMan/State/` には `Idle`/`Walk`/`Run`/`Jump`/`Attack`/`AvoidRolling`/`Hurt`/
  `Down`/`WakeUp`/`Death`/`UseCanon`/`ClimbToTop` などのステートがあります。
- **`Core/Game/Npc/Enemy/`** — 敵の基底 `EnemyBase`、ボス共通の基底 `Boss/BossEnemyBase`（ボスHPゲージ）、ステータス、`Behaviour/`（BehaviourTree実行時）。
  `Behaviour/Action/Content/` はActionリーフをカテゴリ別に整理しています:
  `Basic`（`ChasePlayerForPathFinding`、`ToPlayerDistance`、`ToPlayerRaycast`、`WanderMove`）、
  `Camera`（`PurposeCamera`、`ScenePurposeCamera`、`ShakeCamera`）、`EnemyStatus`（`Attack`、
  `Condition`、`OnDamage`、`OnDeath`、`PlayAnimation`）、`GameObject`（`RadiateProjectile`）、
  `Other`（`Chat`、`Random`、`ReadBlackBoard`、`WriteBlackBoard`）、`Particle`（`GenerateParticle`）、
  `PlayerStatus`（`CheckCompleteQuest`）、`RigidBody`（`ChangeColliderEmotionType`、`ChangeIsGravity`、
  `MoveEventRoute`、`MoveToPlayerPos`、`SetVelocity`、`ShootDownAirShip`）、`Scene`
  （`ChangeToMainIslandScene`）、`Sound`（`PlayBGM`、`PlaySE`）、`Transform`（`Rotation`）、
  `Wait`（`Seconds`）。このカテゴリ分けがそのまま `tools/bt add-action --category` の値になります。
- **`Core/Game/Npc/Friendly/`** — 味方NPC用BehaviourTree（`Friendly_BehaviourTree`）。Actionは
  `Behaviour/Action/Content/` 配下に `Camera`（`EnablePurposeCamera`）、`GameObject`（`Instantiate`、
  `SetEnable`）、`NpcStatus`（`Animation`、`Chat`、`ChatIcon`、`RigidBody`、`Transform`）、`Other`、
  `PlayerStatus`（`Quest`）、`Time`（`Timer`）のカテゴリで整理されています
  （`tools/bt add-action --npc-kind friendly` の対象）。
- **`Core/Game/Scene/`** — ゲームの流れを管理するシーン層。`Main/`（`Title`、`MainIslandScene`、
  `FirstTouchDownMainIsLand`、`GrassLand`、`DrySand` などのメインシーン）と、`Push`/`Pop` で
  スタックに積む `Sub/`（`ChattingUI`、`OtherPlayerStatusUI` などのサブシーン）に分かれます。
- **`Core/Game/{Damage, StatusParameter, MainProgression, Settings}`** — ダメージ（`PhysicsPower` 含む）、
  `Health`/`Stamina`/`MoveSpeed` のステータス値、ゲーム進行度の保存/読み込み、ゲーム設定。
- **`Core/Game/PathFinding/HeightGridAstar/Multithread/`** — 高さサンプリング付き2Dグリッドを使う
  マルチスレッドTheta*経路探索。
- **`Editor/BehaviourTree/`** / **`Editor/Npc/{Enemy,Friendly}/Behaviour`** — インゲームの
  BehaviourTreeグラフエディタ本体（`EnemyNpcBehaviourWindow` / `FriendlyNpcBehaviourWindow`）と、
  実行中のツリーの状態を可視化する `RunningEnemyBehaviourTreeWindow` /
  `RunningFriendlyBehaviourTreeWindow`（`tools/bt` はこれと同じデータへの、自動化向け代替インターフェース）。
- **`GamePlay/Npc/Enemy/`** — `FirstEventDragon`、`Hyena`、`NetworkBehaviourTree`
  （付与されている個体だけBehaviourTreeのTickを権威側限定にするマーカーコンポーネント）、
  `Projectile`、`TrainingDummy`、`Tyrannosaurus`。**`GamePlay/Npc/Friendly/`** — `FriendlyNpc`。
- **`GamePlay/AttackArea/`** — センサー内の対象にダメージを与える攻撃範囲（`NetworkComponent`）。
  ダメージは「対象を自ピアが所有している場合」にだけ適用する被弾側判定で、敵の攻撃発火は
  `AttackAreaFire` RPCで各ピアの同じ `AttackArea` 上に再現されます。
- **`GamePlay/PlayerAvatar/`**（`Bullet`、`ChattableArea`、`HitShakeReceiver`、
  `LockOnDetectionArea`、`SwordMan`、`WakeUpArea`（ダウンした味方の蘇生範囲））、
  **`GamePlay/Prop/`**（`AirShip`、`Canon`、`Cloud`、`DestructibleObject`、`IslandPedestial`、
  `LatticeBarrier`、`ProximityReveal`）、**`GamePlay/Ui/`**（`ActionInstructTutorial`、
  `BillBoardNpcChatIcon`、`BossHealthGauge`、`DealDamageTextBillBoard`、`LockOnReticle`、`NpcChatting`、
  `OtherPlayerStatusUIGroup`、`PlayerStatus`（`Ui_DamageFlash`、`Ui_LowHealthScreenEffect` 等）、
  `StageSelect`（Model/Presenter/MapMarker に分割）等）、**`GamePlay/{Network, Spawn, Sound}`**
  （`Game_CustomNetworkRunner`、`PrefabSpawner`、`SoundPlayer`）。
- **`Core/Network/Packet/`** — エンジン共通のパケット基盤に乗る、ゲーム固有のパケット群
  (`Custom_PacketType`、`CustomPacketDispatcherBase/Group`) と、その上の
  `SpawnPlayer`/`SpawnEnemy` ディスパッチモジュール。
- **`Core/Network/Rpc/`** — ゲーム固有の汎用RPC定義（`Custom_RpcType.h`）と各RPCの実装モジュール
  （後述「汎用RPC」参照）。

### Assets/Data・Assets/Prefab・Assets/Scene ほか（一例）

- `Assets/Data/EnemyBehaviour/*.enemyBehaviourData` — `DemoWolf`、`FirstEventDragon`、
  `HyenaBehaviour`、`T-Rex`、`TrainingDummy`、`Tyrannosaurus` など。
- `Assets/Data/FriendlyNpcBehviour/*.friendBehaviourData` — `ActionInstructure`、`Adventure`、
  `IdleActionInstructure`、`SampleAppearDragon` など（ディレクトリ名の綴りは `Behviour`）。
- `Assets/Data/HeightGridMap/` — 経路探索用の高さサンプリング付き2Dグリッド（ScriptableObject）。
- `Assets/Data/Stage/*.stageData` — ステージ選択UIで使うステージ定義（`GrasslandStage`、`DesertStage`、
  `RockyStage`）。本体ファイルは空で、値は `.meta` 側に保存される ScriptableObject 型です。
- `Assets/Data/{EventNpcWalkingRoute, FriendlyNpcStatus, FriendlyNpcWalkingRoute, NpcChatText, PlayerAvatar}/` —
  NPCの移動ルート・ステータス初期値・会話テキスト、プレイヤーアバターの初期ステータス/リソース/Factoryなど。
- `Assets/Animations/` — `SwordManAnimation.animTree`、`Enemy/{FirstEventDragon, Hyena, Tyrannosaurus}.animTree` など。
- `Assets/Prefab/` — `Bullet/CanonBullet`、`Npc/Enemy/{FirstEventDragon, FirstEventDragonFireBall, Hyena, Tyrannosaurus}`、
  `Particle/`（`DragonDefeatSparkle`、`ElectricDust`、`ExplosionParticle`、`FireBallParticle`、
  `FootstepDust`、`SwordManCharge*` 等）、
  `PlayerAvatar/Swordman/{CameraGroup, Swordman, SwordManStatusPresenter}`、`PlayerAvatar/OtherPlayerStatusPresenter`、
  `UI/`（`ActionInstructTutorialUI`、`BossHealthGaugeUI`、`ChattingUI`、`KnightStatusUI`、`StageSelectUI`、
  `SwordManStatusUI` 等）。
- `Assets/Scene/` — `TitleScene`、`LoadingScene`、`GameManage`、`MainIslandScene`、
  `FirstTouchDownMainIsLandScene`、`GrassLandScene`、UI用サブシーン（`ChattingUiScene`、
  `OtherPlayerStatusUiScene`）、検証用の `_Test/`。

## ネットワークアーキテクチャ

enet(UDP) 上に構築されたクライアント/サーバー型モデルです（純粋なP2Pではありません）。

- **接続方式**（`ProjectConfig/Network/*.json` で設定）:
  - `NetworkMode`: `Server` / `Client`
  - `ServerType`: `Relay`（受信をそのままブロードキャスト。Dispatcherの `OnServerRelayReceive`） /
    `Authoritative`（サーバーで処理し、Dispatcherが `OnServerAuthoritativeReceive` をオーバーライドして
    検証・選択的な再送信を行う）
  - `ConnectionTarget`: `Localhost` / `LAN`（`LanAddress` で接続先指定）
  - `MaxClients`、`UnreliableSendRate`（信頼性なし送信の間引きレート）
- **中核クラス**: `Engine/Core/Network/EnetUDPNetworkSystem` が `INetworkSystem` を実装し、
  `enet::ENetHost`/`ENetPeer` をラップ。信頼性なし送信用のアキュムレータと送信間隔制御、
  接続時に発火する rxcpp の `onConnectPlayer_` サブジェクトを持ちます。ローカル/オフライン用に
  `NullNetworkSystem` も存在します。
- **オブジェクト同期**: `NetworkObjectBase`/`INetworkObject` が同期用サブオブジェクトを
  `CreateSyncObject<T>`/`RegisterSyncObject<T>` で登録する仕組み。任意の型を同期できる
  `Network_SyncParameter`（`SyncParameter<T>`）、`NetworkObjectId`/`PlayerId`/`RpcId`、Prefab GUIDから
  スポーンする `NetworkPrefabObjectRegistry`、生成済みオブジェクトと所有者を管理する
  `NetworkObjectInstanceRegistry`、毎フレーム処理対象を管理する `NetworkTickableRegistry`。
- **ネットワークコンポーネント**: GameObject側では `NetworkRunnerBase`（シングルトンの窓口。ゲーム側は
  `Game_CustomNetworkRunner`）を起点に、`NetworkGameObject`（Spawn対象Prefabのルートに付けて
  `NetworkObjectId` を保持）、`NetworkTransform`、`NetworkAnimator` を組み合わせます。独自の同期処理を
  持つコンポーネントは `NetworkComponent` を継承し、`CreateSyncParameter<T>` / `HasStateAuthority()` /
  `NetworkAwake` / `NetworkedTick` を使います。
- **パケット**: `Packet_ByteBuffer`/`Packet_Codec`を基盤に、`Packet_Dispatch_PacketDispatcherBase`/
  `Packet_PacketDispatcherGroup` 経由でディスパッチ。エンジン組み込みのパケット種別は
  `AssignPlayerId`/`SpawnNetworkObject`/`SyncTransform`/`SyncAnimation`/`SyncParameter`/`Rpc`/
  `PlayerLeft`/`OwnershipSnapshot`（`NetworkSystem_Packet.h`）。spawn履歴・補間バッファ等の固有ロジックは
  種別ごとの専用Dispatcher（`AssignPlayerId`/`SpawnNetworkObject`/`SyncTransform`/`SyncAnimation`/
  `SyncParameter`/`Rpc`、`PlayerLeft`/`OwnershipSnapshot` は `Session`）が持ち、ゲーム側は
  `Custom_PacketType`（101以降）で `SpawnPlayerAvatar`/`SpawnEnemy` を追加しています。
  送受信されたパケットは `PacketLog` に記録され、エディタの `NetworkLoggerWindow` で確認できます。
- **所有者テーブルとクライアント離脱**: `NetworkObjectId`の上位バイトは「Spawnしたピア」でしかなく、現在の所有者(権威)は
  `NetworkObjectInstanceRegistry`の所有者テーブル(`OwnerOf`/`SetOwner`)で管理します。権威判定は`NetworkRunnerBase::IsLocallyOwned`
  に一本化されています。ホストはクライアントの切断を検知すると`PlayerLeft{left, newOwner}`を全員へ配り(自分の受信キューにも積む)、
  全ピアが`SessionDispatcher`で同じ手順を実行します: 登録時の`OwnerLeavePolicy`が`Destroy`(プレイヤーアバター)なら破棄、
  `Transfer`(敵など)ならホストへ所有権を移譲。後入りには`OwnershipSnapshot`で「Spawnしたピア≠所有者」の一覧を送ります。
  ホスト自身の`PlayerId`は`0`(クライアントは1,2,…)。ホスト自体の離脱(ホストマイグレーション)は未対応です。
- **汎用RPC**: 「対象`NetworkObjectId`のコンポーネントに対してメソッドを1つ呼ぶ」形のものは
  `Engine/Module/Network/Rpc/Engine_Network_Rpc.h`の`Rpc<Args...>`/`RpcDef<RpcType, Args...>`
  経由で汎用化されています。単一の`DefaultPacketType::Rpc`パケットタイプ+`RpcHandlerRegistry`
  によるID経路のディスパッチで、新規RPC追加時にenum衝突調整・switch追加・専用Dispatcher
  クラス新設が不要です。受信側は `OnTargeted<Component>(handler, RpcOwnershipFilter)` で登録し、
  `None`（全員）/`SkipIfOwner`（所有者以外への通知）/`OnlyIfOwner`（所有者への要求）で呼び出し先を
  絞れます。RpcIdは、エンジン由来を `EEngineRpcType`（`Engine/Module/Network/Rpc/Engine_Network_RpcType.h`、0起点・現状は空）、ゲーム由来を
  `ERpcType`（`Assets/Scripts/Core/Network/Rpc/Custom_RpcType.h`、1,000,000以降）に分けて衝突を防ぎます。
  ゲーム側では `WakeUpPlayer`/`SyncAvatarState` に加え、敵BehaviourTreeの演出を全ピアへ伝える汎用演出RPC
  （`PlaySe`/`PlayBgm`/`SpawnPrefab`/`SpawnMovingPrefab`/`PurposeCamera`/`ScenePurposeCamera`/`Chat`/
  `ChangeMainScene`/`ShakeCamera`）と敵固有の `AttackAreaFire`/`EnemyDeath` が実装済みです
  （実装は `Assets/Scripts/Core/Network/Rpc/Module/`）。
- **敵AIの権威ゲート**: `NetworkBehaviourTree` コンポーネントが付き有効な `NetworkObjectId` を持つ敵は、
  BehaviourTreeのTickを所有者(権威)ピアだけで行い（`NetworkComponent::HasStateAuthority()` =
  `IsLocallyOwned`）、他ピアへは上記の演出RPCや同期コンポーネントで結果を反映します。

## ビルド方法

**（AIエージェント向け）ユーザーから明示的に指示されない限り、自発的にビルドを実行しないこと。**

```
MSBuild.exe NanamiEngine.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m:12
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

シーン/Prefab、BehaviourTree、AnimationTree、Effekseerエフェクトはcereal JSON / XMLを直接手編集せず、
以下のPythonツールキットから編集します。`.fbx`→`.mv1` のモデル変換も `tools/model` から行います。
これらのツールは `tools/common/` の基盤（cereal JSONコーデック、`.meta` コーデック、vcxproj編集）を
共有しています。各サブコマンドの詳細は `python -m tools.<name> <command> --help` で確認できます。

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
python -m tools.scene import-sprite SOURCE.png [--dest Assets/Art/UI] [--force]  # .pngをAssetsへコピーしSpriteFileの.meta発行
python -m tools.scene regen-catalog [--check]
python tools/scene/selftest.py
```

すべての変更系コマンドは `--dry-run` に対応。既知のv1制限: 対象ファイルは常に「純粋な木構造」の
前提（共有参照は表現できない）、カタログは `ENGINE_REGISTER_COMPONENT` で登録されたコンポーネント
（約85種）が対象、設定可能なパラメータ型は `int|float|bool|string|vec2|vec3|field` に限定、
独自フィールドを持つ中間C++基底を持つコンポーネント（Collider全般、`NetworkComponent` 派生、
`EnemyBase` 派生などのゲームスクリプト）は `add-component` で新規追加不可（既存のGameObject/Prefabを
コピーするか `instantiate-prefab` を使う）、`add-component-type` のような雛形生成コマンドはまだない。
詳細は **`tools/scene/README.md`** 参照。

### `tools/bt`（Enemy / FriendlyNpc の BehaviourTree・Action）

```
python -m tools.bt new-tree NAME [--npc-kind enemy|friendly]   # デフォルトは enemy
python -m tools.bt show|validate FILE                          # Enemy/Friendly はファイルから自動判別
python -m tools.bt add-node|copy-node|remove-node|move-node FILE ...   # デフォルトで自動レイアウト（--no-layoutで無効化）
python -m tools.bt layout FILE [--dx] [--dy]
python -m tools.bt set-params|set-weight FILE ...
python -m tools.bt add-bb-param|remove-bb-param FILE ...
python -m tools.bt apply FILE OPS.json
python -m tools.bt add-action --name X --category "<Cat>" [--npc-kind friendly] [--param n:type=default ...]
python -m tools.bt remove-action ...
python -m tools.bt regen-catalog [--check] [--npc-kind friendly]
python tools/bt/selftest.py
```

`add-action` はC++のAction雛形を生成し、`.vcxproj`/`.vcxproj.filters` とエディタのヘッダ登録まで
配線します（`remove-action` で元に戻せる）。Actionを追加・改名したら `regen-catalog` を実行し、
`tools/bt/catalog.json`（Enemy）/ `catalog_friendly.json`（FriendlyNpc）をコミットします。
既知のv1制限: 純粋な木構造前提、ブラックボードは `AnimationParameter<int>` のみ、
`nested:*`/`unknown` 形状のパラメータはCLIから設定不可。詳細は **`docs/BehaviourTree.md`** 参照。

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
python -m tools.effect add-node --kind sprite|ring|ribbon|model|track|group FILE ...
python -m tools.effect set-params FILE ...
python -m tools.effect apply FILE OPS.json
python -m tools.effect compile FILE           # .efkproj -> .efkefc（Effekseer 1.7.3.0 CUI、環境固有）
python -m tools.effect install EFKEFC --dest Assets/Art/Effect/<Sub>/<Name>.efkefc [--project]
python -m tools.effect check-env              # effect_config.json の設定内容とCUIの有無を表示
python -m tools.effect export --out DIR       # 公開リポジトリ用に配布ファイルだけをコピー
python tools/effect/selftest.py
```

Effekseer CUI のパスやインストール先など環境・プロジェクトごとに変わる値は `tools/effect/effect_config.json`
で設定します。このツールは単体でも [EffekseerEfkprojTool](https://github.com/ramasuke/EffekseerEfkprojTool)
として公開しており（`export` で同期）、導入手順は **`tools/effect/SETUP.md`** にあります。

モデル化されているのは `Sprite`/`Ring`/`Ribbon`/`Model`/`Track` の各ノード種別と、`SoundValues`/
`LocationAbsValues`（重力・引力）ブロック。FCurve（キーフレーム）系やプロジェクトルートのカメラ/ビューア
メタデータは未対応です。`add-node` の専用フラグにない項目は `--set dotted.path=value` で設定できます。
`install` は `tools/common/meta_base.py` 経由で新規guidの `.efkefc.meta`（`ParticleFile`）を発行し
（既存の `.meta` があればguidを維持）、`--project` 指定時は `.efkproj` ソースを
`Assets/Art/Effect/_Source/` にコミットします。詳細は **`tools/effect/README.md`** 参照。

### `tools/model`（DxLib ModelViewer / MV1変換）

```
python -m tools.model convert IN.fbx OUT.mv1 --mode mesh|anim|full [--with-textures] [--modelviewer-path <DxLibModelViewer_64bit.exeのパス>]
python -m tools.model install OUT.mv1 --dest Assets/Art/.../<Name>.mv1 [--with-textures] [--source IN.fbx] [--textures DIR]
python tools/model/selftest.py
```

DxLibModelViewerには公式のCLI/CUIが存在しないため、`convert` は`pywinauto`で実際の
GUIを操作します（`pip install pywinauto` が必要）。`--mode` で保存方法を選びます: `mesh`＝モデルのみ（名前を付けてメッシュのみ保存）、
`anim`＝アニメーションのみ（名前を付けてアニメーションのみ保存）、`full`＝モデル＋アニメーション
（名前を付けて保存）。2026-09-13にDxLibModelViewer ver3.24dで3モードとも実機検証済み。
`--with-textures`（`mesh`/`full`のみ）は、出力`.mv1`を解凍して参照テクスチャの相対パス
（例: `Hyena.fbm\Hyenas_A4_Diffuse.png`）を読み取り、入力`.fbx`の隣や`*.fbm`フォルダから
探して`.mv1`の隣の同じ相対パスへコピーします（見つからないものがあれば一覧を出してexit 1）。
`install` は`tools/common/meta_base.py` 経由で新規guidの`.mv1.meta`（`Mv1File`）を発行し、
`--with-textures` 指定時は同様に参照テクスチャを`<出力先ディレクトリ>`へ、`--source` 指定時は
`.fbx` を `<出力先ディレクトリ>/_Source/` に、`--textures` 指定時は画像ファイルを
`<出力先ディレクトリ>/textures/` に無条件で一括コピーします（`.mv1`は`.efkefc`と違い単一
ルートを持たないため）。既知の制約は **`tools/model/README.md`** 参照。

### `tools/art`（UI素材の生成スクリプト）

```
python tools/art/boss_health_gauge.py [--out-dir Assets/Art/UI/BossHealth] [--preview PATH]
python tools/art/npc_chat_icons.py [--out-dir Assets/Art/UI] [--rim-sweep]
```

ボス体力ゲージ（`BossHealthGaugeUI` プレハブのクリスタルクラウン型のクレスト/シャード）と、NPC頭上アイコン
（`SurpriseMark`/`ChattableIcon`/`ChatIcon`、`--rim-sweep` で光が縁を走るスプライトシート）の `.png` を
SDFベースで生成し、`SpriteFile` の `.png.meta` も出力します（既存の `.meta` はguidを維持するので
再生成しても参照は壊れません）。`Pillow` と `numpy` が必要です。

### `tools/common`（各ツール共通基盤）

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

- **`docs/BehaviourTree.md`** — Enemy / FriendlyNpc 2系統それぞれのデータの置き場所
  （`.enemyBehaviourData` / `.friendBehaviourData`、実行時クラス `Enemy_BehaviourTree` /
  `Friendly_BehaviourTree`、Action基底 `Enemy_Behaviour_ActionBase` / `Friendly_Behaviour_ActionBase`、
  エディタ側集約ヘッダ `*_Behaviour_ActionHeaders.h`）、`tools/bt` のコマンドリファレンス、
  BehaviourTreeファイルの完全なフォーマット仕様
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
- **`tools/{scene,bt,animtree,effect,model}/README.md`** — 各ツールキットのコマンド詳細と既知の制限。
- **`CLAUDE.md`** — AIエージェント（Claude Code）向けのプロジェクト規約とツールの使い分け。

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
（`<coroutine>`/`<ranges>` を含む）です。DxLib / EffekseerForDXLib のヘッダとライブラリは
`Libs/プロジェクトに追加すべきファイル_VC用/` に同梱されています。

**Python ツール**: `tools/scene`・`tools/bt`・`tools/animtree`・`tools/effect` は標準ライブラリのみで
動作します。`tools/model convert` は `pywinauto`、`tools/art` は `Pillow`/`numpy` が必要です。
`tools/effect compile` と `tools/model convert` はローカルにインストールされた Effekseer 1.7.3.0 /
DxLibModelViewer（リポジトリ非同梱）を使います（Effekseer のパスは `tools/effect/effect_config.json` で設定）。

## 開発状況

最近の主な開発内容:

- **ネットワーク**: 所有者テーブルによる権威判定の一本化、クライアント離脱時の破棄/所有権移譲、
  敵BehaviourTreeの演出処理を汎用RPCへ置き換え、権威側のみでBehaviourTreeをTickする権威ゲート
  （ホストマイグレーションは未対応）。
- **ゲームプレイ**: プレイヤーのロックオン、ダウン・起き上がり(蘇生)システム、Tyrannosaurus などの新規敵、
  LatticeBarrier / 雲などのプロップ、ドラゴン撃破演出。
- **UI**: `LayoutGroup` の追加とステージ選択UIの刷新（MVP分割・マップマーカー）、ボス共通の体力ゲージ、
  被ダメージフラッシュ・低HP時の画面エフェクト、ステージ選択からワールド進入時のフェード。
- **エディタ**: 実行中の AnimationTree / BehaviourTree の可視化ウィンドウ、Console / NetworkLogger
  ウィンドウ、例外処理の統一とクラッシュ回復、Projectウィンドウからのファイル名変更。
- **AI向けツール**: `tools/` による BehaviourTree（Enemy/FriendlyNpc）・AnimationTree・シーン/Prefab・
  Effekseerエフェクト（Model/Trackノード対応）の編集と、`tools/model` による `.fbx`→`.mv1` 変換。

詳細は `git log` を参照してください。

## ライセンス

現時点でこのリポジトリにライセンスファイルは存在しません。
