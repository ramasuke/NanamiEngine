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

// 多相の登録は CEREAL_REGISTER_TYPE / CEREAL_REGISTER_POLYMORPHIC_RELATION を直接書かず、下のマクロを使う。
// cereal への登録は同じで、加えて SerializationTypeRegistry にどのモジュールが登録したかを残す。
// TYPE の綴りがそのまま保存ファイルの型名になるので、置き換えるときも綴りを変えないこと。

/** 多相の型 TYPE を登録し、基底 BASE との関係を張る（CEREAL_REGISTER_TYPE + CEREAL_REGISTER_POLYMORPHIC_RELATION） */
#define NANAMI_REGISTER_TYPE(TYPE, BASE)                     \
    CEREAL_REGISTER_TYPE(TYPE)                               \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(BASE, TYPE)         \
    NANAMI_SERIALIZATION_RECORD_(TYPE, BASE, true)

/** 登録済みの型 DERIVED と、もう 1 つの基底 BASE との関係だけを張る（CEREAL_REGISTER_POLYMORPHIC_RELATION） */
#define NANAMI_REGISTER_POLYMORPHIC_RELATION(BASE, DERIVED)  \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(BASE, DERIVED)      \
    NANAMI_SERIALIZATION_RECORD_(DERIVED, BASE, false)

#define NANAMI_SERIALIZATION_CONCAT_INNER_(A, B) A##B
#define NANAMI_SERIALIZATION_CONCAT_(A, B) NANAMI_SERIALIZATION_CONCAT_INNER_(A, B)
#define NANAMI_SERIALIZATION_RECORD_(TYPE, BASE, IS_TYPE)                                              \
    namespace {                                                                                         \
        const bool NANAMI_SERIALIZATION_CONCAT_(nanamiSerializationTypeRecord_, __COUNTER__) =          \
            ::NanamiEngine::Module::Serialization::SerializationTypeRegistry::Record<TYPE, BASE, IS_TYPE>(); \
    }
