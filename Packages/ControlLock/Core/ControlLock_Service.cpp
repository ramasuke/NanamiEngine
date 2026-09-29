#include "ControlLock_Service.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"

namespace NanamiEngine::ControlLock
{
    namespace
    {
        std::size_t ControlLockChannelIndex(const Channel channel)
        {
            return static_cast<std::size_t>(channel);
        }
    }

    Service& Service::Instance()
    {
        static Service instance;
        return instance;
    }

#if NANAMI_CONTROL_LOCK_TRACE_ENABLED
    R4::Disposable Service::Acquire(const Channel channel, const std::source_location location)
    {
        auto& entry = Add(channel, {});
        entry.holder.file     = location.file_name();
        entry.holder.function = location.function_name();
        entry.holder.line     = location.line();
        return entry.token;
    }

    R4::Disposable Service::AcquireKeyed(const std::string_view key, const Channel channel, const std::source_location location)
    {
        const auto it = std::ranges::find_if(entries_, [&](const Entry& entry)
        {
            return entry.holder.channel == channel && entry.holder.key == key;
        });
        if (it != entries_.end())
            return it->token;

        auto& entry = Add(channel, key);
        entry.holder.file     = location.file_name();
        entry.holder.function = location.function_name();
        entry.holder.line     = location.line();
        return entry.token;
    }
#else
    R4::Disposable Service::Acquire(const Channel channel)
    {
        return Add(channel, {}).token;
    }

    R4::Disposable Service::AcquireKeyed(const std::string_view key, const Channel channel)
    {
        const auto it = std::ranges::find_if(entries_, [&](const Entry& entry)
        {
            return entry.holder.channel == channel && entry.holder.key == key;
        });
        if (it != entries_.end())
            return it->token;

        return Add(channel, key).token;
    }
#endif

    R4::Disposable Service::AcquireLabeled(const std::string_view label, const Channel channel)
    {
        auto& entry = Add(channel, {});
#if NANAMI_CONTROL_LOCK_TRACE_ENABLED
        entry.holder.label = label;
#else
        (void)label;
#endif
        return entry.token;
    }

    void Service::ReleaseKeyed(const std::string_view key)
    {
        // NOTE: Dispose が Release を呼んで entries_ を書き換えるので、先に写しを集める
        std::vector<R4::Disposable> tokens;
        for (const auto& entry : entries_)
        {
            if (!entry.holder.key.empty() && entry.holder.key == key)
                tokens.push_back(entry.token);
        }
        for (const auto& token : tokens)
        {
            token.Dispose();
        }
    }

    bool Service::IsKeyHeld(const std::string_view key) const
    {
        return std::ranges::any_of(entries_, [key](const Entry& entry)
        {
            return !entry.holder.key.empty() && entry.holder.key == key;
        });
    }

    bool Service::IsLocked(const Channel channel) const
    {
        if (HasHolder(channel))
            return true;

        const auto index = ControlLockChannelIndex(channel);
        return hasReleased_[index] && lastReleaseFrame_[index] == Time::FrameCount();
    }

    bool Service::HasHolder(const Channel channel) const
    {
        return std::ranges::any_of(entries_, [channel](const Entry& entry)
        {
            return entry.holder.channel == channel;
        });
    }

    std::vector<Holder> Service::Holders() const
    {
        std::vector<Holder> holders;
        holders.reserve(entries_.size());
        for (const auto& entry : entries_)
        {
            holders.push_back(entry.holder);
        }
        return holders;
    }

    void Service::Clear()
    {
        entries_.clear();
        hasReleased_.fill(false);
    }

    Service::Entry& Service::Add(const Channel channel, const std::string_view key)
    {
        const std::uint64_t id = nextId_++;

        Entry entry;
        entry.holder.id      = id;
        entry.holder.channel = channel;
        entry.holder.key     = key;
#if NANAMI_CONTROL_LOCK_TRACE_ENABLED
        entry.holder.acquiredFrame = Time::FrameCount();
#endif
        // NOTE: 解除の処理はエンジン側のコードで、持つのは id だけ。Game.dll を指すものを残さない
        entry.token = R4::Disposable::Create([id]
        {
            Instance().Release(id);
        });
        entries_.push_back(std::move(entry));
        return entries_.back();
    }

    void Service::Release(const std::uint64_t id)
    {
        const auto it = std::ranges::find_if(entries_, [id](const Entry& entry)
        {
            return entry.holder.id == id;
        });
        if (it == entries_.end())
            return;

        const auto index = ControlLockChannelIndex(it->holder.channel);
        entries_.erase(it);
        hasReleased_     [index] = true;
        lastReleaseFrame_[index] = Time::FrameCount();
    }
}
