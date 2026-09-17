#pragma once
#include <optional>

#include "../../../../../../Libs/ImGui/ImGuiHelper.h"
#include "../../../../Object/Field/Field.h"
#include "../../../../../Module/Asset/MV1/MV1File.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Core::MainWindow
{
    /**
     * @brief AnimationViewWindow で再生する 1 クリップ分の再生状態と DxLib のアタッチ管理
     *
     * @details
     *  再生仕様(時間の進め方・区間・ループ/非ループ停止・NameCheck)は AnimationClipNode に合わせ、ゲーム中と同じ見た目になるようにする。
     *  animationFile_ が空のときはモデル自身が持つアニメーションを使う(MV1AttachAnim の AnimSrcMHandle = -1)。
     */
    class AnimationPreviewSlot final
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        AnimationPreviewSlot() = default;
        ~AnimationPreviewSlot();
        AnimationPreviewSlot(const AnimationPreviewSlot&)            = delete;
        AnimationPreviewSlot& operator=(const AnimationPreviewSlot&) = delete;

        /** @brief アニメ .mv1 の読み込みと、クリップ/ソース/モデル変更に応じたアタッチのやり直し。modelHandle が -1 なら何もしない */
        void Sync(int modelHandle, bool nameCheck);
        void Advance(float deltaSecs);
        void Apply(int modelHandle, float blendRate) const;
        void Detach(int modelHandle);
        void Seek(float time);
        void Rewind() { time_ = startTime_; }

        [[nodiscard]] float GetTime() const { return time_; }

        void DrawGui(const char* label, int modelHandle);

    private:
        void  ReleaseSource();
        /** @brief クリップを切り替え、前のクリップ用の再生区間と時間をリセットする */
        void  SelectClip(int clipIndex);
        /** @brief クリップ一覧を引くハンドル。animationFile_ が空ならモデル自身 */
        [[nodiscard]] int   ClipSourceHandle(int modelHandle) const;
        [[nodiscard]] float ClipEndTime() const;

        FIELD(Module::Asset::Mv1File) animationFile_;
        std::optional<Guid> sourceGuid_;
        // LoadDxLibHandle で複製した呼び出し側所有のハンドル
        int sourceHandle_ = -1;

        int  attachIndex_          = -1;
        int  attachedModelHandle_  = -1;
        int  attachedClipIndex_    = -1;
        int  attachedSourceHandle_ = -1;
        bool attachedNameCheck_    = false;

        int   clipIndex_ = 0;
        float totalTime_ = 0.0f;
        float time_      = 0.0f;
        float speed_     = 1.0f;
        float startTime_ = 0.0f;
        /** @brief 0 以下ならクリップ末尾 */
        float endTime_   = 0.0f;
        bool  isLoop_    = true;

        ImGuiTextFilter clipFilter_;
    };
}
