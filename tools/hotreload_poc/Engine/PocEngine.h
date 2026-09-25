#pragma once
// HotReload PoC のエンジン側 (docs/HotReload.md §10 PoC)。実エンジンの Serialization モジュールをそのまま使い、
// 多相型の最小の階層だけをここで定義する。Component は「Scene が持つ多相ポインタ」の代わり。
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <../cereal/include/cereal/cereal.hpp>
#include <../cereal/include/cereal/types/base_class.hpp>
#include "Engine/Core/Api/NanamiApi.h"

namespace Poc
{
    struct Component
    {
        virtual ~Component() = default;
        int id = 0;
        template <class Archive>
        void serialize(Archive& archive, std::uint32_t) { archive(CEREAL_NVP(id)); }
    };

    /** 2 つ目の基底 (ENGINE 側の IUpdatable に相当)。ゲーム側が NANAMI_REGISTER_POLYMORPHIC_RELATION で関係を足す */
    struct IUpdatable
    {
        virtual ~IUpdatable() = default;
        virtual int Tick() = 0;
        template <class Archive>
        void serialize(Archive&, std::uint32_t) {}
    };

    /** エンジンが登録する型。ゲーム側の GameComponent はこれを継承する (推移的な caster の検証用) */
    struct EngineComponent : Component
    {
        float speed = 1.0f;
        template <class Archive>
        void serialize(Archive& archive, std::uint32_t)
        {
            archive(cereal::base_class<Component>(this), CEREAL_NVP(speed));
        }
    };

    /** cereal の表の大きさ。差し替え前後で戻ることを確かめる */
    struct Stats
    {
        std::size_t inputJson = 0, inputBinary = 0, outputJson = 0, outputBinary = 0;
        std::size_t casterBases = 0, casterEntries = 0, reverseEntries = 0;
        std::size_t versions = 0, sharedStatics = 0, records = 0;
        bool operator==(const Stats&) const = default;
    };

    class NANAMI_API Engine
    {
    public:
        static std::shared_ptr<Component> CreateEngineComponent();
        // エンジン側のコードで保存・復元する (= Scene のロードに相当)。ゲーム DLL の型を含んでいてもよい
        static std::string                SaveJson  (const std::shared_ptr<Component>& component);
        static std::shared_ptr<Component> LoadJson  (const std::string& json);
        static std::string                SaveBinary(const std::shared_ptr<Component>& component);
        static std::shared_ptr<Component> LoadBinary(const std::string& bytes);
        static std::string                TypeName  (const Component& component);
        static Stats                      GetStats  ();
    };
}

CEREAL_CLASS_VERSION(Poc::Component,       0);
CEREAL_CLASS_VERSION(Poc::IUpdatable,      0);
CEREAL_CLASS_VERSION(Poc::EngineComponent, 0);
