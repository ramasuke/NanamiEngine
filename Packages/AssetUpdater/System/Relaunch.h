#pragma once
#include <cstddef>

namespace NanamiEngine::AssetUpdater
{
    /**
     * このプロセスが終わったあとで、同じ exe を同じ作業ディレクトリから起動し直す。
     * 終わらせるのは呼び出し側。古いプロセスの終了処理 (セーブなど) と新しいプロセスが重ならないよう、
     * 起動は atexit の中で行う
     */
    class Relauncher final
    {
    public:
        Relauncher() = delete;

        /** 起動し直すよう予約する。2回目以降は何もせず true */
        [[nodiscard]] static bool ScheduleOnExit();

    private:
        static void RelaunchAtExit();

        static constexpr std::size_t PATH_CAPACITY = 4096;

        // NOTE: atexit のコールバックは何も捕まえられないので、破棄の順番を気にしなくていい静的な配列に置く
        static wchar_t executablePath_  [PATH_CAPACITY];
        static wchar_t workingDirectory_[PATH_CAPACITY];
        static wchar_t commandLine_     [PATH_CAPACITY + 2];
        static bool    scheduled_;
    };
}
