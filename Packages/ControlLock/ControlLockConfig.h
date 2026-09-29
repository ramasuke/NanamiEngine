#pragma once

// NOTE: ロックの取得元 (std::source_location) を記録するか。エディタと Debug 構成のゲームビルドだけで有効。
//       Release のゲームビルドでは引数ごと無くし、ソースのフルパスを exe に埋め込まない
#if !defined(NANAMI_GAME_BUILD) || defined(_DEBUG)
#define NANAMI_CONTROL_LOCK_TRACE_ENABLED 1
#else
#define NANAMI_CONTROL_LOCK_TRACE_ENABLED 0
#endif
