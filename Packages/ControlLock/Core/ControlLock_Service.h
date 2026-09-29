#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../ControlLockConfig.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#if NANAMI_CONTROL_LOCK_TRACE_ENABLED
#include <source_location>
#endif

#include "../../R4/R4.h"

namespace NanamiEngine::UiFlow
{
    class UiScreen;
}

namespace NanamiEngine::ControlLock
{
    /** @brief 何を止めるか */
    enum class Channel : std::uint8_t
    {
        PlayerControl = 0,
        Count,
    };

    /** @brief ロックを持っている相手 */
    struct NANAMI_API Holder
    {
        std::uint64_t id      = 0;
        Channel       channel = Channel::PlayerControl;
        /** AcquireKeyed で取ったときだけ入る */
        std::string   key;
#if NANAMI_CONTROL_LOCK_TRACE_ENABLED
        /** UiScreen が取ったときの screenId */
        std::string   label;
        // NOTE: Game.dll の文字列リテラルを指し続けないよう、コピーして持つ
        std::string   file;
        std::string   function;
        std::uint32_t line          = 0;
        std::uint64_t acquiredFrame = 0;
#endif
    };

    /**
     * @brief 操作ロックの集約。取得した数を数え、誰かが持っている間はロック中になる
     * @note  ここは「止めたい」という要求を集めるだけ。実際に止めるのはゲーム側 (アバター) が IsLocked() を見て行う
     */
    class NANAMI_API Service final
    {
    public:
        /** @brief モジュール (exe / DLL) ごとに実体が分かれないよう .cpp で定義する (docs/HotReload.md §3.1) */
        static Service& Instance();

        Service(const Service&)            = delete;
        Service& operator=(const Service&) = delete;

#if NANAMI_CONTROL_LOCK_TRACE_ENABLED
        /** @brief ロックを取る。戻り値を Dispose するまで持ち続けるので、AddTo(this) か ScopedLock で寿命を決める */
        [[nodiscard]] R4::Disposable Acquire(
            Channel channel = Channel::PlayerControl,
            std::source_location location = std::source_location::current());
        /**
         * @brief Lock / Unlock が別々に呼ばれる相手 (BT ノード、RPC) 用。ReleaseKeyed(key) で返す
         * @note  同じ key で取得済みなら数は増やさず、取得済みのものを返す
         */
        R4::Disposable AcquireKeyed(
            std::string_view key,
            Channel channel = Channel::PlayerControl,
            std::source_location location = std::source_location::current());
#else
        [[nodiscard]] R4::Disposable Acquire(Channel channel = Channel::PlayerControl);
        R4::Disposable AcquireKeyed(std::string_view key, Channel channel = Channel::PlayerControl);
#endif
        void ReleaseKeyed(std::string_view key);
        [[nodiscard]] bool IsKeyHeld(std::string_view key) const;

        /**
         * @brief ロック中か
         * @note  最後の持ち主が返したフレームの間も true。閉じるのに使ったキーをゲーム操作が拾わないようにするため
         */
        [[nodiscard]] bool IsLocked(Channel channel = Channel::PlayerControl) const;
        /** @brief いま持ち主がいるか (IsLocked と違い、返したフレームの保持はしない) */
        [[nodiscard]] bool HasHolder(Channel channel = Channel::PlayerControl) const;
        [[nodiscard]] std::vector<Holder> Holders() const;

        /**
         * @brief 全てのロックを捨てる。プレイ終了とホットリロードで呼ぶ
         * @note  捨てた後に残っていた Disposable が Dispose されても何も起きない
         */
        void Clear();

    private:
        friend class UiFlow::UiScreen;

        struct NANAMI_NO_API Entry
        {
            Holder         holder;
            R4::Disposable token;
        };

        Service() = default;

        /** @brief UiScreen 用。取得元が常に UiScreen の .cpp になるので、画面の名前を記録する */
        [[nodiscard]] R4::Disposable AcquireLabeled(std::string_view label, Channel channel);
        Entry& Add(Channel channel, std::string_view key);
        void Release(std::uint64_t id);

        static constexpr std::size_t CHANNEL_COUNT = static_cast<std::size_t>(Channel::Count);

        std::vector<Entry> entries_;
        std::uint64_t      nextId_ = 1;
        std::array<bool,          CHANNEL_COUNT> hasReleased_      {};
        std::array<std::uint64_t, CHANNEL_COUNT> lastReleaseFrame_ {};
    };
}
