#!/usr/bin/env bash
# HotReload PoC を Linux (g++ + dlopen) で動かして、機構だけを確かめる。本番は Windows の HotReloadPoc.sln。
#   bash tools/hotreload_poc/emulate_linux.sh [cycles]
# Engine.so / Game.so / Host に分け、Game.so を RTLD_DEEPBIND で読む (Windows と同じく DLL 内の参照は自分の
# シンボルを優先 = cereal の StaticObject がモジュールごとになる)。Windows.h は linux_stub/ のスタブ。
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
POC="$ROOT/tools/hotreload_poc"
OUT="${POC_OUT:-$POC/linux_out}"
mkdir -p "$OUT"

CXX="${CXX:-g++}"
# -fno-gnu-unique: GCC はテンプレートの static を STB_GNU_UNIQUE にし、その .so を dlclose で外さなくなる (Windows には無い挙動)
FLAGS=(-std=c++20 -O0 -g -fPIC -fno-gnu-unique -Wall -Wno-dangling-reference
       -I"$ROOT" -I"$ROOT/Libs/cereal" -I"$ROOT/Libs/cereal/include" -I"$ROOT/Libs/cereal/include/cereal" -I"$POC/linux_stub"
       -DCEREAL_NANAMI_SHARED_STATIC_OBJECT -DCEREAL_NANAMI_SHARED_STATIC_OBJECT_API=)
SER="$ROOT/Engine/Module/Serialization"

echo "== build Engine.so"
"$CXX" "${FLAGS[@]}" -DNANAMI_ENGINE_BUILD_DLL -shared -o "$OUT/libHotReloadPocEngine.so" \
    "$ROOT/Engine/Core/Api/NanamiModule.cpp" \
    "$SER/Engine_Module_SerializationTypeRegistry.cpp" "$SER/Engine_Module_SharedStaticObject.cpp" \
    "$SER/Engine_Module_SerializationModuleUnloader.cpp" "$POC/Engine/PocEngine.cpp" -ldl
echo "== build Game.so"
"$CXX" "${FLAGS[@]}" -DNANAMI_ENGINE_USE_DLL -shared -o "$OUT/HotReloadPocGame.so" "$POC/Game/PocGame.cpp" \
    -L"$OUT" -lHotReloadPocEngine -ldl
echo "== build Host"
"$CXX" "${FLAGS[@]}" -DNANAMI_ENGINE_USE_DLL -o "$OUT/HotReloadPocHost" "$POC/Host/main.cpp" \
    -L"$OUT" -lHotReloadPocEngine -ldl -Wl,-rpath,"$OUT"
echo "== run"
cd "$OUT" && ./HotReloadPocHost ./HotReloadPocGame.so "${1:-10}"
