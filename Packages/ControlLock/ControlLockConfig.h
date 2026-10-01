#pragma once

// NOTE: ロックの取得元を記録するか。Release のゲームビルドではソースのフルパスを exe に埋め込まないよう無効
#if !defined(NANAMI_GAME_BUILD) || defined(_DEBUG)
#define NANAMI_CONTROL_LOCK_TRACE_ENABLED 1
#else
#define NANAMI_CONTROL_LOCK_TRACE_ENABLED 0
#endif
