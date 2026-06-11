#pragma once
#include <string>
#include <vector>
#include <functional>

#include "../Engine_Module_LocalPrefs.h"

namespace NanamiEngine::Module::LocalPrefs::Editor
{
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

/** 内部実装用マクロ（直接呼ばないでください） */
#define REGISTER_LOCAL_PREF_WITH_PATH_IMPL(Type, KeyName, DefaultValue, SubPath, UniqueID) \
namespace NanamiEngine::Module::LocalPrefs::Internal {                                  \
struct AutoRegister_##UniqueID {                                                        \
    AutoRegister_##UniqueID() {                                                         \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::PrefInfo info;  \
        info.key = KeyName;                                                            \
        info.typeName = #Type;                                                          \
        info.subPath = SubPath;                                                         \
        info.saveDefault = []() {                                                       \
            ::NanamiEngine::Module::LocalPrefs::SaveWithPath<Type>(SubPath, KeyName, DefaultValue); \
        };                                                                              \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().Register(std::move(info)); \
    }                                                                                   \
};                                                                                      \
inline static AutoRegister_##UniqueID global_autoregister_##UniqueID;                   \
}

#define REGISTER_LOCAL_PREF_IMPL(Type, KeyName, DefaultValue, UniqueID)                 \
namespace NanamiEngine::Module::LocalPrefs::Internal {                                  \
struct AutoRegister_##UniqueID {                                                        \
    AutoRegister_##UniqueID() {                                                         \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::PrefInfo info;  \
        info.key = KeyName;                                                            \
        info.typeName = #Type;                                                          \
        info.subPath = "";                                                              \
        info.saveDefault = []() {                                                       \
            ::NanamiEngine::Module::LocalPrefs::Save<Type>(KeyName, DefaultValue);      \
        };                                                                              \
        ::NanamiEngine::Module::LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().Register(std::move(info)); \
    }                                                                                   \
};                                                                                      \
inline static AutoRegister_##UniqueID global_autoregister_##UniqueID;                   \
}

/** --- ユーザーが実際に使用する静的登録用マクロ --- */
// トークン結合（##）のバッティングを防ぐため、__LINE__（行番号）を使って完全にユニークな構造体名を作ります
#define REGISTER_LOCAL_PREF_WITH_PATH(Type, KeyName, DefaultValue, SubPath) \
    REGISTER_LOCAL_PREF_WITH_PATH_IMPL(Type, KeyName, DefaultValue, SubPath, KeyName##_##__LINE__)

#define REGISTER_LOCAL_PREF(Type, KeyName, DefaultValue) \
    REGISTER_LOCAL_PREF_IMPL(Type, KeyName, DefaultValue, KeyName##_##__LINE__)