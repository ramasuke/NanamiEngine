# HotReload PoC (docs/HotReload.md §10 「PoC」)

「Host exe + Engine.dll + Game.dll」で、cereal の多相登録がモジュールを越えて 1 つの表に集まり、
Game.dll のアンロード時に **登録記録 (`SerializationTypeRegistry`) からその分だけ消せる**ことを確かめる最小構成。
実エンジンの `Engine/Module/Serialization/*` と `Libs/cereal` のパッチ (`static_object.hpp`) をそのまま使う。

| 部品 | 中身 |
|---|---|
| `Engine/` (HotReloadPocEngine.dll) | `Engine_Module_SerializationTypeRegistry.cpp` / `Engine_Module_SharedStaticObject.cpp` / `Engine_Module_SerializationModuleUnloader.cpp` + `Poc::Component` / `EngineComponent` / `IUpdatable` と JSON / PortableBinary の保存・復元 API |
| `Game/` (HotReloadPocGame.dll) | `GameComponent : EngineComponent, IUpdatable` (推移的な caster + 2 つ目の基底) と `GameOnly : Component` を `NANAMI_REGISTER_TYPE` で登録。`extern "C" PocGetGameApi()` だけを export |
| `Host/` (HotReloadPocHost.exe) | Game.dll を `HotReloadPocGame_<n>.dll` にコピーして `LoadLibrary` → 検証 → `Unregister` → `FreeLibrary` を N 回 (既定 10) |

## Windows (本番)

```
MSBuild.exe tools\hotreload_poc\HotReloadPoc.sln -p:Configuration=Debug -p:Platform=x64 -p:PreferredToolArchitecture=x64
tools\hotreload_poc\x64\Debug\HotReloadPocHost.exe            # 既定: HotReloadPocGame.dll を 10 回
tools\hotreload_poc\x64\Debug\HotReloadPocHost.exe HotReloadPocGame.dll 50
```

終了コード 0 = PASS。相対パスの DLL 名は exe のあるフォルダ基準なので、リポジトリルートから起動してよい。
2026-09-25: MSVC (v143) の Debug / Release とも 50 サイクル PASS。各サイクルで次を確認する (`[FAIL]` が 1 つでもあれば FAIL):

1. エンジン側のコード (`Engine::SaveJson` / `LoadJson` / `SaveBinary` / `LoadBinary`) でゲーム型を保存・復元できる (= Scene のロード)
2. ゲーム側のコード (`SaveFromGame` / `LoadFromGame`) でエンジン型を保存・復元できる (双方向に同じ表を見ている)
3. `dynamic_cast` が DLL 境界を越える (`IUpdatable` 経由の仮想関数、エンジン基底への cast)
4. `SerializationModuleUnloader::Unregister(module)` 後に、記録・caster (vtable が Game.dll のもの)・その DLL が作った `StaticObject` が残っていない
5. `FreeLibrary` 後、cereal の表の大きさが最初 (エンジンだけ) と同じに戻る。前サイクルのアンロード後とも同一
6. アンロード後もエンジン型の保存・復元が壊れていない

Debug ビルドは `_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF)` で終了時にリークを stderr に出力する。報告される 49 ブロック / 2,957 バイトは
サイクル数 1 / 10 / 50 で同一 = Game.dll の出し入れによるリークではなく、exe の CRT がダンプする時点でまだ生きている
Engine.dll 側の静的オブジェクト (共有スロットの表など。DLL の静的デストラクタは exe のダンプより後に走る)。
サイクル数を変えてブロック数が増えないことで判断する。

## Linux (機構だけの確認)

```
bash tools/hotreload_poc/emulate_linux.sh [cycles]
```

g++ で Engine.so / Game.so / Host を作り、Game.so を `dlopen(RTLD_DEEPBIND)` で読む (Windows と同じく DLL 内の参照は
自分のシンボルを優先し、cereal の `StaticObject` がモジュールごとになる)。`Windows.h` は `linux_stub/` のスタブで、
`GetModuleHandleExW(FROM_ADDRESS)` を `dladdr` で真似る。`-fno-gnu-unique` が必須 (GCC はテンプレートの static を
STB_GNU_UNIQUE にして `dlclose` で外さなくなる。Windows には無い挙動)。
出力は `linux_out/` (gitignore)。2026-09-25: 10 サイクル PASS、valgrind で不正アクセス・definite leak なし。

## 分かったこと (実装に効く注意)

- `nanami_shared_static_object()` は **ロックの外で** `create()` を呼ぶ。`OutputBindingCreator` のコンストラクタが
  `OutputBindingMap` を取りに再入してくるので、ロック中に作ると Windows の `std::mutex` ではデッドロックする。
- 記録の `module` は「登録子のアドレスが属するモジュール」。Host 側は `ModuleOf(GetProcAddress の結果)` で同じ値を得る
  (Windows では `LoadLibrary` の戻り値と一致する。Host が assert する)。
- 推移的な caster (`map[祖先][派生]`、`reverseMap[派生] = 祖先`) は派生をキーに全部消す。保険として vtable が
  Game.dll にある caster を含む項目も掃く (`sweptCasters`。今の PoC では 0 で、記録だけで足りている)。
