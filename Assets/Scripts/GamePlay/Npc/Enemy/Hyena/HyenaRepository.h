#pragma once
#include <memory>
#include <vector>

namespace GamePlay::Npc::Enemy
{
    class Hyena;

    /** 生成した Hyena の weak_ptr の入れ物。読むたびに死んだ分を落とす */
    class HyenaRepository final
    {
    public:
        void Add(const std::weak_ptr<Hyena>& hyena);
        void Remove(const std::weak_ptr<Hyena>& hyena);
        void Clear();

        [[nodiscard]] std::vector<std::shared_ptr<Hyena>> All() const;
        [[nodiscard]] size_t Count() const;

    private:
        void PruneExpired();

        std::vector<std::weak_ptr<Hyena>> hyenas_;
    };
}
