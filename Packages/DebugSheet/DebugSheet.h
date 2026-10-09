#pragma once
// NOTE: DebugSheet を使う側はこれだけ include する。使い方は README.md
#include "DebugSheetConfig.h"
#include "Core/DebugSheet.h"
#include "Core/DebugSheetStyle.h"
#include "Core/DebugSheetWidgets.h"

// NOTE: ページを登録する。.cpp の最後 (グローバルスコープ) に書く
// NOTE: ID は翻訳単位内で一意な識別子、PATH は "カテゴリ/ページ名"、ORDER は小さい方が上
#if NANAMI_DEBUG_SHEET_ENABLED
#define REGISTER_DEBUG_SHEET_PAGE(ID, PATH, ORDER, DRAW)                                  \
    namespace {                                                                           \
        struct DebugSheetPageRegistrar_##ID {                                             \
            DebugSheetPageRegistrar_##ID() {                                              \
                ::NanamiEngine::DebugSheet::Sheet::Instance().RegisterPage(PATH, DRAW, ORDER, NANAMI_CURRENT_MODULE()); \
            }                                                                             \
        };                                                                                \
        const DebugSheetPageRegistrar_##ID debugSheetPageRegistrar_##ID;                  \
    }
#else
#define REGISTER_DEBUG_SHEET_PAGE(ID, PATH, ORDER, DRAW)
#endif
