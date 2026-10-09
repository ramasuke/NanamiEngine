#pragma once
#include "Engine/Core/Api/NanamiApi.h"
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
    // NOTE: プレビュー再生する 1 クリップ分の再生状態と DxLib のアタッチ管理
    // NOTE: 再生仕様は AnimationClipNode に合わせる。animationFile_ が空ならモデル自身のアニメーションを使う
    class NANAMI_API AnimationPreviewSlot final
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        AnimationPreviewSlot() = default;
        ~AnimationPreviewSlot();
        AnimationPreviewSlot(const AnimationPreviewSlot&)            = delete;
        AnimationPreviewSlot& operator=(const AnimationPreviewSlot&) = delete;

        // NOTE: アニメ .mv1 を読み、変更に応じてアタッチし直す。modelHandle が -1 なら何もしない
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
        // NOTE: クリップを切り替え、前のクリップ用の再生区間と時間をリセットする
        void  SelectClip(int clipIndex);
        // NOTE: クリップ一覧を引くハンドル。animationFile_ が空ならモデル自身
        [[nodiscard]] int   ClipSourceHandle(int modelHandle) const;
        [[nodiscard]] float ClipEndTime() const;

        FIELD(Module::Asset::Mv1File) animationFile_;
        std::optional<Guid> sourceGuid_;
        // NOTE: LoadDxLibHandle で複製した自前のハンドル。解放もここで行う
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
        // NOTE: 0 以下ならクリップ末尾
        float endTime_   = 0.0f;
        bool  isLoop_    = true;

        ImGuiTextFilter clipFilter_;
    };
}
