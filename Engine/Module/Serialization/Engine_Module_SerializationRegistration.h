#pragma once
// cereal の型登録（CEREAL_REGISTER_TYPE など）を書く .cpp が include するヘッダー。
// cereal はその翻訳単位で見えている保存形式にだけ型を結びつけるので、
// エンジンが使う保存形式（JSON と PortableBinary）をここでまとめて見せる。
// 登録をヘッダーに書くと、include したすべての .cpp で保存・読み込みコードが生成されてビルドが遅くなる。
// CEREAL_CLASS_VERSION だけはヘッダーに残す（型を保存するすべての場所から見える必要がある）。
#include <../cereal/include/cereal/archives/json.hpp>
#include <../cereal/include/cereal/archives/portable_binary.hpp>
#include <../cereal/include/cereal/types/polymorphic.hpp>
