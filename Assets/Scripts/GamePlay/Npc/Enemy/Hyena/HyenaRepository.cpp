#include "HyenaRepository.h"

#include <algorithm>

#include "GamePlay_Enemy_Hyena.h"

namespace GamePlay::Npc::Enemy
{
    void HyenaRepository::Add(const std::weak_ptr<Hyena>& hyena)
    {
        if (hyena.expired())
            return;

        PruneExpired();
        hyenas_.push_back(hyena);
    }

    void HyenaRepository::Remove(const std::weak_ptr<Hyena>& hyena)
    {
        const auto target = hyena.lock();
        std::erase_if(hyenas_, [&](const std::weak_ptr<Hyena>& weak)
        {
            const auto locked = weak.lock();
            return !locked || locked == target;
        });
    }

    void HyenaRepository::Clear()
    {
        hyenas_.clear();
    }

    std::vector<std::shared_ptr<Hyena>> HyenaRepository::All() const
    {
        std::vector<std::shared_ptr<Hyena>> result;
        result.reserve(hyenas_.size());
        for (const auto& weak : hyenas_)
        {
            if (auto locked = weak.lock())
                result.push_back(std::move(locked));
        }
        return result;
    }

    size_t HyenaRepository::Count() const
    {
        return std::count_if(hyenas_.begin(), hyenas_.end(),
            [](const std::weak_ptr<Hyena>& weak) { return !weak.expired(); });
    }

    void HyenaRepository::PruneExpired()
    {
        std::erase_if(hyenas_, [](const std::weak_ptr<Hyena>& weak) { return weak.expired(); });
    }
}
