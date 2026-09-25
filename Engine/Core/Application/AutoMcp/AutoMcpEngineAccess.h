#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include "../../../Module/Guid/Guid.h"

namespace NanamiEngine::Scene
{
    class Scene;
}

namespace NanamiEngine::Module::Asset
{
    class Mv1File;
}

namespace NanamiEngine::Module::Physics
{
    enum class ColliderShapeKind : uint32_t;
    enum class Layer : uint32_t;
}

namespace NanamiEngine::Core::MainWindow
{
    class AnimationPreviewSlot;
    class AnimationViewWindow;
    class GameWindow;
    class ModelPreviewStage;
    class ModelViewWindow;
}

namespace NanamiEngine::Core::PopupWindow
{
    class IPopupWindow;
    class InspectorWindow;
    class PopupWindowGroup;
}

namespace NanamiEngine::Core::Application::AutoMcp
{
    /**
     * @brief AutoMCP のコマンド実装だけが、エンジン側クラスの非公開部分のうち必要なものに触れるための窓口 (Attorney-Client)。
     * エンジン側クラスはこのクラスだけを friend にし、MCP のために public を増やさない。
     */
    class NANAMI_API AutoMcpEngineAccess final
    {
        friend class AutoMcpCommandHandlers;

        AutoMcpEngineAccess() = delete;

        [[nodiscard]] static std::vector<std::shared_ptr<NanamiEngine::Scene::Scene>> Scenes(const MainWindow::GameWindow& gameWindow);
        [[nodiscard]] static std::shared_ptr<NanamiEngine::Scene::Scene> MainScene(const MainWindow::GameWindow& gameWindow);
        [[nodiscard]] static const std::string& FilePath(const NanamiEngine::Scene::Scene& scene);
        /** @brief 読み込み済みシーンをファイルから読み直して差し替える。見つからなければ nullptr */
        static std::shared_ptr<NanamiEngine::Scene::Scene> ReloadScene(MainWindow::GameWindow& gameWindow, const ::Guid& guid);

        static void Play(MainWindow::GameWindow& gameWindow);
        static void Stop(MainWindow::GameWindow& gameWindow);
        static void End(MainWindow::GameWindow& gameWindow);

        static void ForEachPopupWindow(PopupWindow::PopupWindowGroup& group, const std::function<void(const ::Guid&, PopupWindow::IPopupWindow&)>& action);
        static bool ClosePopupWindow(PopupWindow::PopupWindowGroup& group, const ::Guid& guid);
        /** @brief PopupWindowGroup::Catch と違い、無ければ作らない */
        [[nodiscard]] static std::vector<PopupWindow::InspectorWindow*> ExistingInspectorWindows(PopupWindow::PopupWindowGroup& group);

        [[nodiscard]] static std::vector<std::shared_ptr<Module::Asset::Mv1File>> ModelViewContents(const MainWindow::ModelViewWindow& window);
        [[nodiscard]] static std::optional<::Guid> ModelViewSelectedGuid(const MainWindow::ModelViewWindow& window);
        static void ModelViewClose(MainWindow::ModelViewWindow& window, const ::Guid& guid);
        [[nodiscard]] static MainWindow::ModelPreviewStage& ModelViewStage(MainWindow::ModelViewWindow& window);

        [[nodiscard]] static MainWindow::ModelPreviewStage& AnimationViewStage(MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static std::shared_ptr<Module::Asset::Mv1File> AnimationViewModel(const MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static MainWindow::AnimationPreviewSlot& AnimationViewSlot(MainWindow::AnimationViewWindow& window, bool isSlotB);
        [[nodiscard]] static bool&  AnimationViewPlaying       (MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static bool&  AnimationViewUseBlend      (MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static float& AnimationViewBlendWeight   (MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static bool&  AnimationViewNameCheck     (MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static bool&  AnimationViewLockRootMotion(MainWindow::AnimationViewWindow& window);
        [[nodiscard]] static int&   AnimationViewRootFrameIndex(MainWindow::AnimationViewWindow& window);

        [[nodiscard]] static std::shared_ptr<Module::Asset::Mv1File> SlotAnimationFile(const MainWindow::AnimationPreviewSlot& slot);
        /** @brief nullptr ならモデル自身が持つクリップを使う */
        static void SetSlotAnimationFile(MainWindow::AnimationPreviewSlot& slot, const std::shared_ptr<Module::Asset::Mv1File>& file);
        /** @brief 指定したアニメ元が Sync で反映済みで、ロードも終わっているか(モデル自身のクリップなら反映済みかだけ) */
        [[nodiscard]] static bool  IsSlotSourceReady(const MainWindow::AnimationPreviewSlot& slot);
        /** @brief クリップ一覧を引く DxLib ハンドル。準備できていなければ -1 */
        [[nodiscard]] static int   SlotClipSourceHandle(const MainWindow::AnimationPreviewSlot& slot, int modelHandle);
        static void SelectSlotClip(MainWindow::AnimationPreviewSlot& slot, int clipIndex);
        [[nodiscard]] static bool  IsSlotAttached(const MainWindow::AnimationPreviewSlot& slot);
        [[nodiscard]] static int   SlotClipIndex (const MainWindow::AnimationPreviewSlot& slot);
        [[nodiscard]] static float SlotTotalTime (const MainWindow::AnimationPreviewSlot& slot);
        [[nodiscard]] static float& SlotSpeed    (MainWindow::AnimationPreviewSlot& slot);
        [[nodiscard]] static bool&  SlotLoop     (MainWindow::AnimationPreviewSlot& slot);
        [[nodiscard]] static float& SlotStartTime(MainWindow::AnimationPreviewSlot& slot);
        [[nodiscard]] static float& SlotEndTime  (MainWindow::AnimationPreviewSlot& slot);
        /** @brief アタッチ前でも次フレームの Sync で再シークされるよう、時間は直接入れる */
        static void SetSlotTime(MainWindow::AnimationPreviewSlot& slot, float time);

        /** @brief viewDirection の向きから、モデル全体が入る距離にカメラを置き直す(次のフレームで反映) */
        static void PreviewFrame(MainWindow::ModelPreviewStage& stage, const glm::vec3& viewDirection);
        [[nodiscard]] static glm::vec3 PreviewCameraPosition(const MainWindow::ModelPreviewStage& stage);
        [[nodiscard]] static glm::quat PreviewCameraRotation(const MainWindow::ModelPreviewStage& stage);
        static void SetPreviewCamera(MainWindow::ModelPreviewStage& stage, const glm::vec3& position, const glm::quat& rotation);

        [[nodiscard]] static bool& DebugDrawAllColliders();
        [[nodiscard]] static bool& DebugDrawColliderKind(Module::Physics::ColliderShapeKind kind);
        [[nodiscard]] static bool& DebugDrawColliderLayer(Module::Physics::Layer layer);
        [[nodiscard]] static bool& DebugDrawTriggerColliders();
        [[nodiscard]] static bool& DebugDrawMainCameraFrustum();
        [[nodiscard]] static bool& DebugDrawVirtualCameraFrustums();
    };
}
