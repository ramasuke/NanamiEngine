#pragma once

// NOTE: エディタと Debug 構成のゲームビルドだけで有効。Release のゲームビルドには出さない
#if !defined(NANAMI_GAME_BUILD) || defined(_DEBUG)
#define NANAMI_DEBUG_SHEET_ENABLED 1
#else
#define NANAMI_DEBUG_SHEET_ENABLED 0
#endif
