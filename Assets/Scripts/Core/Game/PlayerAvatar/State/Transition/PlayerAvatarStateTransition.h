#pragma once
#include <cstdint>
#include <functional>

#include "../../Input/PlayerAvatarInput.h"

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarControlAcceptance : uint8_t
    {
        None,
        /// 一瞬で終わるので、受け付ける操作は直前の State のものとみなす
        Momentary,
        Accept,
    };

    enum class PlayerAvatarInputPhase : uint8_t
    {
        Pressed,
        Holding,
        NotHolding,
    };

    /**
     * @brief State が起こしうる遷移と State 内の操作を、評価順に受け取る
     * @note 遷移させる実装は最初に成立した遷移で止まり、以降の宣言は実行しない。表示用の実装には全ての宣言が届く
     * @return 実際に遷移したか。表示など遷移を行わない実装は常に false を返す
     */
    template <typename StateTypeT, typename InputT, typename ActionT>
    class IPlayerAvatarTransitionVisitor
    {
    public:
        using StateType  = StateTypeT;
        using InputType  = InputT;
        using ActionType = ActionT;

        virtual ~IPlayerAvatarTransitionVisitor() = default;

        virtual bool Automatic(StateTypeT to, bool condition) = 0;
        /** @param isUsable 入力以外の遷移条件。操作ガイドの使用可否表示にも使われる */
        virtual bool OnInput(StateTypeT to, InputT input, PlayerAvatarInputPhase phase, bool isUsable) = 0;
        virtual void Action(ActionT action, bool isUsable) = 0;
    };

    template <typename T>
    [[nodiscard]] bool IsInputInPhase(const PlayerAvatarInput<T>& input, const PlayerAvatarInputPhase phase)
    {
        switch (phase)
        {
        case PlayerAvatarInputPhase::Pressed:    return input.IsPressed();
        case PlayerAvatarInputPhase::Holding:    return input.IsUpdatePressed();
        case PlayerAvatarInputPhase::NotHolding: return !input.IsUpdatePressed();
        }
        return false;
    }

    /** @brief VisitTransitions の宣言どおりに遷移させる。最初に成立した遷移で止まる */
    template <typename TransitionVisitorT>
    class PlayerAvatarTransitionExecutorBase : public TransitionVisitorT
    {
    public:
        using StateType  = typename TransitionVisitorT::StateType;
        using InputType  = typename TransitionVisitorT::InputType;
        using ActionType = typename TransitionVisitorT::ActionType;

        explicit PlayerAvatarTransitionExecutorBase(const std::function<void(StateType)>& onChangeState)
            : onChangeState_(onChangeState)
        {
        }

        bool Automatic(const StateType to, const bool condition) final
        {
            return TryChange(to, condition);
        }

        bool OnInput(const StateType to, const InputType input, const PlayerAvatarInputPhase phase, const bool isUsable) final
        {
            return TryChange(to, isUsable && IsTriggered(input, phase));
        }

        void Action(ActionType, bool) final {}

        [[nodiscard]] bool HasChanged() const { return hasChanged_; }

    protected:
        // 遷移した後の宣言は、遷移前の State の条件で書かれているので実行しない
        bool TryChange(const StateType to, const bool condition)
        {
            if (hasChanged_ || !condition)
                return false;

            onChangeState_(to);
            hasChanged_ = true;
            return true;
        }

        void MarkChanged() { hasChanged_ = true; }

        [[nodiscard]] virtual bool IsTriggered(InputType input, PlayerAvatarInputPhase phase) const = 0;

    private:
        const std::function<void(StateType)>& onChangeState_;
        bool hasChanged_ = false;
    };
}
