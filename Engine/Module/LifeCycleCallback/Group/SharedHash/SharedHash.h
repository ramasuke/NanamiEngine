#pragma once
#include <memory>

namespace NanamiEngine::Core::Application
{
    // ハッシュと比較を shared_ptr ベースで定義
    template <typename T>
    struct SharedPtrHash
    {
        size_t operator()(const std::weak_ptr<T>& wp) const
        {
            auto sp = wp.lock();
            return std::hash<T*>()(sp.get());
        }
    };

    template <typename T>
    struct SharedPtrEqual
    {
        bool operator()(const std::weak_ptr<T>& lhs, const std::weak_ptr<T>& rhs) const
        {
            return lhs.lock().get() == rhs.lock().get();
        }
    };
}
