#pragma once

namespace NanamiEngine::Core::Network
{
    /**
     * Network上で同期するParameterのネットワーク上で共通のID
     * フォーマット: 上位32bit = NetworkObjectId, 下位32bit = オブジェクト内paramIndex
     */
    struct ParameterId final
    {
        explicit ParameterId(uint64_t id = UINT64_MAX);

        auto operator<=>(const ParameterId&) const = default;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(id_);
        }

        [[nodiscard]] uint64_t Value() const { return id_; }
        [[nodiscard]] std::string ToString() const;

    private:
        uint64_t id_ = UINT64_MAX;
    };
}

template <>
struct std::hash<NanamiEngine::Core::Network::ParameterId>
{
    size_t operator()(const NanamiEngine::Core::Network::ParameterId& id) const noexcept
    {
        return std::hash<uint64_t>{}(id.Value());
    }
};