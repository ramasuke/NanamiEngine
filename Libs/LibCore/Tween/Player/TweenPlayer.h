#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>

#include "Libs/tweeny/Tweeny/tweeny.h"

namespace LibCore::Tween
{
    // NOTE: 秒を tweeny の区間長(uint16_t の ms)に変換する
    // NOTE: 0ms は NaN になるので最小 1ms
    inline uint16_t Ms(const float secs)
    {
        return static_cast<uint16_t>(std::clamp(secs * 1000.0f, 1.0f, 65535.0f));
    }

    // NOTE: 毎フレーム Tick して使う tween の再生器
    template <typename T>
    class TweenPlayer
    {
    public:
        // NOTE: 最初から再生する
        void Play(tweeny::tween<T> tween)
        {
            Set(std::move(tween));
            isPlaying_ = true;
        }

        // NOTE: 始点に置いて止めておく。往復フェードは Set してから PlayForward / PlayBackward
        void Set(tweeny::tween<T> tween)
        {
            tween_ = std::move(tween);
            tween_->forward();
            tween_->seek(static_cast<int32_t>(0), true);
            remainderMs_ = 0.0f;
            isPlaying_ = false;
        }

        // NOTE: 終わったフレームだけ true を返す
        bool Tick(const float deltaTime)
        {
            if (!isPlaying_ || !tween_)
                return false;

            tween_->step(AdvanceMs(deltaTime), true);
            if (!IsAtEnd())
                return false;

            isPlaying_ = false;
            return true;
        }

        // NOTE: 今の進行度から順方向へ再生する(終端にいれば何もしない)
        void PlayForward()
        {
            SetDirection(true);
        }

        // NOTE: 今の進行度から逆方向へ再生する(始点にいれば何もしない)
        void PlayBackward()
        {
            SetDirection(false);
        }

        // NOTE: 停止する。Value は今の値のまま
        void Stop()
        {
            isPlaying_ = false;
        }

        // NOTE: 再生方向の終端へ飛ばす
        void Complete()
        {
            if (!tween_)
                return;

            tween_->seek(IsForward() ? static_cast<int32_t>(tween_->duration()) : static_cast<int32_t>(0), true);
            isPlaying_ = false;
        }

        [[nodiscard]] const T& Value() const
        {
            return tween_ ? tween_->peek() : empty_;
        }

        [[nodiscard]] float Progress() const
        {
            return tween_ ? tween_->progress() : 0.0f;
        }

        [[nodiscard]] bool IsPlaying() const
        {
            return isPlaying_;
        }

        [[nodiscard]] bool IsFinished() const
        {
            return tween_ && !isPlaying_ && IsAtEnd();
        }

        [[nodiscard]] bool IsForward() const
        {
            return !tween_ || tween_->direction() > 0;
        }

    private:
        void SetDirection(const bool forward)
        {
            if (!tween_)
                return;

            if (forward)
                tween_->forward();
            else
                tween_->backward();
            isPlaying_ = !IsAtEnd();
        }

        [[nodiscard]] bool IsAtEnd() const
        {
            return IsForward() ? tween_->currentTimePoint() >= tween_->duration() : tween_->currentTimePoint() == 0;
        }

        // NOTE: Coroutine::TweenClock と同じく、ms 未満の端数を次のフレームへ繰り越す
        [[nodiscard]] int32_t AdvanceMs(const float deltaTime)
        {
            remainderMs_ += deltaTime * 1000.0f;
            const float wholeMs = std::floor(remainderMs_);
            remainderMs_ -= wholeMs;
            return static_cast<int32_t>(wholeMs);
        }

        std::optional<tweeny::tween<T>> tween_;
        float remainderMs_ = 0.0f;
        bool isPlaying_ = false;
        T empty_{};
    };
}
