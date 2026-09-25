#pragma once
// NOTE: エンジン DLL を使う側 (NANAMI_ENGINE_USE_DLL = ゲーム exe / dll) には DxLib と Effekseer を見せない。
//       自動リンク pragma を巻き込まず、ゲーム側に残った DxLib 呼び出しがリンクエラーになるようにするため (docs/HotReload.md §2)
#ifndef NANAMI_ENGINE_USE_DLL
#include "DxLib.h"
#include "EffekseerForDXLib.h"
#endif
#include "ImGuiHelper.h"
#include "rx.hpp"
#include "Jolt/Jolt.h"
#include "vec2.hpp"
#include "vec3.hpp"
#include "fwd.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <coroutine>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <queue>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
