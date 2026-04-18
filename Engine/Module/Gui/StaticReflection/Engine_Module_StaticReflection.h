#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ImGuiHelper.h"

namespace NanamiEngine::Module::StaticReflection
{
    /** ノード構造（汎用） */
    template <typename T>
    struct NodeTree
    {
        std::string name;
        std::unordered_map<std::string, std::unique_ptr<NodeTree>> children;

        // 生成関数
        std::function<std::unique_ptr<T>()> createFunc = nullptr;
    };

    /** 文字列分割 */
    inline std::vector<std::string> Split(const std::string& str, const std::string& delimiter)
    {
        std::vector<std::string> result;

        size_t start = 0;
        size_t end;

        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
        }

        result.push_back(str.substr(start));
        return result;
    }

    /**  ツリー構築 */
    template <typename T>
    NodeTree<T> BuildTree(
        const std::unordered_map<std::string, std::function<std::unique_ptr<T>()>>& elements)
    {
        NodeTree<T> root;
        root.name = "Root";

        for (const auto& [fullPath, func] : elements)
        {
            auto tokens = Split(fullPath, "::");

            NodeTree<T>* current = &root;

            for (const auto& token : tokens)
            {
                if (!current->children.contains(token))
                {
                    auto node = std::make_unique<NodeTree<T>>();
                    node->name = token;
                    current->children[token] = std::move(node);
                }

                current = current->children[token].get();
            }

            current->createFunc = func;
        }

        return root;
    }

    /** ImGui描画 */
    template <typename T>
    void DrawTreeGui(NodeTree<T>& node, std::unique_ptr<T>& outObject)
    {
        for (auto& [name, child] : node.children)
        {
            if (child->children.empty())
            {
                // leaf
                if (ImGui::MenuItem(name.c_str()))
                {
                    if (child->createFunc)
                    {
                        outObject = child->createFunc();
                    }
                }
            }
            else
            {
                if (ImGui::BeginMenu(name.c_str()))
                {
                    DrawTreeGui(*child, outObject);
                    ImGui::EndMenu();
                }
            }
        }
    }
}
