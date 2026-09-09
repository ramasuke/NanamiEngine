#pragma once
#include <cstdint>
#include <compare>
#include <string>
#include <type_traits>

#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Core::Network
{
    /**
     * 汎用RPC(Module::Network::Rpc<Args...>)の識別子。
     * ゲーム側のERpcType(あるいはengine独自のenum)の数値をそのまま保持する。
     */
    struct RpcId final
    {
        explicit constexpr RpcId(uint32_t value = 0) : value_(value) {}

        template<typename E>
        requires(std::is_enum_v<E> || std::is_integral_v<E>)
        static constexpr RpcId Create(const E value)
        {
            return RpcId(static_cast<uint32_t>(value));
        }

        auto operator<=>(const RpcId&) const = default;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(value_);
        }

        [[nodiscard]] constexpr uint32_t Value() const { return value_; }
        [[nodiscard]] std::string ToString() const;

    private:
        uint32_t value_ = 0;
    };
}
