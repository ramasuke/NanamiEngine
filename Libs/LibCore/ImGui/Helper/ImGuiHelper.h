#pragma once
#include <type_traits>
#include <string>
#include "../../Libs/ImGui/ImGuiHelper.h"
#include <functional>
#include <span>

#include "../glm/vec3.hpp"


// ImVec2演算ヘルパー
inline ImVec2  operator+ (const ImVec2& lhs, const ImVec2& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y};  }
inline ImVec2  operator- (const ImVec2& lhs, const ImVec2& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y};  }
inline ImVec2& operator+=(      ImVec2& lhs, const ImVec2& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs;}
inline ImVec2& operator-=(      ImVec2& lhs, const ImVec2& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs;}
inline ImVec2  operator* (const ImVec2& lhs, const float   rhs) { return {lhs.x * rhs, lhs.y * rhs};      }
inline ImVec2  operator* (const float   lhs, const ImVec2& rhs) { return {lhs * rhs.x, lhs * rhs.y};      }
inline ImVec2  operator/ (const ImVec2& lhs, const float   rhs) { return {lhs.x / rhs, lhs.y / rhs};      }

namespace LibCore::ImGuiHelper
{
    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
        requires std::is_same_v<T, bool>
    {
        ::ImGui::Checkbox(label.c_str(), &value);
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
        requires (std::is_same_v<T, int>)
    {
        ::ImGui::InputInt(label.c_str(), &value);
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
        requires std::is_same_v<T, float>
    {
        ::ImGui::InputFloat(label.c_str(), &value);
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
        requires std::is_same_v<T, double>
    {
        ::ImGui::InputDouble(label.c_str(), &value);
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
        requires std::is_same_v<T, std::string>
    {
        char buffer[256];
        strncpy_s(buffer, value.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';
        if (::ImGui::InputText(label.c_str(), buffer, sizeof(buffer)))
        {
            value = buffer;
        }
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
        requires std::is_same_v<decltype(std::declval<T&>().OnDrawGui()), void>
    {
        if (::ImGui::TreeNode(label.c_str()))
        {
            value.OnDrawGui();
            ::ImGui::TreePop();
            ::ImGui::Spacing();
        }
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string&, T&)
        requires (!std::is_same_v<decltype(std::declval<T&>().OnDrawGui()), void>)
    {
        static_assert(sizeof(T) == 0, "Unsupported type for ImGuiHelper::OnDrawInputField");
    }

    template <typename PtrType>
    static void
    OnDrawInputField(const std::string& label, PtrType& ptr)
        requires std::is_pointer_v<typename PtrType::element_type*>
    {
        if (ptr)
        {
            OnDrawInputField(label, *ptr);
        }
        else
        {
            ::ImGui::Text("%s: nullptr", label.c_str());
        }
    }

    template <typename T>
    static void
    OnDrawTreeInspector(const std::string& label, T& value)
        requires std::is_same_v<decltype(std::declval<T&>().OnDrawGui()), void>
    {
        if (::ImGui::TreeNode(label.c_str()))
        {
            value.OnDrawGui();
            ::ImGui::TreePop();
            ::ImGui::Spacing();
        }
    }

    template <typename Container>
    static void
    OnDrawInputField(const std::string& label, Container& container, const std::function<void()>& drawAddButton)
        requires (!std::is_same_v<typename Container::value_type, void>)
    {
        if (::ImGui::TreeNode(label.c_str()))
        {
            int index = 0;
            for (auto& element : container)
            {
                std::string itemLabel = "Element " + std::to_string(index++);
                OnDrawInputField(itemLabel, element);
            }

            drawAddButton();

            static int deleteIndex = 0;
            ::ImGui::InputInt("Delete Index", &deleteIndex);
            if (::ImGui::Button("Delete") && deleteIndex >= 0 && deleteIndex < static_cast<int>(container.size()))
            {
                container.erase(container.begin() + deleteIndex);
                deleteIndex = 0;
            }

            ::ImGui::TreePop();
            ::ImGui::Spacing();
        }
    }

    template <typename T>
    static void
    OnDrawInputField(const std::string& label, T& value)
    requires std::is_same_v<T, glm::vec3>
    {
        float buffer[3] = { value.x, value.y, value.z };
        if (::ImGui::InputFloat3(label.c_str(), buffer))
        {
            value.x = buffer[0];
            value.y = buffer[1];
            value.z = buffer[2];
        }
    }

    template <typename Enum, size_t N>
    static void
    OnDrawEnumField(const std::string& label,
                    Enum& value,
                    const std::array<Enum, N>& items,
                    std::string_view (*toString)(Enum)) requires std::is_enum_v<Enum>
    {
        const char* current = toString(value).data();

        if (ImGui::BeginCombo(label.c_str(), current))
        {
            for (auto item : items)
            {
                bool isSelected = (item == value);
                if (ImGui::Selectable(toString(item).data(), isSelected))
                {
                    value = item;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
};
