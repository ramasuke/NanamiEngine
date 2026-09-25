#pragma once
// エンジンを DLL にするときの export / import 指定 (docs/HotReload.md §6)。
//   NANAMI_ENGINE_BUILD_DLL : エンジン DLL 自身のビルド      -> dllexport
//   NANAMI_ENGINE_USE_DLL   : エンジン DLL を使う exe / dll -> dllimport
//   どちらも無し            : 今まで通りの静的 lib           -> 空
// 公開クラスには class NANAMI_API Foo と付ける。テンプレートには付けない (呼び出し側で実体化される)。
#if defined(_MSC_VER) && defined(NANAMI_ENGINE_BUILD_DLL)
#define NANAMI_API __declspec(dllexport)
#elif defined(_MSC_VER) && defined(NANAMI_ENGINE_USE_DLL)
#define NANAMI_API __declspec(dllimport)
#else
#define NANAMI_API
#endif
