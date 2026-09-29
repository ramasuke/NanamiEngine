#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>
#include <string_view>
#include <vector>

namespace NanamiEngine::UiFlow
{
    class UiScreen;

    /**
     * @brief 開いている画面の重なり。最後に開いた画面が最前面で、入力を受けるのはその画面だけ
     * @note  画面の出し入れは UiScreen::Open() / Close() が行う
     */
    class NANAMI_API ScreenStack final
    {
    public:
        static ScreenStack& Instance();

        ScreenStack(const ScreenStack&)            = delete;
        ScreenStack& operator=(const ScreenStack&) = delete;

        [[nodiscard]] bool IsEmpty() const { return screens_.empty(); }
        [[nodiscard]] UiScreen* Top() const;
        [[nodiscard]] bool IsOpen(std::string_view screenId) const;
        [[nodiscard]] std::vector<std::string> ScreenIds() const;

        void Clear();

    private:
        friend class UiScreen;

        ScreenStack() = default;

        void Push  (UiScreen& screen);
        void Remove(UiScreen& screen);

        std::vector<UiScreen*> screens_;
    };
}
