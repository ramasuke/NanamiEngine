#pragma once
#include <limits>
#include <string>
#include <vector>

#include "vec2.hpp"
#include "vec3.hpp"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace Data::HeightGridMap
{
    struct HeightGridCell final
    {
        float height = std::numeric_limits<float>::lowest();

        template <class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(height));
        }
    };
}
CEREAL_CLASS_VERSION(Data::HeightGridMap::HeightGridCell, 0);

namespace NanamiEngine::Module::Asset
{
    constexpr auto HEIGHT_GRID_MAP_EXTENSION_LABEL = ".heightGridMap";

    /** XZ平面のグリッド各セルから真下へRaycastし、床の高さを保持する2.5Dマップ */
    class HeightGridMap final : public ScriptableObject
    {
    public:
        explicit HeightGridMap(const std::string& contentPath = "");

        [[nodiscard]] int DivisionsX() const { return divisionsX_; }
        [[nodiscard]] int DivisionsZ() const { return divisionsZ_; }
        [[nodiscard]] const Data::HeightGridMap::HeightGridCell& At(int x, int z) const
        {
            return map_[static_cast<size_t>(z) * divisionsX_ + x];
        }

#pragma region Navigation Helper
        /** セル1個あたりのワールドサイズ */
        [[nodiscard]] glm::vec2 CellSize() const;
        [[nodiscard]] bool      WorldToCell(const glm::vec3& world, int& outX, int& outZ) const;
        [[nodiscard]] glm::vec3 CellToWorld(int x, int z) const;
#pragma endregion

    private:
        [[nodiscard]] glm::vec3 CellCenterOrigin(int x, int z) const;
        void Bake();
        void DrawGridGizmo(int displayDivisions) const;

        glm::vec2 areaMin_{-10.0f, -10.0f};
        glm::vec2 areaMax_{ 10.0f,  10.0f};
        int   divisionsX_     = 10;
        int   divisionsZ_     = 10;
        float samplingHeight_ = 50.0f;
        float rayDistance_    = 100.0f;

        std::vector<Data::HeightGridMap::HeightGridCell> map_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));

            archive(CEREAL_NVP(areaMin_));
            archive(CEREAL_NVP(areaMax_));
            archive(CEREAL_NVP(divisionsX_));
            archive(CEREAL_NVP(divisionsZ_));
            archive(CEREAL_NVP(samplingHeight_));
            archive(CEREAL_NVP(rayDistance_));

            // map_ はデータ量が多く、同一高さ（床なしセンチネルや平坦地）が連続するため
            const std::uint32_t cellCount = static_cast<std::uint32_t>(map_.size());
            archive(cereal::make_nvp("cellCount", cellCount));

            std::vector<std::uint32_t> runLengths;
            std::vector<float>         runHeights;
            for (const auto& cell : map_)
            {
                if (!runLengths.empty() && cell.height == runHeights.back())
                {
                    ++runLengths.back();
                }
                else
                {
                    runLengths.push_back(1);
                    runHeights.push_back(cell.height);
                }
            }

            const std::uint32_t runCount = static_cast<std::uint32_t>(runLengths.size());
            archive(cereal::make_nvp("runCount", runCount));
            for (std::uint32_t i = 0; i < runCount; ++i)
            {
                archive(cereal::make_nvp(("runLen_"    + std::to_string(i)).c_str(), runLengths[i]));
                archive(cereal::make_nvp(("runHeight_" + std::to_string(i)).c_str(), runHeights[i]));
            }
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));

            archive(CEREAL_NVP(areaMin_));
            archive(CEREAL_NVP(areaMax_));
            archive(CEREAL_NVP(divisionsX_));
            archive(CEREAL_NVP(divisionsZ_));
            archive(CEREAL_NVP(samplingHeight_));
            archive(CEREAL_NVP(rayDistance_));

            map_.clear();

            if (version == 0)
            {
                std::uint32_t count = 0;
                archive(cereal::make_nvp("count", count));

                map_.reserve(count);
                for (std::uint32_t i = 0; i < count; ++i)
                {
                    Data::HeightGridMap::HeightGridCell cell;
                    archive(cereal::make_nvp(("cell_" + std::to_string(i)).c_str(), cell));
                    map_.push_back(cell);
                }
                return;
            }
            
            std::uint32_t cellCount = 0;
            archive(cereal::make_nvp("cellCount", cellCount));

            std::uint32_t runCount = 0;
            archive(cereal::make_nvp("runCount", runCount));

            map_.reserve(cellCount);
            for (std::uint32_t i = 0; i < runCount; ++i)
            {
                std::uint32_t runLength = 0;
                float         runHeight = 0.0f;
                archive(cereal::make_nvp(("runLen_"    + std::to_string(i)).c_str(), runLength));
                archive(cereal::make_nvp(("runHeight_" + std::to_string(i)).c_str(), runHeight));

                map_.insert(map_.end(), runLength, Data::HeightGridMap::HeightGridCell{runHeight});
            }
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(HeightGridMap, HEIGHT_GRID_MAP_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::HeightGridMap, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::HeightGridMap);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::HeightGridMap);
#pragma endregion
