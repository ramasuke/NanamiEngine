#pragma once
//NOTE: DebugSheet を使う側はこれだけ include する。使い方は README.md
#include "DebugSheetConfig.h"
#include "Core/DebugSheet.h"
#include "Core/DebugSheetStyle.h"
#include "Core/DebugSheetWidgets.h"

/**
 * ページを登録する。.cpp の最後 (グローバルスコープ) に書く
 * @param ID    翻訳単位内で一意な識別子
 * @param PATH  "カテゴリ/ページ名"。/ で何段でも掘れる
 * @param ORDER 同じ階層内の並び順 (小さい方が上)
 * @param DRAW  void() のページ描画関数
 */
#if NANAMI_DEBUG_SHEET_ENABLED
#define REGISTER_DEBUG_SHEET_PAGE(ID, PATH, ORDER, DRAW)                                  \
    namespace {                                                                           \
        struct DebugSheetPageRegistrar_##ID {                                             \
            DebugSheetPageRegistrar_##ID() {                                              \
                ::NanamiEngine::DebugSheet::Sheet::Instance().RegisterPage(PATH, DRAW, ORDER); \
            }                                                                             \
        };                                                                                \
        const DebugSheetPageRegistrar_##ID debugSheetPageRegistrar_##ID;                  \
    }
#else
#define REGISTER_DEBUG_SHEET_PAGE(ID, PATH, ORDER, DRAW)
#endif
