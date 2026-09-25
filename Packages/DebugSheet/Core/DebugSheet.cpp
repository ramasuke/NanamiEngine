#include "DebugSheet.h"

#include <algorithm>
#include <string_view>

#include "DebugSheetStyle.h"
#include "DebugSheetWidgets.h"
#include "../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../Libs/LibCore/ImGui/Wrapper/ImGuiWrapper.h"

namespace NanamiEngine::DebugSheet
{
    namespace
    {
        constexpr bool IS_GAME_BUILD =
            Core::Application::Configuration::APPLICATION_MODE == Core::Application::Configuration::ApplicationMode::Game;

        /** @brief エディタではプレイ中（一時停止中を含む）。ゲームビルドは起動時に Play するので常に true */
        bool IsGameRunning()
        {
            const auto gameWindow = Core::Application::ApplicationBase::GameWindow();
            return gameWindow && gameWindow->IsPlaying();
        }

        ImVec4 ToImVec4(const Palette::Rgba& color)
        {
            return { color.r, color.g, color.b, color.a };
        }
    }

    Sheet::Sheet()
    {
        root_.name = "DEBUG SHEET";
        stack_.push_back(&root_);
    }

    void Sheet::RegisterPage(const std::string& path, DrawPage draw, const int order, const Core::ModuleHandle module)
    {
        Node* node = &root_;
        std::string_view rest = path;
        while (!rest.empty())
        {
            const auto        slash   = rest.find('/');
            const std::string segment(rest.substr(0, slash));
            rest = slash == std::string_view::npos ? std::string_view() : rest.substr(slash + 1);
            if (segment.empty())
                continue;

            const auto found = std::ranges::find_if(node->children, [&](const auto& child) { return child->name == segment; });
            if (found != node->children.end())
            {
                // NOTE: カテゴリは子ページの中で一番小さい order の位置に並ぶ
                (*found)->order = (std::min)((*found)->order, order);
                node = found->get();
                continue;
            }

            auto child   = std::make_unique<Node>();
            child->name  = segment;
            child->order = order;
            node = node->children.emplace_back(std::move(child)).get();
        }

        node->draw   = std::move(draw);
        node->order  = order;
        node->module = module;
        isSortDirty_ = true;
    }

    void Sheet::RegisterPage(const std::string& path, DrawPage draw, const int order)
    {
        RegisterPage(path, std::move(draw), order, Core::ModuleHandle{});
    }

    std::size_t Sheet::UnregisterModule(const Core::ModuleHandle module)
    {
        const std::size_t removed = RemovePagesOfModule(root_, module);
        if (removed > 0)
        {
            // 消したページを開いていたかもしれないので、ページスタックはルートに戻す
            stack_.clear();
            stack_.push_back(&root_);
            isSortDirty_ = true;
        }
        return removed;
    }

    std::size_t Sheet::RemovePagesOfModule(Node& node, const Core::ModuleHandle module)
    {
        std::size_t removed = 0;
        for (auto& child : node.children)
            removed += RemovePagesOfModule(*child, module);
        removed += std::erase_if(node.children, [module](const std::unique_ptr<Node>& child)
        {
            const bool isPageOfModule = child->draw && child->module == module;
            const bool isEmptyCategory = !child->draw && child->children.empty();
            return isPageOfModule || isEmptyCategory;
        });
        return removed;
    }

    void Sheet::Update()
    {
        if (!IsGameRunning())
        {
            // NOTE: プレイを終えたら閉じ、次のプレイはトップページから開く
            if (isOpen_)
            {
                Close();
                stack_.resize(1);
            }
            toggleKeyHeld_ = false;
            return;
        }

        const bool toggleKey = CheckHitKey(KEY_INPUT_F1) != 0 && GetWindowActiveFlag() != 0;
        if (toggleKey && !toggleKeyHeld_)
            Toggle();
        toggleKeyHeld_ = toggleKey;
    }

    void Sheet::Render()
    {
        if constexpr (!IS_GAME_BUILD)
        {
            if (isOpen_)
                DrawWindow();
        }
        else
        {
            if (!isImGuiReady_)
            {
                if (!isOpen_)
                    return;

                ImGuiWrapper::CreateInstance();
                isImGuiReady_ = true;
            }

            // NOTE: 一度作ったら閉じている間もフレームを回す。回さないと WndProc から来た入力がキューに溜まり続ける
            ImGuiWrapper::Instance().Update();
            if (isOpen_)
                DrawWindow();
            ImGui::EndFrame();

            // NOTE: ここまでに積まれた DxLib の描画を先に出してから、その上に重ねる
            RenderVertex();
            ImGuiWrapper::Instance().Draw();
        }
    }

