#pragma once
// NOTE: エンジン DLL の export / import 指定。静的 lib では空
#if defined(_MSC_VER) && defined(NANAMI_ENGINE_BUILD_DLL)
#define NANAMI_API __declspec(dllexport)
#elif defined(_MSC_VER) && defined(NANAMI_ENGINE_USE_DLL)
#define NANAMI_API __declspec(dllimport)
#else
#define NANAMI_API
#endif
// NOTE: export しないクラスの印 (常に空)
// NOTE: dllexport は暗黙のコピーまで実体化するので、unique_ptr のコンテナを持つ集成体に付ける
#define NANAMI_NO_API
