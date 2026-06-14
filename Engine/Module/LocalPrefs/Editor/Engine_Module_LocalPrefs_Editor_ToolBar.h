#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../Engine_Module_LocalPrefs.h"

#include "../LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::LocalPrefs::Editor
{
    // 型ごとの ImGui ウィジェット描画。プリミティブ型は直接ウィジェットへ、
    // OnDrawGui() を持つ型はそれを呼ぶ。どちらでもない場合は型名を表示するだけ。
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
    
    class LocalPrefsRegistry final
    {
    public:
        // 列挙時にエディタ側が受け取る、各設定項目のメタデータ
        struct PrefInfo final
        {
            std::string key;
            std::string typeName;
            std::string subPath;
            
            // 型を知らなくても、レジストリ側から共通で叩ける操作
            std::function<void()> saveDefault;
            // ファイルから値をロードし、ImGui ウィジェットで編集・保存できるUIを描画する
            // 初回呼び出し時にファイルから値を読み込み、以降は内部 state を保持する
            std::function<void()> drawEditGui;
        };

        // シングルトンインスタンスの取得
        static LocalPrefsRegistry& GetInstance();
        // マクロの初期化ロジックから呼び出される登録関数
        void Register(PrefInfo info);
        // エディタ側で「登録された項目をループで列挙する」ためのゲッター
        [[nodiscard]] const std::vector<PrefInfo>& GetPrefsList() const;

    private:
        LocalPrefsRegistry() = default;
        std::vector<PrefInfo> m_prefsList;
    };

}



/**
 * 内部実装用マクロ（直接呼ばないでください）
 *
 * UniqueID を受け取るのは、同じ KeyName が複数の翻訳単位に現れたとき
 * 構造体名が衝突しないよう行番号をサフィックスに使うため。
 * inline static メンバにすることでヘッダインクルード時に静的初期化が走り、
 * main() より前にレジストリへ登録が完了する。
 *
 * drawEditGui ラムダは shared_ptr<optional<T>> で値を遅延ロードして保持する。
 * 初回描画時にファイルから読み込み、以降は in-memory で編集・保存できる。
 */
#define REGISTER_LOCAL_PREF_WITH_PATH_IMPL(Type, KeyName, DefaultValue, SubPath, UniqueID)  \
namespace NanamiEngine::Module::LocalPrefs::Internal {                                       \
struct AutoRegister_##UniqueID {                                                             \
    AutoRegister_##UniqueID() {                                                              \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::PrefInfo info;       \
        info.key      = KeyName;                                                             \
        info.typeName = #Type;                                                               \
        info.subPath  = SubPath;                                                             \
        info.saveDefault = []() {                                                            \
            ::NanamiEngine::Module::LocalPrefs::SaveWithPath<Type>(SubPath, KeyName, DefaultValue); \
        };                                                                                   \
        info.drawEditGui = [                                                                 \
            statePtr = std::make_shared<std::optional<Type>>(),                             \
            _key     = std::string{KeyName},                                                \
            _subPath = std::string{SubPath}                                                 \
        ]() mutable {                                                                       \
            if (!statePtr->has_value())                                                     \
                *statePtr = ::NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath<Type>(\
                    _subPath, _key, DefaultValue);                                          \
            Type& _val = statePtr->value();                                                 \
            ::ImGui::PushID(_key.c_str());                                                  \
            ::NanamiEngine::Module::LocalPrefs::Editor::DrawLocalPrefWidget(_key, _val);    \
            ::ImGui::Spacing();                                                             \
            if (::ImGui::SmallButton(("Save##" + _key).c_str()))                           \
                ::NanamiEngine::Module::LocalPrefs::SaveWithPath<Type>(_subPath, _key, _val); \
            ::ImGui::SameLine();                                                            \
            if (::ImGui::SmallButton(("Reset##" + _key).c_str())) {                        \
                _val = DefaultValue;                                                        \
                ::NanamiEngine::Module::LocalPrefs::SaveWithPath<Type>(_subPath, _key, _val); \
            }                                                                               \
            ::ImGui::SameLine();                                                            \
            if (::ImGui::SmallButton(("Reload##" + _key).c_str()))                         \
                statePtr->reset();                                                          \
            ::ImGui::PopID();                                                               \
        };                                                                                  \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().Register(std::move(info)); \
    }                                                                                       \
};                                                                                          \
inline static AutoRegister_##UniqueID global_autoregister_##UniqueID;                       \
}

#define REGISTER_LOCAL_PREF_IMPL(Type, KeyName, DefaultValue, UniqueID)                     \
namespace NanamiEngine::Module::LocalPrefs::Internal {                                       \
struct AutoRegister_##UniqueID {                                                             \
    AutoRegister_##UniqueID() {                                                              \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::PrefInfo info;       \
        info.key      = KeyName;                                                             \
        info.typeName = #Type;                                                               \
        info.subPath  = "";                                                                  \
        info.saveDefault = []() {                                                            \
            ::NanamiEngine::Module::LocalPrefs::Save<Type>(KeyName, DefaultValue);           \
        };                                                                                   \
        info.drawEditGui = [                                                                 \
            statePtr = std::make_shared<std::optional<Type>>(),                             \
            _key     = std::string{KeyName}                                                 \
        ]() mutable {                                                                       \
            if (!statePtr->has_value())                                                     \
                *statePtr = ::NanamiEngine::Module::LocalPrefs::LoadOrDefault<Type>(        \
                    _key, DefaultValue);                                                    \
            Type& _val = statePtr->value();                                                 \
            ::ImGui::PushID(_key.c_str());                                                  \
            ::NanamiEngine::Module::LocalPrefs::Editor::DrawLocalPrefWidget(_key, _val);    \
            ::ImGui::Spacing();                                                             \
            if (::ImGui::SmallButton(("Save##" + _key).c_str()))                           \
                ::NanamiEngine::Module::LocalPrefs::Save<Type>(_key, _val);                 \
            ::ImGui::SameLine();                                                            \
            if (::ImGui::SmallButton(("Reset##" + _key).c_str())) {                        \
                _val = DefaultValue;                                                        \
                ::NanamiEngine::Module::LocalPrefs::Save<Type>(_key, _val);                 \
            }                                                                               \
            ::ImGui::SameLine();                                                            \
            if (::ImGui::SmallButton(("Reload##" + _key).c_str()))                         \
                statePtr->reset();                                                          \
            ::ImGui::PopID();                                                               \
        };                                                                                  \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().Register(std::move(info)); \
    }                                                                                       \
};                                                                                          \
inline static AutoRegister_##UniqueID global_autoregister_##UniqueID;                       \
}


/**
 * --- ユーザーが実際に使用する静的登録用マクロ ---
 *
 * NOTE: マクロを2段階に分けている理由
 *   __LINE__ はマクロ展開時点の行番号に置換される。
 *   直接 _IMPL に __LINE__ を渡すと、_IMPL 内の ## 展開より先に __LINE__ が評価されず
 *   文字列 "__LINE__" がそのまま構造体名に入ってしまう。
 *   一段ラップして引数として渡すことで、__LINE__ を確実に数値に展開してから結合できる。
 */
#define REGISTER_LOCAL_PREF_WITH_PATH(Type, KeyName, DefaultValue, SubPath) \
    REGISTER_LOCAL_PREF_WITH_PATH_IMPL(Type, KeyName, DefaultValue, SubPath, KeyName##_##__LINE__)

#define REGISTER_LOCAL_PREF(Type, KeyName, DefaultValue) \
    REGISTER_LOCAL_PREF_IMPL(Type, KeyName, DefaultValue, KeyName##_##__LINE__)