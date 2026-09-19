#pragma once

namespace NanamiEngine::AssetUpdater
{
    /**
     * このプロセスが終わったあとで、同じ exe を同じ作業ディレクトリから起動し直すよう予約する。
     * 終わらせるのは呼び出し側。古いプロセスの終了処理 (セーブなど) と新しいプロセスが重ならないよう、
     * 起動は atexit の中で行う
     */
    [[nodiscard]] bool ScheduleRelaunchOnExit();
}
