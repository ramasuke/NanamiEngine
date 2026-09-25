#include "PocGameApi.h"

#include <sstream>

#include <../cereal/include/cereal/types/memory.hpp>
#include <../cereal/include/cereal/types/string.hpp>
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace Poc::Game
{
    /** エンジンの型を継承し、2 つ目の基底も持つ (実ゲームの Component + IUpdatable と同じ形) */
    struct GameComponent final : EngineComponent, IUpdatable
    {
        int hp = 10;
        int Tick() override { return ++hp; }
        template <class Archive>
        void serialize(Archive& archive, std::uint32_t)
        {
            archive(cereal::base_class<EngineComponent>(this), cereal::base_class<IUpdatable>(this), CEREAL_NVP(hp));
        }
    };

    /** エンジンの基底だけを継承する型 */
    struct GameOnly final : Component
    {
        std::string name = "game-only";
        template <class Archive>
        void serialize(Archive& archive, std::uint32_t)
        {
            archive(cereal::base_class<Component>(this), CEREAL_NVP(name));
        }
    };

    std::shared_ptr<Component> CreateGameComponent()
    {
        auto component = std::make_shared<GameComponent>();
        component->id = 1;
        component->speed = 3.0f;
        return component;
    }

    std::shared_ptr<Component> CreateGameOnly()
    {
        auto component = std::make_shared<GameOnly>();
        component->id = 2;
        return component;
    }

    std::string SaveFromGame(const std::shared_ptr<Component>& component)
    {
        std::stringstream stream;
        {
            cereal::JSONOutputArchive archive(stream);
            archive(cereal::make_nvp("component", component));
        }
        return stream.str();
    }

    std::shared_ptr<Component> LoadFromGame(const std::string& json)
    {
        std::stringstream stream(json);
        cereal::JSONInputArchive archive(stream);
        std::shared_ptr<Component> component;
        archive(cereal::make_nvp("component", component));
        return component;
    }

    int Tick(Component& component)
    {
        auto* updatable = dynamic_cast<IUpdatable*>(&component);
        return updatable ? updatable->Tick() : -1;
    }
}

CEREAL_CLASS_VERSION(Poc::Game::GameComponent, 1);
CEREAL_CLASS_VERSION(Poc::Game::GameOnly,      0);

NANAMI_REGISTER_TYPE(Poc::Game::GameComponent, Poc::EngineComponent);
NANAMI_REGISTER_POLYMORPHIC_RELATION(Poc::IUpdatable, Poc::Game::GameComponent)
NANAMI_REGISTER_TYPE(Poc::Game::GameOnly, Poc::Component);

POC_GAME_EXPORT const Poc::GameApi* PocGetGameApi()
{
    static const Poc::GameApi api{
        1,
        &Poc::Game::CreateGameComponent,
        &Poc::Game::CreateGameOnly,
        &Poc::Game::SaveFromGame,
        &Poc::Game::LoadFromGame,
        &Poc::Game::Tick,
    };
    return &api;
}
