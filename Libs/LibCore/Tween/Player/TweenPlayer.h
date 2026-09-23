#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>

#include "Libs/tweeny/Tweeny/tweeny.h"

namespace LibCore::Tween
{
    // 秒を tweeny の区間長(uint16_t の ms)に変換する
    // NOTE: 0ms の区間は 0/0 で NaN になるので最小 1ms、uint16_t に収まるよう最大 65535ms(約65秒)
    inline uint16_t Ms(const float secs)
    {
        return static_cast<uint16_t>(std::clamp(secs * 1000.0f, 1.0f, 65535.0f));
    }

    // OnUpdate から毎フレーム Tick して使う tween の再生器。コンポーネントのメンバとして持つ
    // tween の組み立ては tweeny::from(...).to(...).during(Ms(...)).via(Ease(...)) のまま
    template <typename T>
    class TweenPlayer
    {
    public:
        // 最初から再生する
        void Play(tweeny::tween<T> tween)
        {
            Set(std::move(tween));
            isPlaying_ = true;
        }

        // 始点に置いて止めておく。往復フェードは Set してから PlayForward / PlayBackward
        void Set(tweeny::tween<T> tween)
        {
            tween_ = std::move(tween);
            tween_->forward();
            tween_->seek(static_cast<int32_t>(0), true);
            remainderMs_ = 0.0f;
            isPlaying_ = false;
        }

        // 終わったフレームだけ true を返す
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

        // 今の進行度から順方向へ再生する(終端にいれば何もしない)
        void PlayForward()
        {
            SetDirection(true);
        }

        // 今の進行度から逆方向へ再生する(始点にいれば何もしない)
        void PlayBackward()
        {
            SetDirection(false);
        }

        // 停止する。Value は今の値のまま
        void Stop()
        {
            isPlaying_ = false;
        }

        // 再生方向の終端へ飛ばす
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

        // Coroutine::TweenClock と同じく、ms 未満の端数を次のフレームへ繰り越す
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
