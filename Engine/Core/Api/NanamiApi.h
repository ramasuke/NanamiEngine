#pragma once
// エンジンを DLL にするときの export / import 指定 (docs/HotReload.md §6)。
//   NANAMI_ENGINE_BUILD_DLL : エンジン DLL 自身のビルド      -> dllexport
//   NANAMI_ENGINE_USE_DLL   : エンジン DLL を使う exe / dll -> dllimport
//   どちらも無し              : 静的 lib           -> 空
#if defined(_MSC_VER) && defined(NANAMI_ENGINE_BUILD_DLL)
#define NANAMI_API __declspec(dllexport)
#elif defined(_MSC_VER) && defined(NANAMI_ENGINE_USE_DLL)
#define NANAMI_API __declspec(dllimport)
#else
#define NANAMI_API
#endif
// export しないクラスの印 (常に空)。tools/engine_api/add_nanami_api.py はこれが付いた class / struct を飛ばす。
// dllexport は暗黙のコピー / デストラクタまで実体化するので、unique_ptr のコンテナを持つ集成体のような
// 「export すると壊れるが export する必要も無い」入れ子の構造体に付ける
#define NANAMI_NO_API
