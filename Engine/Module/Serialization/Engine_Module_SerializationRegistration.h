#pragma once
// 型登録を書く .cpp が include する。cereal は見えている保存形式にだけ型を結びつけるので JSON と PortableBinary を見せる
// NOTE: 登録はヘッダーに書かない (CEREAL_CLASS_VERSION だけはヘッダーに残す)
#include <../cereal/include/cereal/archives/json.hpp>
#include <../cereal/include/cereal/archives/portable_binary.hpp>
#include <../cereal/include/cereal/types/polymorphic.hpp>

#include "Engine_Module_SerializationTypeRegistry.h"

// cereal への登録に加え、DLL アンロード時に消せるよう SerializationTypeRegistry に登録元モジュールを記録する
// WARNING: NANAMI_REGISTER_TYPE の第 1 引数はそのまま保存ファイルの polymorphic_name になるので綴りを変えない
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
