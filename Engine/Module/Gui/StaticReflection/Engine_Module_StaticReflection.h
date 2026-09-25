#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <algorithm>
#include <functional>
#include <map>
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

    /** DrawCategoryMenu の 1 項目。category は "A::B" で入れ子になり、空ならトップレベルに並ぶ */
    struct NANAMI_API CategoryMenuItem
    {
        std::string           category;
        std::string           label;
        bool                  enabled;
        std::function<void()> onSelect;
    };

    struct NANAMI_NO_API CategoryMenuNode
    {
        std::map<std::string, std::unique_ptr<CategoryMenuNode>> children;
        std::vector<const CategoryMenuItem*>                     items;
    };

    inline void DrawCategoryMenuNode(const CategoryMenuNode& node)
    {
        for (const auto& [name, child] : node.children)
        {
            if (ImGui::BeginMenu(name.c_str()))
            {
                DrawCategoryMenuNode(*child);
                ImGui::EndMenu();
            }
        }
        for (const auto* item : node.items)
        {
            if (ImGui::MenuItem(item->label.c_str(), nullptr, false, item->enabled) && item->onSelect)
                item->onSelect();
        }
    }

    /** DrawTreeGui と同じ入れ子メニューを、呼び出し元のコールバックで描く。カテゴリも項目も名前順に並べる */
    inline void DrawCategoryMenu(const std::vector<CategoryMenuItem>& items)
    {
        std::vector<const CategoryMenuItem*> sortedItems;
        sortedItems.reserve(items.size());
        for (const auto& item : items)
            sortedItems.push_back(&item);
        std::ranges::sort(sortedItems, [](const CategoryMenuItem* lhs, const CategoryMenuItem* rhs) { return lhs->label < rhs->label; });

        CategoryMenuNode root;
        for (const auto* item : sortedItems)
        {
            CategoryMenuNode* current = &root;
            if (!item->category.empty())
            {
                for (const auto& token : Split(item->category, "::"))
                {
                    auto& child = current->children[token];
                    if (!child)
                        child = std::make_unique<CategoryMenuNode>();
                    current = child.get();
                }
            }
            current->items.push_back(item);
        }

        DrawCategoryMenuNode(root);
    }
}
