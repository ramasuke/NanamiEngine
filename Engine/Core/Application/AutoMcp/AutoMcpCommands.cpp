#include "AutoMcpCommands.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <ranges>
#include <span>
#include <sstream>
#include <system_error>
#include <typeinfo>
#include <vector>

#include "DxLib.h"
#include "imgui_internal.h"
#include "../HotReload/GameModule.h"
#include "AutoMcpEngineAccess.h"
#include "../ApplicationBase.h"
#include "../Configuration/DebugDraw/ApplicationConfiguration_DebugDraw.h"
#include "../LifeCycle/ApplicationLifeCycle.h"
#include "../Time/Time.h"
#include "../Window/Main/AnimationView/AnimationPreviewSlot.h"
#include "../Window/Main/AnimationView/AnimationViewWindow.h"
#include "../Window/Main/Factory/MainWindowFactory.h"
#include "../Window/Main/Game/GameWindow.h"
#include "../Window/Main/ModelView/ModelViewWindow.h"
#include "../Window/Main/Preview/ModelPreviewStage.h"
#include "../Window/Popup/Factory/PopupWindowFactory.h"
#include "../Window/Popup/Group/PopupWindowGroup.h"
#include "../Window/Popup/Inspector/InspectorWindow.h"
#include "../../FileSystem/Directory/Directory.h"
#include "../../FileSystem/File/File.h"
#include "../../Object/Registry/ObjectRegistry.h"
#include "../../../../Libs/LibCore/DxLib/ShiftJis.h"
#include "../../../Module/Asset/Asset.h"
#include "../../../Module/Asset/MV1/MV1File.h"
#include "../../../Module/Component/ComponentBase.h"
#include "../../../Module/GameObject/Transform/Transform.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Module/Scene/Scene.h"
#include "../../../Module/Scene/GameObject/CopiedPrefabGameObject/CopiedPrefabGameObject.h"
#include "../../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    namespace
    {
        using IGameObject   = NanamiEngine::Module::GameObject::IGameObject;
        using GameObjectPtr = std::shared_ptr<IGameObject>;
        using ScenePtr      = std::shared_ptr<NanamiEngine::Scene::Scene>;
        using ComponentPtr  = std::shared_ptr<NanamiEngine::Module::Component::ComponentBase>;

        struct FoundGameObject
        {
            GameObjectPtr gameObject;
            ScenePtr      scene;
        };

        std::shared_ptr<MainWindow::GameWindow> RequireGameWindow()
        {
            auto gameWindow = ApplicationBase::GameWindow();
            if (!gameWindow)
                throw AutoMcpError("GameWindow is not available");

            return gameWindow;
        }

        /** @brief 生成したコンポーネントは現在のメインウィンドウのライフサイクルに登録されるので、先に GameWindow へ切り替える */
        std::shared_ptr<MainWindow::GameWindow> ActivateGameWindow(JsonValue& result, JsonAllocator& allocator)
        {
            auto gameWindow = RequireGameWindow();
            const bool isSwitched = ApplicationBase::GetMainWindow() != gameWindow;
            if (isSwitched)
                ApplicationBase::OnChangeWindow<MainWindow::GameWindow>();

            result.AddMember("switchedToGameWindow", isSwitched, allocator);
            return gameWindow;
        }

        ComponentPtr FindComponent(IGameObject& gameObject, const std::string& guidText)
        {
            const ::Guid guid(guidText);
            for (const auto& weakComponent : gameObject.Components().Catches<NanamiEngine::Module::Component::ComponentBase>())
            {
                if (auto component = weakComponent.lock(); component && component->GetGuid() == guid)
                    return component;
            }
            throw AutoMcpError("component " + guidText + " not found on GameObject " + gameObject.GetGuid().Value());
        }

        std::string GameObjectPath(const GameObjectPtr& gameObject)
        {
            std::string path = gameObject->Name();
            for (auto parent = gameObject->Transform().GetParent(); parent; parent = parent->Transform().GetParent())
                path = parent->Name() + "/" + path;

            return path;
        }

        std::string ToLowerAscii(std::string text)
        {
            std::ranges::transform(text, text.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return text;
        }

        std::string CurrentMainWindowName()
        {
            const auto& mainWindow = ApplicationBase::GetMainWindow();
            return mainWindow ? ShortTypeName(typeid(*mainWindow).name()) : std::string();
        }

        std::vector<GameObjectPtr> RootGameObjects(const ScenePtr& scene)
        {
            std::vector<GameObjectPtr> roots;
            scene->ForEachGameObject([&roots](const GameObjectPtr& gameObject)
            {
                if (gameObject && !gameObject->Transform().GetParent())
                    roots.push_back(gameObject);
            });
            return roots;
        }

        bool TryGetVec2(const JsonArgs& args, const char* name, ImVec2& out)
        {
            const JsonValue* member = args.FindMember(name);
            if (member == nullptr)
                return false;

            if (!member->IsArray() || member->Size() != 2 || !(*member)[0].IsNumber() || !(*member)[1].IsNumber())
                throw AutoMcpError(std::string("argument must be [x, y]: ") + name);

            out = ImVec2((*member)[0].GetFloat(), (*member)[1].GetFloat());
            return true;
        }

        JsonValue MakeVec2(const ImVec2& value, JsonAllocator& allocator)
        {
            JsonValue array(rapidjson::kArrayType);
            array.PushBack(static_cast<double>(value.x), allocator);
            array.PushBack(static_cast<double>(value.y), allocator);
            return array;
        }

        template <typename MapT>
        JsonValue SortedKeys(const MapT& map, JsonAllocator& allocator)
        {
            std::vector<std::string> keys;
            for (const auto& key : map | std::views::keys)
                keys.push_back(key);
            std::ranges::sort(keys);

            JsonValue array(rapidjson::kArrayType);
            for (const auto& key : keys)
                array.PushBack(MakeString(key, allocator), allocator);
            return array;
        }

        JsonValue DescribeComponent(const NanamiEngine::Module::Component::ComponentBase& component, JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            value.AddMember("type",    MakeString(ShortTypeName(typeid(component).name()), allocator), allocator);
            value.AddMember("fqn",     MakeString(FullTypeName(typeid(component).name()), allocator), allocator);
            value.AddMember("guid",    MakeString(component.GetGuid().Value(), allocator), allocator);
            value.AddMember("enabled", component.IsEnable(), allocator);
            return value;
        }

        JsonValue DescribeComponents(IGameObject& gameObject, JsonAllocator& allocator)
        {
            JsonValue components(rapidjson::kArrayType);
            for (const auto& weakComponent : gameObject.Components().Catches<NanamiEngine::Module::Component::ComponentBase>())
            {
                if (const auto component = weakComponent.lock())
                    components.PushBack(DescribeComponent(*component, allocator), allocator);
            }
            return components;
        }

        void AddGameObjectHeader(JsonValue& value, IGameObject& gameObject, JsonAllocator& allocator)
        {
            value.AddMember("name",     MakeString(gameObject.Name(), allocator), allocator);
            value.AddMember("guid",     MakeString(gameObject.GetGuid().Value(), allocator), allocator);
            value.AddMember("type",     MakeString(ShortTypeName(typeid(gameObject).name()), allocator), allocator);
            value.AddMember("isActive", gameObject.IsEnable(), allocator);
            value.AddMember("mark",     MakeString(NanamiEngine::Module::GameObject::ToName(gameObject.Mark()), allocator), allocator);
        }

        JsonValue DescribeHierarchy(const GameObjectPtr& gameObject, const int depth, const int maxDepth, const bool includeComponents, JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            AddGameObjectHeader(value, *gameObject, allocator);
            if (includeComponents)
                value.AddMember("components", DescribeComponents(*gameObject, allocator), allocator);

            const auto children = gameObject->Transform().GetChildren();
            value.AddMember("childCount", static_cast<int>(children.size()), allocator);
            if (maxDepth < 0 || depth < maxDepth)
            {
                JsonValue childValues(rapidjson::kArrayType);
                for (const auto& child : children)
                {
                    if (child)
                        childValues.PushBack(DescribeHierarchy(child, depth + 1, maxDepth, includeComponents, allocator), allocator);
                }
                value.AddMember("children", childValues, allocator);
            }
            return value;
        }

        JsonValue DescribeTransform(NanamiEngine::Module::GameObject::Transform& transform, JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            value.AddMember("localPosition",     MakeVec3(transform.GetLocalPos(), allocator), allocator);
            value.AddMember("localRotation",     MakeQuat(transform.GetLocalRot(), allocator), allocator);
            value.AddMember("localEulerDegrees", MakeVec3(glm::degrees(glm::eulerAngles(transform.GetLocalRot())), allocator), allocator);
            value.AddMember("localScale",        MakeVec3(transform.GetLocalScale(), allocator), allocator);
            value.AddMember("worldPosition",     MakeVec3(transform.GetWorldPos(), allocator), allocator);
            value.AddMember("worldEulerDegrees", MakeVec3(glm::degrees(glm::eulerAngles(transform.GetWorldRot())), allocator), allocator);
            value.AddMember("worldScale",        MakeVec3(transform.GetWorldScale(), allocator), allocator);
            return value;
        }

        void AddPlayState(JsonValue& result, const MainWindow::GameWindow& gameWindow, JsonAllocator& allocator)
        {
            result.AddMember("playMode", gameWindow.IsPlayMode(), allocator);
            result.AddMember("playing",  gameWindow.IsPlaying(),  allocator);
        }

        using AssetPtr   = std::shared_ptr<NanamiEngine::Module::Asset::AssetBase>;
        using Mv1FilePtr = std::shared_ptr<NanamiEngine::Module::Asset::Mv1File>;

        bool IsDxHandleReady(const int handle)
        {
            return handle != -1 && CheckHandleASyncLoad(handle) == FALSE;
        }

        std::string DxLibName(const char* name)
        {
            return name ? LibCore::Dxlib::ShiftJisToUtf8(name) : std::string();
        }

        /** @brief contentPath_ は "Assets\\Art\\..." 形式なので、区切りと大文字小文字をそろえて比較する */
        std::string NormalizeAssetPath(std::string path)
        {
            std::ranges::replace(path, '\\', '/');
            while (path.starts_with("./"))
                path.erase(0, 2);
            return ToLowerAscii(path);
        }

        std::string DisplayAssetPath(const NanamiEngine::Module::Asset::AssetBase& asset)
        {
            std::string path = ToUtf8(asset.GetContentPath());
            std::ranges::replace(path, '\\', '/');
            return path;
        }

        void CollectAssets(FileSystem::Directory& directory, std::vector<AssetPtr>& out)
        {
            for (auto& file : directory.Files())
            {
                if (file.GetContent())
                    out.push_back(file.GetContent());
            }
            for (auto& child : directory.GetDirectories())
                CollectAssets(child, out);
        }

        std::vector<AssetPtr> AllAssets()
        {
            std::vector<AssetPtr> assets;
            CollectAssets(ApplicationBase::AssetsDirectory(), assets);
            return assets;
        }

        JsonValue DescribeAsset(const NanamiEngine::Module::Asset::AssetBase& asset, JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            value.AddMember("path", MakeString(DisplayAssetPath(asset), allocator), allocator);
            value.AddMember("guid", MakeString(asset.GetGuid().Value(), allocator), allocator);
            value.AddMember("type", MakeString(ShortTypeName(typeid(asset).name()), allocator), allocator);
            return value;
        }

        Mv1FilePtr FindMv1ByPath(const std::string& requestedPath)
        {
            const std::string wanted = NormalizeAssetPath(requestedPath);
            std::vector<Mv1FilePtr> suffixMatches;
            for (const auto& asset : AllAssets())
            {
                auto file = std::dynamic_pointer_cast<NanamiEngine::Module::Asset::Mv1File>(asset);
                if (!file)
                    continue;

                const std::string path = NormalizeAssetPath(ToUtf8(file->GetContentPath()));
                if (path == wanted)
                    return file;

                // "SwordMan.mv1" のようにファイル名や途中からのパスだけでも引けるようにする
                if (path.size() > wanted.size() && path.ends_with(wanted) && path[path.size() - wanted.size() - 1] == '/')
                    suffixMatches.push_back(file);
            }

            if (suffixMatches.size() == 1)
                return suffixMatches.front();

            if (suffixMatches.empty())
                throw AutoMcpError("no .mv1 asset matches " + requestedPath + " (find paths with assets.find)");

            std::string candidates;
            for (const auto& file : suffixMatches)
                candidates += "\n  " + DisplayAssetPath(*file);
            throw AutoMcpError(requestedPath + " matches several .mv1 assets; pass a longer path:" + candidates);
        }

        Mv1FilePtr ResolveMv1(const JsonArgs& args, const char* guidKey, const char* pathKey)
        {
            if (args.FindMember(guidKey) != nullptr)
            {
                const std::string guid = args.RequireString(guidKey);
                if (auto file = ApplicationBase::ObjectRegistry().Catch<NanamiEngine::Module::Asset::Mv1File>(::Guid(guid)).lock())
                    return file;

                throw AutoMcpError("no .mv1 asset with guid " + guid);
            }

            if (args.FindMember(pathKey) != nullptr)
                return FindMv1ByPath(args.RequireString(pathKey));

            throw AutoMcpError(std::string("pass ") + pathKey + " or " + guidKey);
        }

        int FindClipIndex(const int sourceHandle, const std::string& requestedName)
        {
            const std::string wanted = ToLowerAscii(requestedName);
            const int         count  = MV1GetAnimNum(sourceHandle);
            std::vector<int>  partialMatches;
            for (int i = 0; i < count; ++i)
            {
                const std::string name = ToLowerAscii(DxLibName(MV1GetAnimName(sourceHandle, i)));
                if (name == wanted)
                    return i;
                if (name.find(wanted) != std::string::npos)
                    partialMatches.push_back(i);
            }

            if (partialMatches.size() == 1)
                return partialMatches.front();

            if (partialMatches.empty())
                throw AutoMcpError("no clip named " + requestedName + " among " + std::to_string(count) + " clips (list them with animationview.state includeClips)");

            throw AutoMcpError(requestedName + " matches " + std::to_string(partialMatches.size()) + " clips; use the full name or clipIndex");
        }

        /** @brief yaw 0 / pitch 0 で +Z 側から -Z 向きに見る。pitch が正なら見下ろす */
        glm::vec3 ViewDirectionFromAngles(const float yawDegrees, const float pitchDegrees)
        {
            const float yaw   = glm::radians(yawDegrees);
            const float pitch = glm::radians(std::clamp(pitchDegrees, -89.0f, 89.0f));
            return glm::vec3(std::sin(yaw) * std::cos(pitch), -std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
        }

        glm::quat LookRotation(const glm::vec3& direction)
        {
            const glm::vec3 up = std::abs(direction.y) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            // Editor3DCamera の前方は +Z
            return glm::quatLookAtLH(direction, up);
        }
    }

    /**
     * @brief コマンド実装。この .cpp の中だけで定義し、AutoMcpEngineAccess の friend として
     * エンジン側クラスの非公開部分に触れる。
     */
    class AutoMcpCommandHandlers final
    {
    public:
        AutoMcpCommandHandlers() = delete;

        [[nodiscard]] static const std::unordered_map<std::string, AutoMcpCommand>& Table();

    private:
        static FoundGameObject FindGameObject(const std::string& guidText)
        {
            const ::Guid guid(guidText);
            for (const auto& scene : AutoMcpEngineAccess::Scenes(*RequireGameWindow()))
            {
                GameObjectPtr match;
                scene->ForEachGameObject([&match, &guid](const GameObjectPtr& gameObject)
                {
                    if (!match && gameObject && gameObject->GetGuid() == guid)
                        match = gameObject;
                });

                if (match)
                    return {match, scene};
            }
            throw AutoMcpError("GameObject not found in the loaded scenes: " + guidText);
        }

        static FoundGameObject RequireGameObject(const JsonArgs& args)
        {
            return FindGameObject(args.RequireString("guid"));
        }

        static JsonValue DescribeScene(const ScenePtr& scene, const ScenePtr& mainScene, JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            value.AddMember("name",      MakeString(scene->Name(), allocator), allocator);
            value.AddMember("guid",      MakeString(scene->GetGuid().Value(), allocator), allocator);
            value.AddMember("filePath",  MakeString(AutoMcpEngineAccess::FilePath(*scene), allocator), allocator);
            value.AddMember("isMain",    scene == mainScene, allocator);
            value.AddMember("rootCount", static_cast<int>(RootGameObjects(scene).size()), allocator);
            return value;
        }

        /**
         * JSON から作り直した GameObject で既存のものを置き換える。
         * GUID を引き継ぐため、GameWindow::TryReplaceGameObject（同じ GUID だと Scene の追加と削除が打ち消し合う）は使わない。
         */
        static void ReplaceGameObject(const FoundGameObject& target, const GameObjectPtr& replacement)
        {
            const GameObjectPtr previous = target.gameObject;
            const GameObjectPtr parent   = previous->Transform().GetParent();

            std::size_t siblingIndex = 0;
            if (parent)
            {
                const auto siblings = parent->Transform().GetChildren();
                siblingIndex = static_cast<std::size_t>(std::distance(siblings.begin(), std::ranges::find(siblings, previous)));
            }

            std::vector<PopupWindow::InspectorWindow*> inspectors;
            for (auto* inspector : AutoMcpEngineAccess::ExistingInspectorWindows(ApplicationBase::PopupWindows()))
            {
                if (inspector->DisplayObject().lock() == previous)
                    inspectors.push_back(inspector);
            }

            previous->ImplementDestroy();

            replacement->InitGameObject(parent ? std::weak_ptr<IGameObject>(parent) : std::weak_ptr<IGameObject>(), replacement);

            // worldMatrix_ は JSON に保存された値のままなので、ローカル値と親から計算し直させる
            const glm::vec3 localPosition = replacement->Transform().GetLocalPos();
            replacement->Transform().SetLocalPos(localPosition);

            if (parent)
            {
                replacement->Transform().SetParent(parent, siblingIndex, true);
                ApplicationBase::ApplicationLifeCycle().OnUpdateFieldInittables();
            }
            else
            {
                target.scene->AddGameObject(replacement);
            }

            for (auto* inspector : inspectors)
                inspector->TryAddDisplayObject(replacement);
        }

        static void CommandPing(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            result.AddMember("engine",   MakeString("NanamiEngine", allocator), allocator);
            result.AddMember("protocol", 1, allocator);
        }

        static void CommandStatus(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            AddPlayState(result, *gameWindow, allocator);
            result.AddMember("timeScale", static_cast<double>(NanamiEngine::Time::GetTimeScale()), allocator);
            result.AddMember("fps",       static_cast<double>(GetFPS()), allocator);

            int width  = 0;
            int height = 0;
            GetDrawScreenSize(&width, &height);
            JsonValue screen(rapidjson::kObjectType);
            screen.AddMember("width",  width,  allocator);
            screen.AddMember("height", height, allocator);
            result.AddMember("screen", screen, allocator);

            result.AddMember("currentMainWindow", MakeString(CurrentMainWindowName(), allocator), allocator);

            const auto mainScene = AutoMcpEngineAccess::MainScene(*gameWindow);
            if (mainScene)
                result.AddMember("mainScene", DescribeScene(mainScene, mainScene, allocator), allocator);
            else
                result.AddMember("mainScene", JsonValue(rapidjson::kNullType), allocator);

            result.AddMember("sceneCount",           static_cast<int>(AutoMcpEngineAccess::Scenes(*gameWindow).size()), allocator);
            result.AddMember("loadingResourceCount", NanamiEngine::Module::Asset::Asset::GetLoadingResourceCount(), allocator);

            std::error_code error;
            const std::u8string workingDirectory = std::filesystem::current_path(error).u8string();
            result.AddMember("workingDirectory", MakeString(std::string(workingDirectory.begin(), workingDirectory.end()), allocator), allocator);
        }

        static void CommandWindowsList(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const bool includeHidden = args.OptionalBool("includeHidden", false);

            JsonValue popups(rapidjson::kArrayType);
            AutoMcpEngineAccess::ForEachPopupWindow(ApplicationBase::PopupWindows(), [&popups, &allocator](const ::Guid& guid, PopupWindow::IPopupWindow& window)
            {
                JsonValue popup(rapidjson::kObjectType);
                popup.AddMember("type", MakeString(ShortTypeName(typeid(window).name()), allocator), allocator);
                popup.AddMember("guid", MakeString(guid.Value(), allocator), allocator);
                popups.PushBack(popup, allocator);
            });
            result.AddMember("popups",            popups, allocator);
            result.AddMember("popupTypes",        SortedKeys(PopupWindow::PopupWindowFactory::Instance().GetAll(), allocator), allocator);
            result.AddMember("mainWindows",       SortedKeys(MainWindow::MainWindowFactory::Instance().GetLoaders(), allocator), allocator);
            result.AddMember("currentMainWindow", MakeString(CurrentMainWindowName(), allocator), allocator);

            JsonValue imguiWindows(rapidjson::kArrayType);
            if (const ImGuiContext* context = ImGui::GetCurrentContext())
            {
                for (const ImGuiWindow* window : context->Windows)
                {
                    if (window == nullptr || (window->Flags & ImGuiWindowFlags_ChildWindow) != 0)
                        continue;

                    if (!includeHidden && !window->Active)
                        continue;

                    JsonValue value(rapidjson::kObjectType);
                    value.AddMember("name",      MakeString(window->Name, allocator), allocator);
                    value.AddMember("position",  MakeVec2(window->Pos, allocator), allocator);
                    value.AddMember("size",      MakeVec2(window->Size, allocator), allocator);
                    value.AddMember("collapsed", window->Collapsed, allocator);
                    value.AddMember("visible",   window->Active, allocator);
                    value.AddMember("focused",   context->NavWindow == window, allocator);
                    imguiWindows.PushBack(value, allocator);
                }
            }
            result.AddMember("imguiWindows", imguiWindows, allocator);
        }

        static void CommandWindowsOpen(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string type = args.RequireString("type");
            const auto& factories = PopupWindow::PopupWindowFactory::Instance().GetAll();
            const auto it = factories.find(type);
            if (it == factories.end())
                throw AutoMcpError("unknown popup window type: " + type + " (see popupTypes in windows.list)");

            auto window = it->second();
            const std::string guid = window->Guid().Value();
            ApplicationBase::PopupWindows().InjectWindow(std::move(window));

            result.AddMember("type", MakeString(type, allocator), allocator);
            result.AddMember("guid", MakeString(guid, allocator), allocator);
        }

        static void CommandWindowsClose(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string guid = args.RequireString("guid");
            if (!AutoMcpEngineAccess::ClosePopupWindow(ApplicationBase::PopupWindows(), ::Guid(guid)))
                throw AutoMcpError("popup window not found: " + guid);

            result.AddMember("closed", MakeString(guid, allocator), allocator);
        }

        static void CommandWindowsSet(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string name = args.RequireString("name");
            if (ImGui::FindWindowByName(name.c_str()) == nullptr)
                throw AutoMcpError("ImGui window not found: " + name + " (use the exact name from windows.list, including ##id)");

            ImVec2 vector;
            if (TryGetVec2(args, "position", vector))
                ImGui::SetWindowPos(name.c_str(), vector, ImGuiCond_Always);
            if (TryGetVec2(args, "size", vector))
                ImGui::SetWindowSize(name.c_str(), vector, ImGuiCond_Always);
            if (args.FindMember("collapsed") != nullptr)
                ImGui::SetWindowCollapsed(name.c_str(), args.RequireBool("collapsed"), ImGuiCond_Always);
            if (args.OptionalBool("focus", false))
                ImGui::SetWindowFocus(name.c_str());

            result.AddMember("name", MakeString(name, allocator), allocator);
        }

        static void CommandMainWindowSwitch(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string name = args.RequireString("name");
            const auto& loaders = MainWindow::MainWindowFactory::Instance().GetLoaders();
            const auto it = loaders.find(name);
            if (it == loaders.end())
                throw AutoMcpError("unknown main window: " + name + " (see mainWindows in windows.list)");

            ApplicationBase::OnChangeWindow(it->second());
            result.AddMember("currentMainWindow", MakeString(name, allocator), allocator);
        }

        static void CommandSceneList(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            const auto mainScene  = AutoMcpEngineAccess::MainScene(*gameWindow);

            JsonValue scenes(rapidjson::kArrayType);
            for (const auto& scene : AutoMcpEngineAccess::Scenes(*gameWindow))
                scenes.PushBack(DescribeScene(scene, mainScene, allocator), allocator);

            result.AddMember("scenes", scenes, allocator);
        }

        static void CommandSceneLoad(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string path     = args.RequireString("path");
            const bool        makeMain = args.OptionalBool("makeMain", true);

            const std::filesystem::path filePath(std::u8string(path.begin(), path.end()));
            std::error_code error;
            if (!std::filesystem::is_regular_file(filePath, error))
                throw AutoMcpError("scene file not found (paths are relative to workingDirectory in status): " + path);

            const auto gameWindow = ActivateGameWindow(result, allocator);
            const auto scene = std::make_shared<NanamiEngine::Scene::Scene>(filePath.string());
            gameWindow->AddContent(scene);
            if (makeMain)
                gameWindow->ChangeMainScene(scene);

            result.AddMember("scene", DescribeScene(scene, AutoMcpEngineAccess::MainScene(*gameWindow), allocator), allocator);
        }

        static void CommandSceneReload(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            const auto mainScene  = AutoMcpEngineAccess::MainScene(*gameWindow);
            const std::string guid = args.OptionalString("guid", mainScene ? mainScene->GetGuid().Value() : std::string());
            if (guid.empty())
                throw AutoMcpError("no main scene; pass guid");

            ActivateGameWindow(result, allocator);
            const auto scene = AutoMcpEngineAccess::ReloadScene(*gameWindow, ::Guid(guid));
            if (!scene)
                throw AutoMcpError("scene is not loaded: " + guid);

            result.AddMember("scene", DescribeScene(scene, AutoMcpEngineAccess::MainScene(*gameWindow), allocator), allocator);
        }

        static void CommandHierarchy(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string sceneGuid         = args.OptionalString("sceneGuid", std::string());
            const int         maxDepth          = args.OptionalInt("maxDepth", -1);
            const bool        includeComponents = args.OptionalBool("includeComponents", true);

            const auto gameWindow = RequireGameWindow();
            const auto mainScene  = AutoMcpEngineAccess::MainScene(*gameWindow);

            JsonValue scenes(rapidjson::kArrayType);
            for (const auto& scene : AutoMcpEngineAccess::Scenes(*gameWindow))
            {
                if (!sceneGuid.empty() && scene->GetGuid().Value() != sceneGuid)
                    continue;

                JsonValue sceneValue = DescribeScene(scene, mainScene, allocator);
                JsonValue roots(rapidjson::kArrayType);
                for (const auto& root : RootGameObjects(scene))
                    roots.PushBack(DescribeHierarchy(root, 0, maxDepth, includeComponents, allocator), allocator);

                sceneValue.AddMember("roots", roots, allocator);
                scenes.PushBack(sceneValue, allocator);
            }

            if (!sceneGuid.empty() && scenes.Empty())
                throw AutoMcpError("scene is not loaded: " + sceneGuid);

            result.AddMember("scenes", scenes, allocator);
        }

        static void CommandGameObjectFind(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string query      = args.RequireString("name");
            const bool        exact      = args.OptionalBool("exact", false);
            const int         limit      = (std::max)(1, args.OptionalInt("limit", 50));
            const std::string lowerQuery = ToLowerAscii(query);

            JsonValue matches(rapidjson::kArrayType);
            bool isTruncated = false;
            for (const auto& scene : AutoMcpEngineAccess::Scenes(*RequireGameWindow()))
            {
                scene->ForEachGameObject([&](const GameObjectPtr& gameObject)
                {
                    if (!gameObject)
                        return;

                    const bool isMatch = exact ? gameObject->Name() == query : ToLowerAscii(gameObject->Name()).find(lowerQuery) != std::string::npos;
                    if (!isMatch)
                        return;

                    if (static_cast<int>(matches.Size()) >= limit)
                    {
                        isTruncated = true;
                        return;
                    }

                    JsonValue match(rapidjson::kObjectType);
                    match.AddMember("name",      MakeString(gameObject->Name(), allocator), allocator);
                    match.AddMember("guid",      MakeString(gameObject->GetGuid().Value(), allocator), allocator);
                    match.AddMember("path",      MakeString(GameObjectPath(gameObject), allocator), allocator);
                    match.AddMember("sceneGuid", MakeString(scene->GetGuid().Value(), allocator), allocator);
                    matches.PushBack(match, allocator);
                });
            }

            result.AddMember("matches",   matches, allocator);
            result.AddMember("truncated", isTruncated, allocator);
        }

        static void CommandGameObjectGet(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);
            IGameObject& gameObject = *found.gameObject;

            AddGameObjectHeader(result, gameObject, allocator);
            result.AddMember("path",      MakeString(GameObjectPath(found.gameObject), allocator), allocator);
            result.AddMember("sceneGuid", MakeString(found.scene->GetGuid().Value(), allocator), allocator);

            if (const auto parent = gameObject.Transform().GetParent())
                result.AddMember("parentGuid", MakeString(parent->GetGuid().Value(), allocator), allocator);
            else
                result.AddMember("parentGuid", JsonValue(rapidjson::kNullType), allocator);

            result.AddMember("transform",  DescribeTransform(gameObject.Transform(), allocator), allocator);
            result.AddMember("components", DescribeComponents(gameObject, allocator), allocator);

            JsonValue children(rapidjson::kArrayType);
            for (const auto& child : gameObject.Transform().GetChildren())
            {
                if (!child)
                    continue;

                JsonValue childValue(rapidjson::kObjectType);
                childValue.AddMember("name", MakeString(child->Name(), allocator), allocator);
                childValue.AddMember("guid", MakeString(child->GetGuid().Value(), allocator), allocator);
                children.PushBack(childValue, allocator);
            }
            result.AddMember("children", children, allocator);
        }

        static void CommandGameObjectGetJson(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);

            std::ostringstream stream;
            {
                cereal::JSONOutputArchive archive(stream);
                GameObjectPtr gameObject = found.gameObject;
                archive(cereal::make_nvp("gameObject", gameObject));
            }

            result.AddMember("guid", MakeString(found.gameObject->GetGuid().Value(), allocator), allocator);
            result.AddMember("json", MakeString(stream.str(), allocator), allocator);
        }

        static void CommandGameObjectSetJson(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found    = RequireGameObject(args);
            const std::string     jsonText = args.RequireString("json");

            GameObjectPtr replacement;
            {
                std::istringstream stream(jsonText);
                cereal::JSONInputArchive archive(stream);
                archive(cereal::make_nvp("gameObject", replacement));
            }

            if (!replacement)
                throw AutoMcpError("json did not contain a GameObject under \"gameObject\"");

            if (!(replacement->GetGuid() == found.gameObject->GetGuid()))
                throw AutoMcpError("json guid " + replacement->GetGuid().Value() + " does not match the target " + found.gameObject->GetGuid().Value() + "; keep the guids from gameobject.get_json");

            ActivateGameWindow(result, allocator);
            ReplaceGameObject(found, replacement);

            AddGameObjectHeader(result, *replacement, allocator);
            result.AddMember("components", DescribeComponents(*replacement, allocator), allocator);
        }

        static void CommandGameObjectSetTransform(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);
            auto& transform = found.gameObject->Transform();

            glm::vec3 vector;
            glm::quat rotation;
            if (args.TryGetVec3("localScale", vector))
                transform.SetLocalScale(vector);
            if (args.TryGetQuat("localRotation", rotation))
                transform.SetLocalRot(rotation);
            if (args.TryGetVec3("localEulerDegrees", vector))
                transform.SetLocalRot(glm::quat(glm::radians(vector)));
            if (args.TryGetVec3("localPosition", vector))
                transform.SetLocalPos(vector);
            if (args.TryGetVec3("worldEulerDegrees", vector))
                transform.SetWorldRot(glm::quat(glm::radians(vector)));
            if (args.TryGetVec3("worldPosition", vector))
                transform.SetWorldPos(vector);

            result.AddMember("transform", DescribeTransform(transform, allocator), allocator);
        }

        static void CommandGameObjectSetEnable(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);
            found.gameObject->SetEnable(args.RequireBool("enable"));
            result.AddMember("components", DescribeComponents(*found.gameObject, allocator), allocator);
        }

        static void CommandComponentSetEnable(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);
            const ComponentPtr component = FindComponent(*found.gameObject, args.RequireString("componentGuid"));
            component->SetEnable(args.RequireBool("enable"));
            result.AddMember("component", DescribeComponent(*component, allocator), allocator);
        }

        static void CommandGameObjectSelect(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);

            int inspectorCount = 0;
            for (auto* inspector : ApplicationBase::PopupWindows().Catch<PopupWindow::InspectorWindow>())
            {
                inspector->TryAddDisplayObject(found.gameObject);
                ++inspectorCount;
            }
            result.AddMember("inspectorCount", inspectorCount, allocator);
        }

        static void CommandGameObjectDestroy(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const FoundGameObject found = RequireGameObject(args);
            RequireGameWindow()->RemoveGameObject(found.gameObject);
            result.AddMember("destroyed", MakeString(found.gameObject->GetGuid().Value(), allocator), allocator);
        }

        static void CommandPlay(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            AutoMcpEngineAccess::Play(*gameWindow);
            AddPlayState(result, *gameWindow, allocator);
        }

        static void CommandStop(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            AutoMcpEngineAccess::Stop(*gameWindow);
            AddPlayState(result, *gameWindow, allocator);
        }

        static void CommandEnd(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = ActivateGameWindow(result, allocator);
            AutoMcpEngineAccess::End(*gameWindow);
            AddPlayState(result, *gameWindow, allocator);
        }

        static void AddHotReloadState(JsonValue& result, JsonAllocator& allocator)
        {
            const auto& gameModule = HotReload::GameModule::Instance();
            result.AddMember("loaded",         gameModule.IsLoaded(),       allocator);
            result.AddMember("generation",     gameModule.Generation(),     allocator);
            result.AddMember("keepOldModules", gameModule.KeepOldModules(), allocator);
            const std::u8string source = gameModule.SourcePath().u8string();
            result.AddMember("source",         MakeString(std::string(source.begin(), source.end()), allocator), allocator);
            result.AddMember("lastReport",     MakeString(gameModule.LastReport(), allocator), allocator);
        }

        static void CommandHotReloadStatus(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            AddHotReloadState(result, allocator);
        }

        // NOTE: 差し替え自体は ApplicationBase::Run の ScreenFlip 後
        static void CommandHotReloadReload(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            auto& gameModule = HotReload::GameModule::Instance();
            if (!gameModule.IsLoaded())
            {
                throw AutoMcpError("game module is not loaded (static build?)");
            }
            if (args.FindMember("keepOldModules") != nullptr)
            {
                gameModule.SetKeepOldModules(args.RequireBool("keepOldModules"));
            }
            gameModule.RequestReload();
            result.AddMember("requested", true, allocator);
            AddHotReloadState(result, allocator);
        }

        static void CommandTimeSetScale(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const double scale = args.RequireNumber("scale");
            if (scale < 0.0)
                throw AutoMcpError("scale must be >= 0");

            NanamiEngine::Time::SetTimeScale(static_cast<float>(scale));
            result.AddMember("timeScale", static_cast<double>(NanamiEngine::Time::GetTimeScale()), allocator);
        }

        static void CommandCameraGet(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            result.AddMember("controller", MakeString(gameWindow->IsPlayMode() ? "cinemachine" : "editor", allocator), allocator);

            const glm::quat rotation = gameWindow->GetCameraRotation();
            JsonValue editor(rapidjson::kObjectType);
            editor.AddMember("position",     MakeVec3(gameWindow->GetCameraPosition(), allocator), allocator);
            editor.AddMember("rotation",     MakeQuat(rotation, allocator), allocator);
            editor.AddMember("eulerDegrees", MakeVec3(glm::degrees(glm::eulerAngles(rotation)), allocator), allocator);
            editor.AddMember("forward",      MakeVec3(rotation * glm::vec3(0.0f, 0.0f, 1.0f), allocator), allocator);
            result.AddMember("editorCamera", editor, allocator);

            const VECTOR position = DxLib::GetCameraPosition();
            const VECTOR target   = DxLib::GetCameraTarget();
            JsonValue rendered(rapidjson::kObjectType);
            rendered.AddMember("position", MakeVec3(glm::vec3(position.x, position.y, position.z), allocator), allocator);
            rendered.AddMember("target",   MakeVec3(glm::vec3(target.x, target.y, target.z), allocator), allocator);
            result.AddMember("renderedCamera", rendered, allocator);
        }

        static void CommandCameraSet(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const auto gameWindow = RequireGameWindow();
            if (gameWindow->IsPlayMode())
                throw AutoMcpError("camera.set only works in edit mode; Cinemachine drives the camera while playing");

            glm::vec3 position = gameWindow->GetCameraPosition();
            glm::vec3 vector;
            glm::quat rotation;
            if (args.TryGetVec3("position", vector))
            {
                position = vector;
                gameWindow->SetCameraPosition(position);
            }
            if (args.TryGetQuat("rotation", rotation))
                gameWindow->SetCameraRotation(rotation);
            if (args.TryGetVec3("eulerDegrees", vector))
                gameWindow->SetCameraRotation(glm::quat(glm::radians(vector)));
            if (args.TryGetVec3("lookAt", vector))
            {
                const glm::vec3 offset = vector - position;
                if (glm::length(offset) < 1.0e-4f)
                    throw AutoMcpError("lookAt is at the camera position");

                const glm::vec3 direction = glm::normalize(offset);
                const glm::vec3 up = std::abs(direction.y) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                // Editor3DCamera の前方は +Z
                gameWindow->SetCameraRotation(glm::quatLookAtLH(direction, up));
            }

            CommandCameraGet(args, result, allocator);
        }

        using DebugDrawChanges = std::vector<std::pair<bool*, bool>>;

        static std::span<const char* const> PhysicsLayerNames()
        {
            return { NanamiEngine::Module::Physics::PhysicsLayers::Names(), static_cast<std::size_t>(NanamiEngine::Module::Physics::PhysicsLayers::Count()) };
        }

        template <typename EnumT>
        static JsonValue DescribeDebugDrawFlags(const std::span<const char* const> names, bool& (*flag)(EnumT), JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            for (std::size_t i = 0; i < names.size(); ++i)
                value.AddMember(rapidjson::StringRef(names[i]), flag(static_cast<EnumT>(i)), allocator);
            return value;
        }

        static void DescribeDebugDraw(JsonValue& result, JsonAllocator& allocator)
        {
            result.AddMember("colliders",             AutoMcpEngineAccess::DebugDrawAllColliders(), allocator);
            result.AddMember("shapes",                DescribeDebugDrawFlags(NanamiEngine::Module::Physics::COLLIDER_SHAPE_KIND_NAMES, &AutoMcpEngineAccess::DebugDrawColliderKind, allocator), allocator);
            result.AddMember("layers",                DescribeDebugDrawFlags(PhysicsLayerNames(), &AutoMcpEngineAccess::DebugDrawColliderLayer, allocator), allocator);
            result.AddMember("triggers",              AutoMcpEngineAccess::DebugDrawTriggerColliders(), allocator);
            result.AddMember("mainCameraFrustum",     AutoMcpEngineAccess::DebugDrawMainCameraFrustum(), allocator);
            result.AddMember("virtualCameraFrustums", AutoMcpEngineAccess::DebugDrawVirtualCameraFrustums(), allocator);
        }

        static void CollectDebugDrawFlag(const JsonArgs& args, const char* key, bool& flag, DebugDrawChanges& changes)
        {
            if (args.FindMember(key) != nullptr)
                changes.emplace_back(&flag, args.RequireBool(key));
        }

        /** @brief true/false なら全部、{"名前": bool} なら名前ごと (大文字小文字は区別しない) に切り替える */
        template <typename EnumT>
        static void CollectDebugDrawFlags(const JsonArgs& args, const char* key, const std::span<const char* const> names, bool& (*flag)(EnumT), DebugDrawChanges& changes)
        {
            const JsonValue* member = args.FindMember(key);
            if (member == nullptr)
                return;

            if (member->IsBool())
            {
                for (std::size_t i = 0; i < names.size(); ++i)
                    changes.emplace_back(&flag(static_cast<EnumT>(i)), member->GetBool());
                return;
            }

            std::string validNames;
            for (const char* name : names)
                validNames += validNames.empty() ? name : std::string(", ") + name;

            if (!member->IsObject())
                throw AutoMcpError(std::string(key) + " must be true/false (all) or {\"<name>\": bool} with names from: " + validNames);

            for (auto it = member->MemberBegin(); it != member->MemberEnd(); ++it)
            {
                const std::string requested = it->name.GetString();
                const auto match = std::ranges::find_if(names, [&requested](const char* name) { return ToLowerAscii(name) == ToLowerAscii(requested); });
                if (match == std::ranges::end(names))
                    throw AutoMcpError(std::string("unknown ") + key + " name: " + requested + " (valid: " + validNames + ")");
                if (!it->value.IsBool())
                    throw AutoMcpError(std::string(key) + "." + requested + " must be true or false");

                changes.emplace_back(&flag(static_cast<EnumT>(match - std::ranges::begin(names))), it->value.GetBool());
            }
        }

        static void CommandDebugDrawGet(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            DescribeDebugDraw(result, allocator);
        }

        /** @brief 既定では ProjectConfig/DebugDraw に保存しない (git 管理下なので)。save で Config 画面の変更と同じく保存する */
        static void CommandDebugDrawSet(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            // 途中の引数が不正でも一部だけ反映されないよう、全部読んでから書き込む
            DebugDrawChanges changes;
            CollectDebugDrawFlag (args, "colliders", AutoMcpEngineAccess::DebugDrawAllColliders(), changes);
            CollectDebugDrawFlags(args, "shapes",    NanamiEngine::Module::Physics::COLLIDER_SHAPE_KIND_NAMES, &AutoMcpEngineAccess::DebugDrawColliderKind, changes);
            CollectDebugDrawFlags(args, "layers",    PhysicsLayerNames(), &AutoMcpEngineAccess::DebugDrawColliderLayer, changes);
            CollectDebugDrawFlag (args, "triggers",  AutoMcpEngineAccess::DebugDrawTriggerColliders(), changes);
            CollectDebugDrawFlag (args, "mainCameraFrustum",     AutoMcpEngineAccess::DebugDrawMainCameraFrustum(), changes);
            CollectDebugDrawFlag (args, "virtualCameraFrustums", AutoMcpEngineAccess::DebugDrawVirtualCameraFrustums(), changes);
            const bool save = args.OptionalBool("save", false);

            for (const auto& [flag, value] : changes)
                *flag = value;

            if (save)
                Configuration::DebugDrawConfiguration::Save();

            DescribeDebugDraw(result, allocator);
            result.AddMember("saved", save, allocator);
        }

        static const char* ToLevelName(const NanamiEngine::Module::LogLevel level)
        {
            switch (level)
            {
            case NanamiEngine::Module::LogLevel::Warning: return "warning";
            case NanamiEngine::Module::LogLevel::Error:   return "error";
            default:                                      return "info";
            }
        }

        static void CommandLogTail(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::size_t count    = static_cast<std::size_t>(std::clamp(args.OptionalInt("count", 50), 1, 2000));
            const std::string minLevel = args.OptionalString("minLevel", "info");
            const std::string contains = args.OptionalString("contains", std::string());

            NanamiEngine::Module::LogLevel threshold = NanamiEngine::Module::LogLevel::Info;
            if (minLevel == "warning")
                threshold = NanamiEngine::Module::LogLevel::Warning;
            else if (minLevel == "error")
                threshold = NanamiEngine::Module::LogLevel::Error;
            else if (minLevel != "info")
                throw AutoMcpError("minLevel must be info, warning or error");

            const auto history = NanamiEngine::Module::LogHistory();
            std::vector<std::pair<NanamiEngine::Module::LogLevel, std::string>> selected;
            for (auto it = history.rbegin(); it != history.rend() && selected.size() < count; ++it)
            {
                if (static_cast<int>(it->level) < static_cast<int>(threshold))
                    continue;

                std::string text = ToUtf8(it->text);
                if (!contains.empty() && text.find(contains) == std::string::npos)
                    continue;

                selected.emplace_back(it->level, std::move(text));
            }

            JsonValue records(rapidjson::kArrayType);
            for (auto it = selected.rbegin(); it != selected.rend(); ++it)
            {
                JsonValue record(rapidjson::kObjectType);
                record.AddMember("level", MakeString(ToLevelName(it->first), allocator), allocator);
                record.AddMember("text",  MakeString(it->second, allocator), allocator);
                records.PushBack(record, allocator);
            }

            result.AddMember("records",        records, allocator);
            result.AddMember("historyCount",   static_cast<int>(history.size()), allocator);
        }

        static void AddPreviewCamera(const MainWindow::ModelPreviewStage& stage, JsonValue& result, JsonAllocator& allocator)
        {
            const glm::quat rotation = AutoMcpEngineAccess::PreviewCameraRotation(stage);
            JsonValue camera(rapidjson::kObjectType);
            camera.AddMember("position", MakeVec3(AutoMcpEngineAccess::PreviewCameraPosition(stage), allocator), allocator);
            camera.AddMember("forward",  MakeVec3(rotation * glm::vec3(0.0f, 0.0f, 1.0f), allocator), allocator);
            result.AddMember("camera", camera, allocator);
        }

        static void DescribeModelViewState(MainWindow::ModelViewWindow& window, JsonValue& result, JsonAllocator& allocator)
        {
            const auto selectedGuid = AutoMcpEngineAccess::ModelViewSelectedGuid(window);

            JsonValue models(rapidjson::kArrayType);
            for (const auto& file : AutoMcpEngineAccess::ModelViewContents(window))
            {
                JsonValue model = DescribeAsset(*file, allocator);
                model.AddMember("selected", selectedGuid.has_value() && *selectedGuid == file->GetGuid(), allocator);
                models.PushBack(model, allocator);
            }

            auto& stage = AutoMcpEngineAccess::ModelViewStage(window);
            result.AddMember("isCurrentMainWindow",  ApplicationBase::GetMainWindow().get() == &window, allocator);
            result.AddMember("models",               models, allocator);
            result.AddMember("modelReady",           IsDxHandleReady(stage.ModelHandle()), allocator);
            result.AddMember("loadingResourceCount", NanamiEngine::Module::Asset::Asset::GetLoadingResourceCount(), allocator);
            AddPreviewCamera(stage, result, allocator);
        }

        static JsonValue DescribeAnimationSlot(MainWindow::AnimationPreviewSlot& slot, const int modelHandle, const bool includeClips, JsonAllocator& allocator)
        {
            JsonValue value(rapidjson::kObjectType);
            if (const auto file = AutoMcpEngineAccess::SlotAnimationFile(slot))
                value.AddMember("animation", DescribeAsset(*file, allocator), allocator);
            else
                value.AddMember("animation", JsonValue(rapidjson::kNullType), allocator);

            const bool isSourceReady = IsDxHandleReady(modelHandle) && AutoMcpEngineAccess::IsSlotSourceReady(slot);
            const int  sourceHandle  = isSourceReady ? AutoMcpEngineAccess::SlotClipSourceHandle(slot, modelHandle) : -1;
            const int  clipCount     = sourceHandle != -1 ? MV1GetAnimNum(sourceHandle) : 0;
            const int  clipIndex     = AutoMcpEngineAccess::SlotClipIndex(slot);
            const std::string clipName = clipIndex >= 0 && clipIndex < clipCount ? DxLibName(MV1GetAnimName(sourceHandle, clipIndex)) : std::string();

            value.AddMember("sourceReady", sourceHandle != -1, allocator);
            value.AddMember("attached",    AutoMcpEngineAccess::IsSlotAttached(slot), allocator);
            value.AddMember("clipIndex",   clipIndex, allocator);
            value.AddMember("clipName",    MakeString(clipName, allocator), allocator);
            value.AddMember("clipCount",   clipCount, allocator);
            value.AddMember("time",        static_cast<double>(slot.GetTime()), allocator);
            value.AddMember("totalTime",   static_cast<double>(AutoMcpEngineAccess::SlotTotalTime(slot)), allocator);
            value.AddMember("speed",       static_cast<double>(AutoMcpEngineAccess::SlotSpeed(slot)), allocator);
            value.AddMember("loop",        AutoMcpEngineAccess::SlotLoop(slot), allocator);
            value.AddMember("start",       static_cast<double>(AutoMcpEngineAccess::SlotStartTime(slot)), allocator);
            value.AddMember("end",         static_cast<double>(AutoMcpEngineAccess::SlotEndTime(slot)), allocator);

            if (includeClips && sourceHandle != -1)
            {
                JsonValue clips(rapidjson::kArrayType);
                for (int i = 0; i < clipCount; ++i)
                {
                    JsonValue clip(rapidjson::kObjectType);
                    clip.AddMember("index",     i, allocator);
                    clip.AddMember("name",      MakeString(DxLibName(MV1GetAnimName(sourceHandle, i)), allocator), allocator);
                    clip.AddMember("totalTime", static_cast<double>(MV1GetAnimTotalTime(sourceHandle, i)), allocator);
                    clips.PushBack(clip, allocator);
                }
                value.AddMember("clips", clips, allocator);
            }
            return value;
        }

        static void DescribeAnimationViewState(MainWindow::AnimationViewWindow& window, const bool includeClips, JsonValue& result, JsonAllocator& allocator)
        {
            auto&     stage       = AutoMcpEngineAccess::AnimationViewStage(window);
            const int modelHandle = stage.ModelHandle();
            const bool isModelReady = IsDxHandleReady(modelHandle);

            result.AddMember("isCurrentMainWindow", ApplicationBase::GetMainWindow().get() == &window, allocator);
            if (const auto model = AutoMcpEngineAccess::AnimationViewModel(window))
                result.AddMember("model", DescribeAsset(*model, allocator), allocator);
            else
                result.AddMember("model", JsonValue(rapidjson::kNullType), allocator);

            result.AddMember("modelReady",     isModelReady, allocator);
            result.AddMember("playing",        AutoMcpEngineAccess::AnimationViewPlaying(window), allocator);
            result.AddMember("useBlend",       AutoMcpEngineAccess::AnimationViewUseBlend(window), allocator);
            result.AddMember("blendWeight",    static_cast<double>(AutoMcpEngineAccess::AnimationViewBlendWeight(window)), allocator);
            result.AddMember("nameCheck",      AutoMcpEngineAccess::AnimationViewNameCheck(window), allocator);
            result.AddMember("lockRootMotion", AutoMcpEngineAccess::AnimationViewLockRootMotion(window), allocator);

            const int rootFrameIndex = AutoMcpEngineAccess::AnimationViewRootFrameIndex(window);
            JsonValue rootFrame(rapidjson::kObjectType);
            rootFrame.AddMember("index", rootFrameIndex, allocator);
            const bool hasRootFrame = isModelReady && rootFrameIndex >= 0 && rootFrameIndex < MV1GetFrameNum(modelHandle);
            rootFrame.AddMember("name", MakeString(hasRootFrame ? DxLibName(MV1GetFrameName(modelHandle, rootFrameIndex)) : std::string(), allocator), allocator);
            result.AddMember("rootFrame", rootFrame, allocator);

            JsonValue slots(rapidjson::kObjectType);
            slots.AddMember("A", DescribeAnimationSlot(AutoMcpEngineAccess::AnimationViewSlot(window, false), modelHandle, includeClips, allocator), allocator);
            slots.AddMember("B", DescribeAnimationSlot(AutoMcpEngineAccess::AnimationViewSlot(window, true),  modelHandle, includeClips, allocator), allocator);
            result.AddMember("slots", slots, allocator);

            AddPreviewCamera(stage, result, allocator);
        }

        static void CommandAssetsFind(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string query     = NormalizeAssetPath(args.OptionalString("query", std::string()));
            const std::string extension = ToLowerAscii(args.OptionalString("extension", std::string()));
            const int         limit     = std::clamp(args.OptionalInt("limit", 50), 1, 1000);

            JsonValue assets(rapidjson::kArrayType);
            bool isTruncated = false;
            for (const auto& asset : AllAssets())
            {
                const std::string path = NormalizeAssetPath(ToUtf8(asset->GetContentPath()));
                if (!extension.empty() && !path.ends_with(extension))
                    continue;
                if (!query.empty() && path.find(query) == std::string::npos)
                    continue;

                if (static_cast<int>(assets.Size()) >= limit)
                {
                    isTruncated = true;
                    break;
                }
                assets.PushBack(DescribeAsset(*asset, allocator), allocator);
            }

            result.AddMember("assets",    assets, allocator);
            result.AddMember("truncated", isTruncated, allocator);
        }

        /** @brief 再読み込みしたアセットの非同期ロードは次フレームの OnEnableAsset から始まるので、ここでは loadingResourceCount を返さない */
        static void CommandAssetsReload(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            const int previousAssetCount = static_cast<int>(AllAssets().size());
            ApplicationBase::ResetAssetsDirectory();

            result.AddMember("previousAssetCount", previousAssetCount, allocator);
            result.AddMember("assetCount",         static_cast<int>(AllAssets().size()), allocator);
        }

        static void CommandModelViewOpen(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const Mv1FilePtr file   = ResolveMv1(args, "guid", "path");
            const auto       window = ApplicationBase::MainWindows().Catch<MainWindow::ModelViewWindow>();
            // ComponentGroup::Add<T> はカレント MainWindow の LifeCycle に登録するため、AddContent より先に切り替える (Mv1File::OnDoubleClick と同じ)
            ApplicationBase::OnChangeWindow(window);
            window->AddContent(file);
            DescribeModelViewState(*window, result, allocator);
        }

        static void CommandModelViewState(const JsonArgs&, JsonValue& result, JsonAllocator& allocator)
        {
            DescribeModelViewState(*ApplicationBase::MainWindows().Catch<MainWindow::ModelViewWindow>(), result, allocator);
        }

        static void CommandModelViewSelect(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const Mv1FilePtr file   = ResolveMv1(args, "guid", "path");
            const auto       window = ApplicationBase::MainWindows().Catch<MainWindow::ModelViewWindow>();
            const auto       models = AutoMcpEngineAccess::ModelViewContents(*window);
            if (std::ranges::none_of(models, [&file](const Mv1FilePtr& model) { return model->GetGuid() == file->GetGuid(); }))
                throw AutoMcpError(DisplayAssetPath(*file) + " is not open in ModelView; use modelview.open");

            window->Select(file->GetGuid());
            DescribeModelViewState(*window, result, allocator);
        }

        static void CommandModelViewClose(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const std::string guid   = args.RequireString("guid");
            const auto        window = ApplicationBase::MainWindows().Catch<MainWindow::ModelViewWindow>();
            const auto        models = AutoMcpEngineAccess::ModelViewContents(*window);
            if (std::ranges::none_of(models, [&guid](const Mv1FilePtr& model) { return model->GetGuid() == ::Guid(guid); }))
                throw AutoMcpError("model is not open in ModelView: " + guid);

            AutoMcpEngineAccess::ModelViewClose(*window, ::Guid(guid));
            DescribeModelViewState(*window, result, allocator);
        }

        static void CommandAnimationViewOpen(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const Mv1FilePtr file   = ResolveMv1(args, "modelGuid", "modelPath");
            const auto       window = ApplicationBase::MainWindows().Catch<MainWindow::AnimationViewWindow>();
            ApplicationBase::OnChangeWindow(window);
            window->AddContent(file);
            DescribeAnimationViewState(*window, false, result, allocator);
        }

        static void CommandAnimationViewState(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            DescribeAnimationViewState(*ApplicationBase::MainWindows().Catch<MainWindow::AnimationViewWindow>(), args.OptionalBool("includeClips", false), result, allocator);
        }

        static void CommandAnimationViewSet(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const auto window = ApplicationBase::MainWindows().Catch<MainWindow::AnimationViewWindow>();

            if (args.FindMember("playing") != nullptr)
                AutoMcpEngineAccess::AnimationViewPlaying(*window) = args.RequireBool("playing");
            if (args.FindMember("useBlend") != nullptr)
                AutoMcpEngineAccess::AnimationViewUseBlend(*window) = args.RequireBool("useBlend");
            if (args.FindMember("blendWeight") != nullptr)
                AutoMcpEngineAccess::AnimationViewBlendWeight(*window) = std::clamp(static_cast<float>(args.RequireNumber("blendWeight")), 0.0f, 1.0f);
            if (args.FindMember("nameCheck") != nullptr)
                AutoMcpEngineAccess::AnimationViewNameCheck(*window) = args.RequireBool("nameCheck");
            if (args.FindMember("lockRootMotion") != nullptr)
                AutoMcpEngineAccess::AnimationViewLockRootMotion(*window) = args.RequireBool("lockRootMotion");
            if (args.FindMember("rootFrameIndex") != nullptr)
            {
                const int modelHandle = AutoMcpEngineAccess::AnimationViewStage(*window).ModelHandle();
                const int frameIndex  = args.OptionalInt("rootFrameIndex", -1);
                if (!IsDxHandleReady(modelHandle) || frameIndex < 0 || frameIndex >= MV1GetFrameNum(modelHandle))
                    throw AutoMcpError("rootFrameIndex is out of range or the model is not loaded yet");

                AutoMcpEngineAccess::AnimationViewRootFrameIndex(*window) = frameIndex;
            }

            DescribeAnimationViewState(*window, false, result, allocator);
        }

        static void CommandAnimationViewBones(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const auto window      = ApplicationBase::MainWindows().Catch<MainWindow::AnimationViewWindow>();
            const int  modelHandle = AutoMcpEngineAccess::AnimationViewStage(*window).ModelHandle();
            if (!IsDxHandleReady(modelHandle))
                throw AutoMcpError("the AnimationView model is not loaded yet");

            // NOTE: 部分一致(大文字小文字無視)。空なら全フレーム
            const std::string filter = ToLowerAscii(args.OptionalString("nameContains", std::string()));

            const auto rowOf = [&](const MATRIX& matrix, const int row)
            {
                return MakeVec3(glm::vec3(matrix.m[row][0], matrix.m[row][1], matrix.m[row][2]), allocator);
            };

            JsonValue frames(rapidjson::kArrayType);
            const int frameNum = MV1GetFrameNum(modelHandle);
            for (int frame = 0; frame < frameNum; ++frame)
            {
                const std::string name = DxLibName(MV1GetFrameName(modelHandle, frame));
                if (!filter.empty() && ToLowerAscii(name).find(filter) == std::string::npos)
                    continue;

                const MATRIX world = MV1GetFrameLocalWorldMatrix(modelHandle, frame);
                JsonValue value(rapidjson::kObjectType);
                value.AddMember("index",    frame, allocator);
                value.AddMember("name",     MakeString(name, allocator), allocator);
                value.AddMember("parent",   MV1GetFrameParent(modelHandle, frame), allocator);
                value.AddMember("position", rowOf(world, 3), allocator);
                value.AddMember("axisX",    rowOf(world, 0), allocator);
                value.AddMember("axisY",    rowOf(world, 1), allocator);
                value.AddMember("axisZ",    rowOf(world, 2), allocator);
                frames.PushBack(value, allocator);
            }

            const MATRIX model = MV1GetMatrix(modelHandle);
            JsonValue modelMatrix(rapidjson::kObjectType);
            modelMatrix.AddMember("position", rowOf(model, 3), allocator);
            modelMatrix.AddMember("axisX",    rowOf(model, 0), allocator);
            modelMatrix.AddMember("axisY",    rowOf(model, 1), allocator);
            modelMatrix.AddMember("axisZ",    rowOf(model, 2), allocator);

            result.AddMember("modelMatrix", modelMatrix, allocator);
            result.AddMember("frames",      frames, allocator);
        }

        static void CommandAnimationViewSetClip(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            const auto        window   = ApplicationBase::MainWindows().Catch<MainWindow::AnimationViewWindow>();
            const std::string slotName = args.OptionalString("slot", "A");
            if (slotName != "A" && slotName != "a" && slotName != "B" && slotName != "b")
                throw AutoMcpError("slot must be \"A\" or \"B\"");

            auto& slot = AutoMcpEngineAccess::AnimationViewSlot(*window, slotName == "B" || slotName == "b");

            if (args.OptionalBool("useModelClips", false))
                AutoMcpEngineAccess::SetSlotAnimationFile(slot, nullptr);
            else if (args.FindMember("animationGuid") != nullptr || args.FindMember("animationPath") != nullptr)
                AutoMcpEngineAccess::SetSlotAnimationFile(slot, ResolveMv1(args, "animationGuid", "animationPath"));

            // クリップを選ぶと再生区間と時間がリセットされるので、区間・時間より先に選ぶ
            if (args.FindMember("clipName") != nullptr || args.FindMember("clipIndex") != nullptr)
            {
                const int modelHandle = AutoMcpEngineAccess::AnimationViewStage(*window).ModelHandle();
                const bool isSourceReady = IsDxHandleReady(modelHandle) && AutoMcpEngineAccess::IsSlotSourceReady(slot);
                const int sourceHandle = isSourceReady ? AutoMcpEngineAccess::SlotClipSourceHandle(slot, modelHandle) : -1;

                if (args.FindMember("clipName") != nullptr)
                {
                    if (sourceHandle == -1)
                        throw AutoMcpError("clips are not loaded yet (model or animation source still loading); retry when animationview.state shows sourceReady");

                    AutoMcpEngineAccess::SelectSlotClip(slot, FindClipIndex(sourceHandle, args.RequireString("clipName")));
                }
                else
                {
                    const int clipIndex = args.OptionalInt("clipIndex", 0);
                    if (clipIndex < 0 || (sourceHandle != -1 && clipIndex >= MV1GetAnimNum(sourceHandle)))
                        throw AutoMcpError("clipIndex is out of range");

                    AutoMcpEngineAccess::SelectSlotClip(slot, clipIndex);
                }
            }

            if (args.FindMember("speed") != nullptr)
                AutoMcpEngineAccess::SlotSpeed(slot) = static_cast<float>(args.RequireNumber("speed"));
            if (args.FindMember("loop") != nullptr)
                AutoMcpEngineAccess::SlotLoop(slot) = args.RequireBool("loop");
            if (args.FindMember("start") != nullptr)
                AutoMcpEngineAccess::SlotStartTime(slot) = (std::max)(0.0f, static_cast<float>(args.RequireNumber("start")));
            if (args.FindMember("end") != nullptr)
                AutoMcpEngineAccess::SlotEndTime(slot) = (std::max)(0.0f, static_cast<float>(args.RequireNumber("end")));
            if (args.FindMember("time") != nullptr)
                AutoMcpEngineAccess::SetSlotTime(slot, (std::max)(0.0f, static_cast<float>(args.RequireNumber("time"))));

            DescribeAnimationViewState(*window, false, result, allocator);
        }

        static MainWindow::ModelPreviewStage& CurrentPreviewStage()
        {
            const auto& mainWindow = ApplicationBase::GetMainWindow();
            if (const auto modelView = std::dynamic_pointer_cast<MainWindow::ModelViewWindow>(mainWindow))
                return AutoMcpEngineAccess::ModelViewStage(*modelView);
            if (const auto animationView = std::dynamic_pointer_cast<MainWindow::AnimationViewWindow>(mainWindow))
                return AutoMcpEngineAccess::AnimationViewStage(*animationView);

            throw AutoMcpError("preview.camera needs ModelViewWindow or AnimationViewWindow as the current main window (modelview.open / animationview.open)");
        }

        static void CommandPreviewCamera(const JsonArgs& args, JsonValue& result, JsonAllocator& allocator)
        {
            auto& stage = CurrentPreviewStage();

            glm::vec3 position;
            glm::vec3 lookAt;
            const bool hasPosition = args.TryGetVec3("position", position);
            const bool hasLookAt   = args.TryGetVec3("lookAt", lookAt);
            const bool hasYaw      = args.FindMember("yawDegrees") != nullptr;
            const bool hasPitch    = args.FindMember("pitchDegrees") != nullptr;

            if (hasPosition || hasLookAt)
            {
                if (!hasPosition || !hasLookAt)
                    throw AutoMcpError("position and lookAt must be given together");
                if (glm::length(lookAt - position) < 1.0e-4f)
                    throw AutoMcpError("lookAt is at the camera position");

                AutoMcpEngineAccess::SetPreviewCamera(stage, position, LookRotation(glm::normalize(lookAt - position)));
                result.AddMember("appliedNextFrame", false, allocator);
            }
            else if (hasYaw || hasPitch)
            {
                const float yaw   = hasYaw   ? static_cast<float>(args.RequireNumber("yawDegrees"))   : 0.0f;
                const float pitch = hasPitch ? static_cast<float>(args.RequireNumber("pitchDegrees")) : 20.0f;
                AutoMcpEngineAccess::PreviewFrame(stage, ViewDirectionFromAngles(yaw, pitch));
                result.AddMember("appliedNextFrame", true, allocator);
            }
            else if (args.OptionalBool("frame", false))
            {
                stage.RequestFrame();
                result.AddMember("appliedNextFrame", true, allocator);
            }
            else
            {
                throw AutoMcpError("pass frame, yawDegrees/pitchDegrees, or position + lookAt");
            }

            AddPreviewCamera(stage, result, allocator);
        }
    };

    const std::unordered_map<std::string, AutoMcpCommand>& AutoMcpCommandHandlers::Table()
    {
        static const std::unordered_map<std::string, AutoMcpCommand> commands = {
            {"ping",                    {AutoMcpPhase::FrameEnd,   CommandPing}},
            {"status",                  {AutoMcpPhase::FrameEnd,   CommandStatus}},
            {"windows.list",            {AutoMcpPhase::FrameEnd,   CommandWindowsList}},
            {"windows.open",            {AutoMcpPhase::FrameEnd,   CommandWindowsOpen}},
            {"windows.close",           {AutoMcpPhase::FrameEnd,   CommandWindowsClose}},
            {"windows.set",             {AutoMcpPhase::FrameBegin, CommandWindowsSet}},
            {"mainwindow.switch",       {AutoMcpPhase::FrameEnd,   CommandMainWindowSwitch}},
            {"scene.list",              {AutoMcpPhase::FrameEnd,   CommandSceneList}},
            {"scene.load",              {AutoMcpPhase::FrameEnd,   CommandSceneLoad}},
            {"scene.reload",            {AutoMcpPhase::FrameEnd,   CommandSceneReload}},
            {"hierarchy",               {AutoMcpPhase::FrameEnd,   CommandHierarchy}},
            {"gameobject.find",         {AutoMcpPhase::FrameEnd,   CommandGameObjectFind}},
            {"gameobject.get",          {AutoMcpPhase::FrameEnd,   CommandGameObjectGet}},
            {"gameobject.get_json",     {AutoMcpPhase::FrameEnd,   CommandGameObjectGetJson}},
            {"gameobject.set_json",     {AutoMcpPhase::FrameEnd,   CommandGameObjectSetJson}},
            {"gameobject.set_transform",{AutoMcpPhase::FrameEnd,   CommandGameObjectSetTransform}},
            {"gameobject.set_enable",   {AutoMcpPhase::FrameEnd,   CommandGameObjectSetEnable}},
            {"gameobject.select",       {AutoMcpPhase::FrameEnd,   CommandGameObjectSelect}},
            {"gameobject.destroy",      {AutoMcpPhase::FrameEnd,   CommandGameObjectDestroy}},
            {"component.set_enable",    {AutoMcpPhase::FrameEnd,   CommandComponentSetEnable}},
            {"play",                    {AutoMcpPhase::FrameEnd,   CommandPlay}},
            {"stop",                    {AutoMcpPhase::FrameEnd,   CommandStop}},
            {"end",                     {AutoMcpPhase::FrameEnd,   CommandEnd}},
            {"time.set_scale",          {AutoMcpPhase::FrameEnd,   CommandTimeSetScale}},
            {"hotreload.status",        {AutoMcpPhase::FrameEnd,   CommandHotReloadStatus}},
            {"hotreload.reload",        {AutoMcpPhase::FrameEnd,   CommandHotReloadReload}},
            {"camera.get",              {AutoMcpPhase::FrameEnd,   CommandCameraGet}},
            {"camera.set",              {AutoMcpPhase::FrameEnd,   CommandCameraSet}},
            {"debugdraw.get",           {AutoMcpPhase::FrameEnd,   CommandDebugDrawGet}},
            {"debugdraw.set",           {AutoMcpPhase::FrameEnd,   CommandDebugDrawSet}},
            {"log.tail",                {AutoMcpPhase::FrameEnd,   CommandLogTail}},
            {"assets.find",             {AutoMcpPhase::FrameEnd,   CommandAssetsFind}},
            {"assets.reload",           {AutoMcpPhase::FrameEnd,   CommandAssetsReload}},
            {"modelview.open",          {AutoMcpPhase::FrameEnd,   CommandModelViewOpen}},
            {"modelview.state",         {AutoMcpPhase::FrameEnd,   CommandModelViewState}},
            {"modelview.select",        {AutoMcpPhase::FrameEnd,   CommandModelViewSelect}},
            {"modelview.close",         {AutoMcpPhase::FrameEnd,   CommandModelViewClose}},
            {"animationview.open",      {AutoMcpPhase::FrameEnd,   CommandAnimationViewOpen}},
            {"animationview.state",     {AutoMcpPhase::FrameEnd,   CommandAnimationViewState}},
            {"animationview.set",       {AutoMcpPhase::FrameEnd,   CommandAnimationViewSet}},
            {"animationview.set_clip",  {AutoMcpPhase::FrameEnd,   CommandAnimationViewSetClip}},
            {"animationview.bones",     {AutoMcpPhase::FrameEnd,   CommandAnimationViewBones}},
            {"preview.camera",          {AutoMcpPhase::FrameEnd,   CommandPreviewCamera}},
        };
        return commands;
    }

    const std::unordered_map<std::string, AutoMcpCommand>& AutoMcpCommandTable::Get()
    {
        return AutoMcpCommandHandlers::Table();
    }
}
