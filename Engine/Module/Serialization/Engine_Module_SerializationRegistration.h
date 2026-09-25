#pragma once
// cereal の型登録（NANAMI_REGISTER_TYPE など）を書く .cpp が include するヘッダー。
// cereal はその翻訳単位で見えている保存形式にだけ型を結びつけるので、
// エンジンが使う保存形式（JSON と PortableBinary）をここでまとめて見せる。
// 登録をヘッダーに書くと、include したすべての .cpp で保存・読み込みコードが生成されてビルドが遅くなる。
// CEREAL_CLASS_VERSION だけはヘッダーに残す（型を保存するすべての場所から見える必要がある）。
#include <../cereal/include/cereal/archives/json.hpp>
#include <../cereal/include/cereal/archives/portable_binary.hpp>
#include <../cereal/include/cereal/types/polymorphic.hpp>

#include "Engine_Module_SerializationTypeRegistry.h"

// 多相登録はこの 2 つのマクロで書く (cereal のマクロを直接呼ばない)。
// cereal への登録に加えて SerializationTypeRegistry に「型 / 基底 / polymorphic_name / 登録元モジュール」を記録し、
// ゲーム DLL のアンロード時にその分だけ cereal の表から消せるようにする (docs/HotReload.md §3.2)。
//
//   NANAMI_REGISTER_TYPE(T, Base)                  型の登録 + Base との関係。第 1 引数のトークン列がそのまま保存ファイルの
//                                                   polymorphic_name になるので、綴りは変えない
//   NANAMI_REGISTER_POLYMORPHIC_RELATION(Base, T)  2 つ目以降の基底 (IUpdatable など) や中間基底との関係だけ
//
// どちらもグローバルスコープに書く。行末の ; はあってもなくてもよい。
#define NANAMI_REGISTER_DETAIL_CONCAT_(a, b) a##b
#define NANAMI_REGISTER_DETAIL_CONCAT(a, b)  NANAMI_REGISTER_DETAIL_CONCAT_(a, b)
#define NANAMI_REGISTER_DETAIL_RECORD(T, Base, IsType)                                                   \
    namespace                                                                                            \
    {                                                                                                    \
        const bool NANAMI_REGISTER_DETAIL_CONCAT(nanamiSerializationTypeRecord_, __COUNTER__) =          \
            ::NanamiEngine::Module::Serialization::Detail::RecordPolymorphicRegistration<T, Base, IsType>(); \
    }

#define NANAMI_REGISTER_TYPE(T, Base)              \
    CEREAL_REGISTER_TYPE(T)                        \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)  \
    NANAMI_REGISTER_DETAIL_RECORD(T, Base, true)

#define NANAMI_REGISTER_POLYMORPHIC_RELATION(Base, T) \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)     \
    NANAMI_REGISTER_DETAIL_RECORD(T, Base, false)
