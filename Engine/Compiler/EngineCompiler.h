#pragma once

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <iostream>
#include <fstream>
#include <set>

#include "../../Libs/LibCore/FilePathHelper/FilePathHelper.h"

namespace NanamiEngine::Compiler
{
    class EngineCompiler final
    {
    public:
        struct Member
        {
            std::string type_;
            std::string name_;
            int version_;
        };

        struct Inheritance
        {
            std::string base_;
            std::string derived_;
        };

        /// 指定ディレクトリ以下の *.h ファイルをすべて処理
        static void ProcessHeaderFiles(const std::string& directoryPath, const bool forceRewrite = false)
        {
            const auto headers = LibCore::FilePath::SortFileExtension(directoryPath, ".h", true);

            if (headers.empty())
            {
                std::cerr << "Warning: No header files found in " << directoryPath << "\n";
                return;
            }

            for (const auto& header : headers)
            {
                std::cout << "Processing: " << header << "\n";
                if (!InjectSerializeToFile(header.string(), forceRewrite))
                {
                    std::cerr << "Failed to process " << header << "\n";
                }
            }
        }

        [[nodiscard]] static std::vector<Member> ExtractSerializedMembers(const std::string& filepath)
        {
            std::ifstream file(filepath);
            if (!file)
            {
                std::cerr << "Error: Failed to open file: " << filepath << "\n";
                return {};
            }

            const std::string code((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());

            std::vector<Member> members;

            // [[serialize(N)]] のみを許可
            const std::regex pattern(
                R"(\[\[serialize\((\d+)\)\]\]\s+([\w:<>]+|\w+\(.*\))\s+(\w+)\s*(?:=\s*[^;]*)?;)"
            );

            std::smatch match;
            auto searchStart = code.cbegin();

            while (std::regex_search(searchStart, code.cend(), match, pattern))
            {
                int version = std::stoi(match[1]);

                std::string type = match[2];
                // FIELD(...) を Field<...> に変換
                if (type.starts_with("FIELD("))
                {
                    type = std::regex_replace(type, std::regex(R"(FIELD\((.*)\))"), "Field<$1>");
                }

                members.push_back({type, match[3], version});
                searchStart = match.suffix().first;
            }

            return members;
        }

        [[nodiscard]] static std::string GenerateSaveLoadFunctions(
            const std::vector<Member>& members,
            const std::vector<Inheritance>& inheritance,
            const std::string& className)
        {
            std::ostringstream oss;

            // OnDrawGui() の生成
            oss << "void OnDrawGui() {\n";
            for (const auto& m : members)
            {
                oss << "    LibCore::ImGuiHelper::OnDrawInputField(\"" << m.name_ << "\", " << m.name_ << ");\n";
            }
            oss << "}\n\n";

            // save()
            oss << "template<class Archive>\n";
            oss << "void save(Archive& archive, const std::uint32_t version) const {\n";
            for (const auto& r : inheritance)
            {
                if (r.derived_ == className)
                    oss << "    archive(cereal::base_class<" << r.base_ << ">(this));\n";
            }
            for (const auto& m : members)
            {
                oss << "    archive(CEREAL_NVP(" << m.name_ << "));\n";
            }
            oss << "}\n\n";

            // load()
            oss << "template<class Archive>\n";
            oss << "void load(Archive& archive, const std::uint32_t version) {\n";
            for (const auto& r : inheritance)
            {
                if (r.derived_ == className)
                    oss << "    archive(cereal::base_class<" << r.base_ << ">(this));\n";
            }
            for (const auto& m : members)
            {
                oss << "    if (version >= " << m.version_ << ") archive(CEREAL_NVP(" << m.name_ << "));\n";
            }
            oss << "}\n";

            return oss.str();
        }

        [[nodiscard]] static bool InjectSerializeToFile(const std::string& inputPath, bool forceRewrite = false)
        {
            std::ifstream file(inputPath);
            if (!file)
            {
                std::cerr << "Error: Failed to open input file: " << inputPath << "\n";
                return false;
            }

            const std::string code((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());

            const auto members = ExtractSerializedMembers(inputPath);
            if (members.empty())
            {
                std::cerr << "Warning: No [[serialize(N)]] members found in " << inputPath << "\n";
                return false;
            }

            const auto inheritance = ExtractInheritanceRelations(code);

            std::string modified = code;
            if (forceRewrite)
            {
                modified = RemoveExistingSaveLoad(modified);
            }

            // クラス終端の位置を検索
            const std::regex classEnd(R"(\};)");
            std::smatch endMatch;
            if (!std::regex_search(modified, endMatch, classEnd))
            {
                std::cerr << "Error: Class definition end not found in " << inputPath << "\n";
                return false;
            }
            const size_t insertPos = endMatch.position(0);

            const std::string className = ExtractClassName(modified, insertPos);

            const std::string generated = GenerateSaveLoadFunctions(members, inheritance, className);

            std::ostringstream oss;
            oss << "\n#pragma region Serialization Function\n";
            oss << "public:\n";
            oss << generated;
            oss << "#pragma endregion\n";

            modified.insert(insertPos, oss.str());

            // classVersion と polymorphic を一括挿入
            if (!className.empty())
            {
                const int maxVersion = GetMaxVersion(members);
                InjectSerializationMacros(modified, className, maxVersion, inheritance);
            }

            std::ofstream outFile(inputPath);
            if (!outFile)
            {
                std::cerr << "Error: Failed to write to file: " << inputPath << "\n";
                return false;
            }

            outFile << modified;
            return true;
        }

        [[nodiscard]] static std::string ExtractClassName(const std::string& code, const std::size_t searchEnd = std::string::npos)
        {
            const std::string target = (searchEnd == std::string::npos) ? code : code.substr(0, searchEnd);
            const std::regex classPattern(R"((?:\bclass\b|\bstruct\b)\s+((?:\w+::)*\w+)[^{;]*\{)");
            std::smatch match;
            std::string lastMatch;

            auto searchStart = target.cbegin();
            while (std::regex_search(searchStart, target.cend(), match, classPattern))
            {
                lastMatch = match[1];
                searchStart = match.suffix().first;
            }

            return lastMatch;
        }

        [[nodiscard]] static int GetMaxVersion(const std::vector<Member>& members);

        static void InjectSerializationMacros(std::string& code,
                                      const std::string& className,
                                      int classVersion,
                                      const std::vector<Inheritance>& inheritance)
        {
            // すでに SerializationMacro region が存在する場合は何もしない
            if (code.find("#pragma region SerializationMacro") != std::string::npos)
            {
                std::cout << "SerializationMacro already exists, skipping macro generation.\n";
                return;
            }

            // マクロ生成
            std::ostringstream oss;
            oss << "CEREAL_CLASS_VERSION(" << className << ", " << classVersion << ");\n";

            std::set<std::string> emittedTypes;
            std::set<std::string> emittedRelations;

            for (const auto& r : inheritance)
            {
                if (r.derived_ != className) continue;

                if (emittedTypes.insert(r.derived_).second)
                    oss << "CEREAL_REGISTER_TYPE(" << r.derived_ << ");\n";

                const std::string relationKey = r.base_ + ":" + r.derived_;
                if (emittedRelations.insert(relationKey).second)
                    oss << "CEREAL_REGISTER_POLYMORPHIC_RELATION(" << r.base_ << ", " << r.derived_ << ");\n";
            }

            std::ostringstream region;
            region << "\n#pragma region SerializationMacro\n";
            region << oss.str();
            region << "#pragma endregion\n";

            code += region.str();
        }

        [[nodiscard]] static std::vector<Inheritance> ExtractInheritanceRelations(const std::string& code)
        {
            std::vector<Inheritance> relations;
            const std::regex classPattern(R"(class\s+((?:\w+::)*\w+)[^{]*:\s*((?:public\s+(?:\w+::)*\w+(?:\s*,\s*)?)+))");
            std::smatch match;

            auto searchStart = code.cbegin();
            while (std::regex_search(searchStart, code.cend(), match, classPattern))
            {
                const std::string derived = match[1];
                const std::string bases = match[2];

                const std::regex basePattern(R"(public\s+((?:\w+::)*\w+))");
                auto baseStart = bases.cbegin();
                std::smatch baseMatch;
                while (std::regex_search(baseStart, bases.cend(), baseMatch, basePattern))
                {
                    relations.push_back({baseMatch[1], derived});
                    baseStart = baseMatch.suffix().first;
                }

                searchStart = match.suffix().first;
            }

            return relations;
        }

        static std::string RemoveExistingSaveLoad(const std::string& code)
        {
            const std::regex regionBlock(R"(\s*#pragma\s+region\s+Serialization Function[\s\S]*?#pragma\s+endregion\s*)");
            std::string result = std::regex_replace(code, regionBlock, "");

            const std::regex serializePublic(R"(\n\s*public:\s*\n\s*template\s*<class\s+Archive>)");
            result = std::regex_replace(result, serializePublic, "\n\ntemplate<class Archive>");

            return result;
        }
    };
}
