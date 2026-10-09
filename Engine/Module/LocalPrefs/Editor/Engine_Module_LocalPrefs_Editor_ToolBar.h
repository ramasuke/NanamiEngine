#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>
#include "../Engine_Module_LocalPrefs.h"

#include "../LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::LocalPrefs::Editor
{
    // NOTE: 型ごとの ImGui ウィジェット描画。未対応の型は型名を表示するだけ
    template<typename T>
    void DrawLocalPrefWidget(const std::string& label, T& value)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            ImGui::Checkbox(label.c_str(), &value);
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            ImGui::InputInt(label.c_str(), &value);
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            ImGui::InputFloat(label.c_str(), &value);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            ImGui::InputDouble(label.c_str(), &value);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            char buf[1024] = {};
            strncpy_s(buf, value.c_str(), sizeof(buf) - 1);
            if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
                value = buf;
        }
        else if constexpr (requires { value.OnDrawGui(); })
        {
            if (ImGui::TreeNode(label.c_str()))
            {
                value.OnDrawGui();
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TextDisabled("[%s: GUI未対応]", label.c_str());
        }
    }
    
    class NANAMI_API LocalPrefsRegistry final
    {
    public:
        // NOTE: 登録された設定項目 1 つ分のメタデータ
        struct NANAMI_API PrefInfo final
        {
            std::string key;
            std::string typeName;
            std::string subPath;
            
            // NOTE: 型を知らなくても、レジストリ側から共通で叩ける操作
            std::function<void()> saveDefault;
            // NOTE: 編集・保存 UI を描画する。値は初回だけファイルから読み、以降は内部 state を使う
            std::function<void()> drawEditGui;
            // NOTE: 登録元のモジュール
            Core::ModuleHandle module;
        };

        static LocalPrefsRegistry& GetInstance();
        void Register(PrefInfo info);
        // NOTE: module が登録した項目を消す。戻り値は消した数
        std::size_t UnregisterModule(Core::ModuleHandle module);
        [[nodiscard]] const std::vector<PrefInfo>& GetPrefsList() const;

    private:
        LocalPrefsRegistry() = default;
        std::vector<PrefInfo> m_prefsList;
    };

    // NOTE: 登録マクロの本体。makeDefault はデフォルト値が必要になったときに呼ぶ
    template<typename T, typename MakeDefault>
    bool RegisterLocalPref(std::string key, std::string subPath, std::string typeName,
                           MakeDefault makeDefault, Core::ModuleHandle module)
    {
        LocalPrefsRegistry::PrefInfo info;
        info.key      = key;
        info.typeName = std::move(typeName);
        info.subPath  = subPath;
        info.module   = module;
        info.saveDefault = [key, subPath, makeDefault]()
        {
            SaveWithPath<T>(subPath, key, makeDefault());
        };
        // NOTE: std::function はコピー可能なラムダを要るので、ムーブ専用かもしれない T は shared_ptr 越しに持つ
        info.drawEditGui = [key, subPath, makeDefault, state = std::make_shared<std::optional<T>>()]()
        {
            if (!state->has_value())
                *state = LoadOrDefaultWithPath<T>(subPath, key, makeDefault());
            T& value = state->value();

            ImGui::PushID(key.c_str());
            DrawLocalPrefWidget(key, value);
            ImGui::Spacing();
            if (ImGui::SmallButton("Save"))
                SaveWithPath<T>(subPath, key, value);
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset"))
            {
                value = makeDefault();
                SaveWithPath<T>(subPath, key, value);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Reload"))
                state->reset();
            ImGui::PopID();
        };
        LocalPrefsRegistry::GetInstance().Register(std::move(info));
        return true;
    }
}

#define NANAMI_LOCAL_PREF_CONCAT_(a, b) a##b
#define NANAMI_LOCAL_PREF_CONCAT(a, b)  NANAMI_LOCAL_PREF_CONCAT_(a, b)

// NOTE: LocalPrefs の項目をエディタのツールバーへ静的登録する
// NOTE: .cpp のグローバル / namespace スコープに書く (末尾の ; は不要)
#define REGISTER_LOCAL_PREF_WITH_PATH(Type, KeyName, DefaultValue, SubPath)                    \
    static const bool NANAMI_LOCAL_PREF_CONCAT(nanamiLocalPrefRegistered_, __COUNTER__) =      \
        ::NanamiEngine::Module::LocalPrefs::Editor::RegisterLocalPref<Type>(                   \
            KeyName, SubPath, #Type, []() -> Type { return DefaultValue; }, NANAMI_CURRENT_MODULE());

#define REGISTER_LOCAL_PREF(Type, KeyName, DefaultValue) \
    REGISTER_LOCAL_PREF_WITH_PATH(Type, KeyName, DefaultValue, "")
