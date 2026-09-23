#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "vec2.hpp"
#include "vec3.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace Data::GrassField
{
    using ChunkKey = std::pair<int, int>;

    // qx/qz はチャンク内の XZ 位置、qy はチャンクの [minY, maxY] 内の高さを 0..65535 に量子化した値
    struct QuantizedBlade final
    {
        std::uint16_t qx = 0;
        std::uint16_t qz = 0;
        std::uint16_t qy = 0;
    };

    struct GrassChunk final
    {
        float minY = 0.0f;
        float maxY = 0.0f;
        std::vector<QuantizedBlade> blades;
    };

    // 保存用。blades は (qx, qz, qy) を u16 リトルエンディアンで並べたバイト列の base64
    struct GrassChunkRecord final
    {
        int           cx    = 0;
        int           cz    = 0;
        float         minY  = 0.0f;
        float         maxY  = 0.0f;
        std::uint32_t count = 0;
        std::string   blades;

        template <class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(cx));
            archive(CEREAL_NVP(cz));
            archive(CEREAL_NVP(minY));
            archive(CEREAL_NVP(maxY));
            archive(CEREAL_NVP(count));
            archive(CEREAL_NVP(blades));
        }
    };

    // 保存しない草ごとのばらつき。量子化済みの整数から求めるのでロード後も同じ値になる
    struct BladeVariation final
    {
        float yaw      = 0.0f;
        float height01 = 0.0f;
        float width01  = 0.0f;
        float color01  = 0.0f;
        float phase01  = 0.0f;
    };
}
CEREAL_CLASS_VERSION(Data::GrassField::GrassChunkRecord, 0);

namespace NanamiEngine::Module::Asset
{
    constexpr auto GRASS_FIELD_EXTENSION_LABEL = ".grassField";

    /** 草の見た目・風の設定と、配置モードで生やした草の位置(量子化済み)を持つ */
    class GrassField final : public ScriptableObject
    {
    public:
        explicit GrassField(const std::string& contentPath = "");

        [[nodiscard]] std::uint64_t Revision() const { return revision_; }
        [[nodiscard]] const std::map<Data::GrassField::ChunkKey, Data::GrassField::GrassChunk>& Chunks() const { return chunks_; }
        [[nodiscard]] glm::vec3 DecodeBlade(const Data::GrassField::ChunkKey& key,
                                            const Data::GrassField::GrassChunk& chunk,
                                            const Data::GrassField::QuantizedBlade& blade) const;
        [[nodiscard]] static Data::GrassField::BladeVariation Variation(const Data::GrassField::ChunkKey& key,
                                                                        const Data::GrassField::QuantizedBlade& blade);

        [[nodiscard]] float     HeightMin      () const { return heightMin_;       }
        [[nodiscard]] float     HeightMax      () const { return heightMax_;       }
        [[nodiscard]] float     WidthMin       () const { return widthMin_;        }
        [[nodiscard]] float     WidthMax       () const { return widthMax_;        }
        [[nodiscard]] float     BendAmount     () const { return bendAmount_;      }
        [[nodiscard]] float     ColorVariation () const { return colorVariation_;  }
        [[nodiscard]] glm::vec3 BaseColor      () const { return baseColor_.ToVec3(); }
        [[nodiscard]] glm::vec3 TipColor       () const { return tipColor_.ToVec3();  }
        [[nodiscard]] float     Ambient        () const { return ambient_;         }
        [[nodiscard]] float     WindStrength   () const { return windStrength_;    }
        [[nodiscard]] float     MaxDrawDistance() const { return maxDrawDistance_; }

    private:
        void UpdatePlacement();
        void PlaceAround(const glm::vec3& center);
        void AddBlade(const glm::vec3& worldPos);
        void SaveOwnFile() const;
        [[nodiscard]] std::size_t BladeCount() const;
        [[nodiscard]] std::vector<Data::GrassField::GrassChunkRecord> EncodeChunks() const;
        void DecodeChunks(const std::vector<Data::GrassField::GrassChunkRecord>& records);

        int   bladesPerClick_ = 200;
        float brushRadius_    = 300.0f;
        float maxSlopeDeg_    = 40.0f;
        float rayHeight_      = 500.0f;

        float heightMin_  = 35.0f;
        float heightMax_  = 80.0f;
        float widthMin_   = 4.0f;
        float widthMax_   = 8.0f;
        float bendAmount_ = 0.25f;

        NanamiEngine::Color32 baseColor_      {31, 82, 20};
        NanamiEngine::Color32 tipColor_       {140, 199, 77};
        float                 colorVariation_ = 0.25f;
        float                 ambient_        = 0.4f;

        float windStrength_     = 15.0f;

        float maxDrawDistance_ = 8000.0f;
        float chunkSize_       = 1600.0f;

        std::map<Data::GrassField::ChunkKey, Data::GrassField::GrassChunk> chunks_;
        std::uint64_t revision_  = 1;
        bool          isPlacing_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(bladesPerClick_));
            archive(CEREAL_NVP(brushRadius_));
            archive(CEREAL_NVP(maxSlopeDeg_));
            archive(CEREAL_NVP(rayHeight_));
            archive(CEREAL_NVP(heightMin_));
            archive(CEREAL_NVP(heightMax_));
            archive(CEREAL_NVP(widthMin_));
            archive(CEREAL_NVP(widthMax_));
            archive(CEREAL_NVP(bendAmount_));
            archive(CEREAL_NVP(baseColor_));
            archive(CEREAL_NVP(tipColor_));
            archive(CEREAL_NVP(colorVariation_));
            archive(CEREAL_NVP(ambient_));
            archive(CEREAL_NVP(windStrength_));
            archive(CEREAL_NVP(maxDrawDistance_));
            archive(CEREAL_NVP(chunkSize_));

            const std::vector<Data::GrassField::GrassChunkRecord> chunks = EncodeChunks();
            archive(CEREAL_NVP(chunks));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(bladesPerClick_));
            archive(CEREAL_NVP(brushRadius_));
            archive(CEREAL_NVP(maxSlopeDeg_));
            archive(CEREAL_NVP(rayHeight_));
            archive(CEREAL_NVP(heightMin_));
            archive(CEREAL_NVP(heightMax_));
            archive(CEREAL_NVP(widthMin_));
            archive(CEREAL_NVP(widthMax_));
            archive(CEREAL_NVP(bendAmount_));
            LoadColor(archive, version, "baseColor_", baseColor_);
            LoadColor(archive, version, "tipColor_",  tipColor_);
            archive(CEREAL_NVP(colorVariation_));
            archive(CEREAL_NVP(ambient_));
            archive(CEREAL_NVP(windStrength_));
            archive(CEREAL_NVP(maxDrawDistance_));
            archive(CEREAL_NVP(chunkSize_));

            std::vector<Data::GrassField::GrassChunkRecord> chunks;
            archive(CEREAL_NVP(chunks));
            DecodeChunks(chunks);
        }

    private:
        // version 1 までは色を 0..1 の glm::vec3 で保存していた
        template <class Archive>
        static void LoadColor(Archive& archive, const std::uint32_t version, const char* name, NanamiEngine::Color32& color)
        {
            if (version >= 2)
            {
                archive(cereal::make_nvp(name, color));
                return;
            }
            glm::vec3 legacy;
            archive(cereal::make_nvp(name, legacy));
            color = NanamiEngine::Color32::FromVec3(legacy);
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::GrassField, 2);
#pragma endregion
