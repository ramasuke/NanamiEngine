#include "Data_HeightGridMap.h"

#include <algorithm>

#include "DxLib.h"
#include "../../../Engine/Module/Physics/Engine_Physics_Physics.h"

namespace NanamiEngine::Module::Asset
{
    HeightGridMap::HeightGridMap(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    glm::vec2 HeightGridMap::CellSize() const
    {
        const float sizeX = divisionsX_ > 0 ? (areaMax_.x - areaMin_.x) / static_cast<float>(divisionsX_) : 0.0f;
        const float sizeZ = divisionsZ_ > 0 ? (areaMax_.y - areaMin_.y) / static_cast<float>(divisionsZ_) : 0.0f;
        return {sizeX, sizeZ};
    }

    bool HeightGridMap::WorldToCell(const glm::vec3& world, int& outX, int& outZ) const
    {
        const float rangeX = areaMax_.x - areaMin_.x;
        const float rangeZ = areaMax_.y - areaMin_.y;
        if (rangeX == 0.0f || rangeZ == 0.0f || divisionsX_ < 1 || divisionsZ_ < 1)
            return false;

        const float u = (world.x - areaMin_.x) / rangeX;
        const float w = (world.z - areaMin_.y) / rangeZ;
        if (u < 0.0f || u >= 1.0f || w < 0.0f || w >= 1.0f)
            return false;

        outX = std::clamp(static_cast<int>(u * divisionsX_), 0, divisionsX_ - 1);
        outZ = std::clamp(static_cast<int>(w * divisionsZ_), 0, divisionsZ_ - 1);
        return true;
    }

    glm::vec3 HeightGridMap::CellToWorld(int x, int z) const
    {
        const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(divisionsX_);
        const float w = (static_cast<float>(z) + 0.5f) / static_cast<float>(divisionsZ_);

        const float worldX = areaMin_.x + (areaMax_.x - areaMin_.x) * u;
        const float worldZ = areaMin_.y + (areaMax_.y - areaMin_.y) * w;
        const float worldY = At(x, z).height;

        return {worldX, worldY, worldZ};
    }

    glm::vec3 HeightGridMap::CellCenterOrigin(int x, int z) const
    {
        const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(divisionsX_);
        const float w = (static_cast<float>(z) + 0.5f) / static_cast<float>(divisionsZ_);

        const float worldX = areaMin_.x + (areaMax_.x - areaMin_.x) * u;
        const float worldZ = areaMin_.y + (areaMax_.y - areaMin_.y) * w;

        return {worldX, samplingHeight_, worldZ};
    }

    void HeightGridMap::Bake()
    {
        if (divisionsX_ < 1 || divisionsZ_ < 1)
            return;

        map_.assign(static_cast<size_t>(divisionsX_) * divisionsZ_, Data::HeightGridMap::HeightGridCell{});

        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        for (int z = 0; z < divisionsZ_; ++z)
        {
            for (int x = 0; x < divisionsX_; ++x)
            {
                const auto hit = Physics::Raycast(CellCenterOrigin(x, z), glm::vec3(0, -1, 0), rayDistance_, mask);

                auto& cell = map_[static_cast<size_t>(z) * divisionsX_ + x];
                if (hit.Hit())
                {
                    cell.height = hit.Position().y;
                }
            }
        }
    }

    void HeightGridMap::DrawGridGizmo(int displayDivisions) const
    {
        if (displayDivisions < 1)
            return;

        // 分割数を変更して未再ベイクの場合、map_ のサイズが現在の分割数と食い違うため
        // サイズ一致まで確認する（不一致なら未ベイク扱いで samplingHeight_ 平面を描く）。
        const size_t expectedCells = static_cast<size_t>(divisionsX_) * static_cast<size_t>(divisionsZ_);
        const bool baked = !map_.empty() && map_.size() == expectedCells;

        // 表示格子点のYを求める。未ベイク時は samplingHeight_、
        // ベイク済みなら最も近い実セルの高さをサンプリングする。
        const auto sampleHeight = [&](float worldX, float worldZ) -> float
        {
            if (!baked)
                return samplingHeight_;

            const float rangeX = areaMax_.x - areaMin_.x;
            const float rangeZ = areaMax_.y - areaMin_.y;
            if (rangeX == 0.0f || rangeZ == 0.0f)
                return samplingHeight_;

            const float u = (worldX - areaMin_.x) / rangeX;
            const float w = (worldZ - areaMin_.y) / rangeZ;

            int cx = std::clamp(static_cast<int>(u * divisionsX_), 0, divisionsX_ - 1);
            int cz = std::clamp(static_cast<int>(w * divisionsZ_), 0, divisionsZ_ - 1);

            const float h = map_[static_cast<size_t>(cz) * divisionsX_ + cx].height;
            return h == std::numeric_limits<float>::lowest() ? samplingHeight_ : h;
        };

        const auto pointAt = [&](int gx, int gz) -> VECTOR
        {
            const float u = static_cast<float>(gx) / static_cast<float>(displayDivisions);
            const float w = static_cast<float>(gz) / static_cast<float>(displayDivisions);
            const float worldX = areaMin_.x + (areaMax_.x - areaMin_.x) * u;
            const float worldZ = areaMin_.y + (areaMax_.y - areaMin_.y) * w;

            return VGet(worldX, sampleHeight(worldX, worldZ), worldZ);
        };

        // ベイク済みは床高さに沿った格子をシアン、未ベイクは samplingHeight_ 平面を白で描画
        const int color = baked ? GetColor(0, 255, 255) : GetColor(255, 255, 255);

        // 縦横の格子線を描画
        for (int gz = 0; gz <= displayDivisions; ++gz)
        {
            for (int gx = 0; gx <= displayDivisions; ++gx)
            {
                if (gx < displayDivisions)
                    DrawLine3D(pointAt(gx, gz), pointAt(gx + 1, gz), color);
                if (gz < displayDivisions)
                    DrawLine3D(pointAt(gx, gz), pointAt(gx, gz + 1), color);
            }
        }
    }

    void HeightGridMap::OnDrawGui()
    {
        if (!ImGui::CollapsingHeader("Height Grid Map", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        static int displayDivisions = 16;

        DrawGridGizmo(displayDivisions);

        ImGui::DragFloat2("Area Min (X,Z)", &areaMin_.x, 0.1f);
        ImGui::DragFloat2("Area Max (X,Z)", &areaMax_.x, 0.1f);

        if (ImGui::InputInt("Divisions X", &divisionsX_))
            divisionsX_ = (std::max)(divisionsX_, 1);
        if (ImGui::InputInt("Divisions Z", &divisionsZ_))
            divisionsZ_ = (std::max)(divisionsZ_, 1);

        ImGui::DragFloat("Sampling Height", &samplingHeight_, 0.1f);
        ImGui::DragFloat("Ray Distance", &rayDistance_, 0.1f, 0.01f, 100000.0f);

        ImGui::SliderInt("Display Divisions", &displayDivisions, 1, 1000);

        ImGui::Separator();

        if (ImGui::Button("Bake Height Map"))
            Bake();

        ImGui::Text("Cells: %d (%d x %d)", static_cast<int>(map_.size()), divisionsX_, divisionsZ_);
    }
}
