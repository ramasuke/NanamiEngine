#pragma once
#include <stdexcept>
#include <string>

#include "cereal/archives/json.hpp"
#include "cereal/external/rapidjson/stringbuffer.h"
#include "cereal/external/rapidjson/writer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm.hpp"
#include "gtc/quaternion.hpp"

namespace NanamiEngine::Core::Application::AutoMcp
{
    using JsonValue     = rapidjson::Value;
    using JsonDocument  = rapidjson::Document;
    using JsonAllocator = rapidjson::Document::AllocatorType;

    /** @brief クライアントへ ok:false として返すエラー */
    class AutoMcpError final : public std::runtime_error
    {
    public:
        explicit AutoMcpError(const std::string& message) : std::runtime_error(message) {}
    };

    /** @brief UTF-8 として不正なら ACP (CP932) とみなして UTF-8 に直す。文字列リテラル由来のログ等が ACP のため */
    [[nodiscard]] std::string ToUtf8(const std::string& text);
    /** @brief UTF-8 のパスを MultiByte ビルドの std::ifstream 等が受け取れる ACP 文字列にする */
    [[nodiscard]] std::string Utf8PathToNative(const std::string& utf8Path);
    [[nodiscard]] std::string ToJsonText(const JsonValue& value);
    /** @brief typeid 名から "class " と名前空間を落とした短い型名 */
    [[nodiscard]] std::string ShortTypeName(const char* typeidName);
    [[nodiscard]] std::string FullTypeName(const char* typeidName);

    [[nodiscard]] JsonValue MakeString(const std::string& text, JsonAllocator& allocator);
    [[nodiscard]] JsonValue MakeVec3(const glm::vec3& value, JsonAllocator& allocator);
    [[nodiscard]] JsonValue MakeQuat(const glm::quat& value, JsonAllocator& allocator);

    [[nodiscard]] const JsonValue* FindMember(const JsonValue& object, const char* name);
    [[nodiscard]] std::string RequireString(const JsonValue& args, const char* name);
    [[nodiscard]] std::string OptionalString(const JsonValue& args, const char* name, const std::string& fallback);
    [[nodiscard]] bool        RequireBool(const JsonValue& args, const char* name);
    [[nodiscard]] bool        OptionalBool(const JsonValue& args, const char* name, bool fallback);
    [[nodiscard]] double      RequireNumber(const JsonValue& args, const char* name);
    [[nodiscard]] int         OptionalInt(const JsonValue& args, const char* name, int fallback);
    [[nodiscard]] bool        TryGetVec3(const JsonValue& args, const char* name, glm::vec3& out);
    /** @brief [x, y, z, w] の順で受け取る */
    [[nodiscard]] bool        TryGetQuat(const JsonValue& args, const char* name, glm::quat& out);
}