    void Sheet::Open()
    {
        if (!IsGameRunning())
            return;

        isOpen_ = true;
        if constexpr (IS_GAME_BUILD)
            SetMouseDispFlag(TRUE);
    }

    void Sheet::Close()
    {
        isOpen_ = false;
        // NOTE: ゲームはゲーム側のカーソルを描くので OS のカーソルを消し直す
        if constexpr (IS_GAME_BUILD)
            SetMouseDispFlag(FALSE);
    }

    void Sheet::Toggle()
    {
        if (isOpen_) Close();
        else         Open();
    }

    void Sheet::DrawWindow()
    {
        if (isSortDirty_)
        {
            SortChildren(root_);
            isSortDirty_ = false;
        }

        ScopedStyle style;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float  width   = (std::min)(Metrics::PANEL_WIDTH, display.x - Metrics::PANEL_MARGIN * 2.0f);
        ImGui::SetNextWindowPos (ImVec2(display.x - width - Metrics::PANEL_MARGIN, Metrics::PANEL_MARGIN));
        ImGui::SetNextWindowSize(ImVec2(width, display.y - Metrics::PANEL_MARGIN * 2.0f));

        constexpr ImGuiWindowFlags FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                                         | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse
                                         | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("##NanamiDebugSheet", nullptr, FLAGS))
        {
            ImGui::SetWindowFontScale(Metrics::FONT_SCALE);
            DrawHeader();

            if (ImGui::BeginChild("##DebugSheetPage"))
            {
                ImGui::SetWindowFontScale(Metrics::FONT_SCALE);
                DrawNode(Current());
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

    void Sheet::DrawHeader()
    {
        const ImVec2 rowStart = ImGui::GetCursorPos();
        const float  right    = rowStart.x + ImGui::GetContentRegionAvail().x;
        const ImVec2 squareButton(Metrics::CELL_HEIGHT, Metrics::CELL_HEIGHT);

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

        if (stack_.size() > 1)
        {
            if (ImGui::Button("<", squareButton))
                stack_.pop_back();
            ImGui::SameLine();
        }

        // NOTE: ボタンの行の縦中央にタイトルを置く
        const ImVec2 titlePosition = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(titlePosition.x + 4.0f, titlePosition.y + (Metrics::CELL_HEIGHT - ImGui::GetTextLineHeight()) * 0.5f),
            ImGui::GetColorU32(ToImVec4(Palette::ACCENT)),
            Current().name.c_str());

        ImGui::SetCursorPos(ImVec2(right - Metrics::CELL_HEIGHT, rowStart.y));
        if (ImGui::Button("x", squareButton))
            Close();

        ImGui::PopStyleVar();

        if (stack_.size() > 1)
        {
            std::string breadcrumb;
            for (std::size_t i = 1; i < stack_.size(); ++i)
                breadcrumb += (i == 1 ? "" : " / ") + stack_[i]->name;
            Widgets::Note(breadcrumb);
        }
        else
        {
            Widgets::Note("F1 で開閉");
        }

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            cursor,
            ImVec2(cursor.x + ImGui::GetContentRegionAvail().x, cursor.y),
            ImGui::GetColorU32(ToImVec4(Palette::ACCENT)),
            2.0f);
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
    }

    void Sheet::DrawNode(Node& node)
    {
        if (node.draw)
        {
            ImGui::PushID(&node);
            node.draw();
            ImGui::PopID();
        }

        if (node.children.empty())
        {
            if (!node.draw)
                Widgets::Note("登録されたページがありません");
            return;
        }

        if (node.draw)
            Widgets::Header("ページ");

        for (const auto& child : node.children)
        {
            if (Widgets::NavigationCell(child->name))
                stack_.push_back(child.get());
        }
    }

    void Sheet::SortChildren(Node& node)
    {
        std::ranges::stable_sort(node.children, [](const auto& lhs, const auto& rhs)
        {
            if (lhs->order != rhs->order)
                return lhs->order < rhs->order;
            return lhs->name < rhs->name;
        });

        for (const auto& child : node.children)
            SortChildren(*child);
    }

    Sheet::Node& Sheet::Current() const
    {
        return *stack_.back();
    }
}

NanamiEngine::DebugSheet::Sheet& NanamiEngine::DebugSheet::Sheet::Instance()
{
    static Sheet instance;
    return instance;
}
