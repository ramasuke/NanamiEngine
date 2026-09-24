#include "AutoMcpJson.h"

#include <filesystem>
#include <string_view>
#include <Windows.h>

namespace NanamiEngine::Core::Application::AutoMcp
{
    namespace
    {
        bool IsValidUtf8(const std::string& text)
        {
            if (text.empty())
                return true;

            return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0) > 0;
        }
    }

    std::string ToUtf8(const std::string& text)
    {
        if (IsValidUtf8(text))
            return text;

        const int wideSize = MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (wideSize <= 0)
            return {};

        std::wstring wide(wideSize, L'\0');
        MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), wide.data(), wideSize);

        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
        std::string utf8(utf8Size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, utf8.data(), utf8Size, nullptr, nullptr);
        return utf8;
    }

    std::string Utf8PathToNative(const std::string& utf8Path)
    {
        return std::filesystem::path(std::u8string(utf8Path.begin(), utf8Path.end())).string();
    }

    std::string ToJsonText(const JsonValue& value)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        value.Accept(writer);
        return std::string(buffer.GetString(), buffer.GetSize());
    }

    std::string FullTypeName(const char* typeidName)
    {
        std::string_view name = typeidName;
        for (const std::string_view prefix : {std::string_view("class "), std::string_view("struct ")})
        {
            if (name.starts_with(prefix))
            {
                name.remove_prefix(prefix.size());
                break;
            }
        }
        return std::string(name);
    }

    std::string ShortTypeName(const char* typeidName)
    {
        const std::string fullName = FullTypeName(typeidName);
        const size_t lastColon = fullName.rfind("::");
        return lastColon == std::string::npos ? fullName : fullName.substr(lastColon + 2);
    }

    JsonValue MakeString(const std::string& text, JsonAllocator& allocator)
    {
        const std::string utf8 = ToUtf8(text);
        JsonValue value;
        value.SetString(utf8.c_str(), static_cast<rapidjson::SizeType>(utf8.size()), allocator);
        return value;
    }

    JsonValue MakeVec3(const glm::vec3& value, JsonAllocator& allocator)
    {
        JsonValue array(rapidjson::kArrayType);
        array.PushBack(value.x, allocator);
        array.PushBack(value.y, allocator);
        array.PushBack(value.z, allocator);
        return array;
    }

    JsonValue MakeQuat(const glm::quat& value, JsonAllocator& allocator)
    {
        JsonValue array(rapidjson::kArrayType);
        array.PushBack(value.x, allocator);
        array.PushBack(value.y, allocator);
        array.PushBack(value.z, allocator);
        array.PushBack(value.w, allocator);
        return array;
    }

    const JsonValue* JsonArgs::FindMember(const char* name) const
    {
        if (!object_.IsObject())
            return nullptr;

        const auto it = object_.FindMember(name);
        if (it == object_.MemberEnd() || it->value.IsNull())
            return nullptr;

        return &it->value;
    }

    const JsonValue& JsonArgs::RequireMember(const char* name) const
    {
        const JsonValue* member = FindMember(name);
        if (member == nullptr)
            throw AutoMcpError(std::string("missing argument: ") + name);

        return *member;
    }

    std::string JsonArgs::RequireString(const char* name) const
    {
        const JsonValue& member = RequireMember(name);
        if (!member.IsString())
            throw AutoMcpError(std::string("argument must be a string: ") + name);

        return std::string(member.GetString(), member.GetStringLength());
    }

    std::string JsonArgs::OptionalString(const char* name, const std::string& fallback) const
    {
        return FindMember(name) ? RequireString(name) : fallback;
    }

    bool JsonArgs::RequireBool(const char* name) const
    {
        const JsonValue& member = RequireMember(name);
        if (!member.IsBool())
            throw AutoMcpError(std::string("argument must be a bool: ") + name);

        return member.GetBool();
    }

    bool JsonArgs::OptionalBool(const char* name, const bool fallback) const
    {
        return FindMember(name) ? RequireBool(name) : fallback;
    }

    double JsonArgs::RequireNumber(const char* name) const
    {
        const JsonValue& member = RequireMember(name);
        if (!member.IsNumber())
            throw AutoMcpError(std::string("argument must be a number: ") + name);

        return member.GetDouble();
    }

    int JsonArgs::OptionalInt(const char* name, const int fallback) const
    {
        return FindMember(name) ? static_cast<int>(RequireNumber(name)) : fallback;
    }

    bool JsonArgs::TryGetVec3(const char* name, glm::vec3& out) const
    {
        const JsonValue* member = FindMember(name);
        if (member == nullptr)
            return false;

        if (!member->IsArray() || member->Size() != 3 || !(*member)[0].IsNumber() || !(*member)[1].IsNumber() || !(*member)[2].IsNumber())
            throw AutoMcpError(std::string("argument must be [x, y, z]: ") + name);

        out = glm::vec3((*member)[0].GetFloat(), (*member)[1].GetFloat(), (*member)[2].GetFloat());
        return true;
    }

    bool JsonArgs::TryGetQuat(const char* name, glm::quat& out) const
    {
        const JsonValue* member = FindMember(name);
        if (member == nullptr)
            return false;

        if (!member->IsArray() || member->Size() != 4)
            throw AutoMcpError(std::string("argument must be [x, y, z, w]: ") + name);

        for (rapidjson::SizeType i = 0; i < 4; ++i)
        {
            if (!(*member)[i].IsNumber())
                throw AutoMcpError(std::string("argument must be [x, y, z, w]: ") + name);
        }

        out = glm::normalize(glm::quat((*member)[3].GetFloat(), (*member)[0].GetFloat(), (*member)[1].GetFloat(), (*member)[2].GetFloat()));
        return true;
    }
}
