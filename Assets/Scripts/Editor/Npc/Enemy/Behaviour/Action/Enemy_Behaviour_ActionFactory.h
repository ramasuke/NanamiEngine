#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../../../../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    class ActionBase;
}

namespace Editor::Npc::Enemy::Behaviour
{
    class ActionFactory final : public SingletonBase<ActionFactory>
    {
    public:
        using CreateFunc = std::function<std::unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase>()>;

        /** アクション型登録 */
        void Register(const std::string& typeName, CreateFunc func);

        /** アクション生成 */
        [[nodiscard]] std::unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase> Create(const std::string& typeName) const;

        /** 登録済み型一覧（エディタ用） */
        [[nodiscard]] const std::unordered_map<std::string, CreateFunc>& CreatableActions() const;

    private:
        std::unordered_map<std::string, CreateFunc> creatableActionFactories_;
    };
}

#define REGISTER_ENEMY_ACTION_WITH_NAME(TYPE, NAME)                    \
namespace                                                              \
{                                                                      \
    struct TYPE##ActionAutoRegister                                    \
    {                                                                  \
        TYPE##ActionAutoRegister()                                     \
        {                                                              \
            auto& factory =                                            \
            Editor::Npc::Enemy::Behaviour::ActionFactory::Instance();  \
            factory.Register(                                          \
                NAME,                                                  \
                []() -> std::unique_ptr<                               \
                GameCore::Npc::Enemy::Behaviour::ActionBase>           \
                {                                                      \
                    return std::make_unique<TYPE>();                   \
                });                                                    \
        }                                                              \
    };                                                                 \
    [[maybe_unused]] static TYPE##ActionAutoRegister                   \
    global_##TYPE##ActionAutoRegister;                                 \
}

#define REGISTER_ENEMY_ACTION(TYPE) \
REGISTER_ENEMY_ACTION_WITH_NAME(TYPE, #TYPE)