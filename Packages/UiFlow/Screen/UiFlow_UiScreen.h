#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <string>

#include "../Input/UiFlow_UiInputReader.h"
#include "../../R4/R4.h"
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace NanamiEngine::UiFlow
{
    enum class ScreenState : std::uint8_t
    {
        Closed,
        Opened,
        Covered,
    };

    /**
     * @brief メニュー画面 1 枚。画面の Presenter と同じ GameObject に付け、Presenter が Open() / Close() と Input() を使う
     * @note  開いている間の操作ロックと、閉じずに破棄されたときの後始末はここが行う
     */
    class NANAMI_API UiScreen final : public Component::ComponentBase,
                                      public LifeCycleCallback::IUpdatable
    {
    public:
        UiScreen();
        ~UiScreen() override;

        bool Open();
        void Close();

        [[nodiscard]] ScreenState State() const { return state_; }
        [[nodiscard]] bool IsOpen   () const { return state_ != ScreenState::Closed; }
        [[nodiscard]] bool IsFocused() const { return state_ == ScreenState::Opened; }
        [[nodiscard]] const std::string& ScreenId() const { return screenId_; }
        /** @brief 最前面で開いている間だけ入力を返す */
        [[nodiscard]] UiInputReader& Input() { return input_; }

        [[nodiscard]] R4::Observable<R4::Unit> OnOpened  () const { return onOpened_  .AsObservable(); }
        [[nodiscard]] R4::Observable<R4::Unit> OnClosed  () const { return onClosed_  .AsObservable(); }
        /** @brief 上に別の画面が開いた */
        [[nodiscard]] R4::Observable<R4::Unit> OnCovered () const { return onCovered_ .AsObservable(); }
        /** @brief 上の画面が閉じて、最前面に戻った */
        [[nodiscard]] R4::Observable<R4::Unit> OnRevealed() const { return onRevealed_.AsObservable(); }

    private:
        friend class ScreenStack;

        void OnUpdate () override;
        void OnDestroy() override;

        /** @brief 通知を出さずにスタックから外れ、ロックを返す */
        void Detach();
        void Cover ();
        void Reveal();

        [[serialize(0)]] std::string screenId_;
        [[serialize(0)]] bool  locksPlayerControl_  = true;
        
        [[serialize(0)]] bool  destroysOnClose_     = true;
        [[serialize(0)]] float repeatDelay_secs_    = 0.35f;
        [[serialize(0)]] float repeatInterval_secs_ = 0.08f;

        ScreenState   state_ = ScreenState::Closed;
        UiInputReader input_;
        bool          isDestroyPending_ = false;
        R4::SerialDisposable  controlLock_;
        R4::Subject<R4::Unit> onOpened_;
        R4::Subject<R4::Unit> onClosed_;
        R4::Subject<R4::Unit> onCovered_;
        R4::Subject<R4::Unit> onRevealed_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(screenId_));
            archive(CEREAL_NVP(locksPlayerControl_));
            archive(CEREAL_NVP(destroysOnClose_));
            archive(CEREAL_NVP(repeatDelay_secs_));
            archive(CEREAL_NVP(repeatInterval_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(screenId_));
            if (version >= 0) archive(CEREAL_NVP(locksPlayerControl_));
            if (version >= 0) archive(CEREAL_NVP(destroysOnClose_));
            if (version >= 0) archive(CEREAL_NVP(repeatDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(repeatInterval_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::UiFlow::UiScreen, 0);
