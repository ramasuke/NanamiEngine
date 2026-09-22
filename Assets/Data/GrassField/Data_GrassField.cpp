#include "Data_GrassField.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <ranges>

#include "DxLib.h"
#include "glm.hpp"
#include "ImGuizmo.h"
#include "cereal/external/base64.hpp"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Core/FileSystem/Directory/Directory.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../Scripts/GamePlay/Prop/Grass/Grassable.h"

namespace NanamiEngine::Module::Asset
{
    namespace
    {
        constexpr float       GRASS_QUANTIZE_MAX       = 65535.0f;
        constexpr std::size_t GRASS_BYTES_PER_BLADE    = 6;
        constexpr std::size_t GRASS_BASE64_PER_BLADE   = 8;

        std::uint16_t QuantizeGrass01(const float value01)
        {
            return static_cast<std::uint16_t>(std::clamp(std::lround(value01 * GRASS_QUANTIZE_MAX), 0L, 65535L));
        }

        float DequantizeGrass01(const std::uint16_t value)
        {
            return static_cast<float>(value) / GRASS_QUANTIZE_MAX;
        }

        std::uint16_t QuantizeGrassHeight(const float minY, const float maxY, const float y)
        {
            const float range = maxY - minY;
            return range > 0.0f ? QuantizeGrass01((y - minY) / range) : 0;
        }

        std::uint32_t GrassHash32(std::uint32_t x)
        {
            x ^= x >> 16;
            x *= 0x7feb352dU;
            x ^= x >> 15;
            x *= 0x846ca68bU;
            x ^= x >> 16;
            return x;
        }

        float GrassHashTo01(const std::uint32_t hash)
        {
            return static_cast<float>(hash >> 8) / 16777216.0f;
        }

        void PushGrassU16(std::vector<unsigned char>& bytes, const std::uint16_t value)
        {
            bytes.push_back(static_cast<unsigned char>(value & 0xff));
            bytes.push_back(static_cast<unsigned char>(value >> 8));
        }

        std::uint16_t ReadGrassU16(const std::string& bytes, const std::size_t offset)
        {
            return static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset]) |
                                              static_cast<unsigned char>(bytes[offset + 1]) << 8);
        }

        bool IsGrassableHit(const Physics::RaycastHit& hit)
        {
            return hit.Hit() && !hit.HitObject().Components().Catch<::GamePlay::Prop::Grassable>().expired();
        }

        const Core::FileSystem::File* FindGrassFieldFile(Core::FileSystem::Directory& directory, const AssetBase* content)
        {
            for (const auto& file : directory.Files())
            {
                if (file.GetContent().get() == content)
                    return &file;
            }
            for (auto& child : directory.GetDirectories())
            {
                if (const auto* found = FindGrassFieldFile(child, content))
                    return found;
            }
            return nullptr;
        }
    }

    GrassField::GrassField(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    glm::vec3 GrassField::DecodeBlade(const Data::GrassField::ChunkKey& key,
                                      const Data::GrassField::GrassChunk& chunk,
                                      const Data::GrassField::QuantizedBlade& blade) const
    {
        return {
            (static_cast<float>(key.first)  + DequantizeGrass01(blade.qx)) * chunkSize_,
            chunk.minY + DequantizeGrass01(blade.qy) * (chunk.maxY - chunk.minY),
            (static_cast<float>(key.second) + DequantizeGrass01(blade.qz)) * chunkSize_,
        };
    }

    Data::GrassField::BladeVariation GrassField::Variation(const Data::GrassField::ChunkKey& key,
                                                           const Data::GrassField::QuantizedBlade& blade)
    {
        std::uint32_t hash = GrassHash32(static_cast<std::uint32_t>(key.first)  * 73856093U ^
                                         static_cast<std::uint32_t>(key.second) * 19349663U);
        hash = GrassHash32(hash ^ (static_cast<std::uint32_t>(blade.qx) | static_cast<std::uint32_t>(blade.qz) << 16));

        Data::GrassField::BladeVariation variation;
        variation.yaw      = GrassHashTo01(hash) * DX_TWO_PI_F;
        hash               = GrassHash32(hash);
        variation.height01 = GrassHashTo01(hash);
        hash               = GrassHash32(hash);
        variation.width01  = GrassHashTo01(hash);
        hash               = GrassHash32(hash);
        variation.color01  = GrassHashTo01(hash);
        hash               = GrassHash32(hash);
        variation.phase01  = GrassHashTo01(hash);
        return variation;
    }

    void GrassField::AddBlade(const glm::vec3& worldPos)
    {
        const Data::GrassField::ChunkKey key{
            static_cast<int>(std::floor(worldPos.x / chunkSize_)),
            static_cast<int>(std::floor(worldPos.z / chunkSize_)),
        };

        auto [it, inserted] = chunks_.try_emplace(key);
        Data::GrassField::GrassChunk& chunk = it->second;
        if (inserted)
        {
            chunk.minY = worldPos.y;
            chunk.maxY = worldPos.y;
        }
        else if (worldPos.y < chunk.minY || worldPos.y > chunk.maxY)
        {
            // 高さの範囲が広がると既存の qy の意味が変わるので、実座標に戻してから詰め直す
            const float newMinY = (std::min)(chunk.minY, worldPos.y);
            const float newMaxY = (std::max)(chunk.maxY, worldPos.y);
            for (auto& blade : chunk.blades)
            {
                const float y = chunk.minY + DequantizeGrass01(blade.qy) * (chunk.maxY - chunk.minY);
                blade.qy = QuantizeGrassHeight(newMinY, newMaxY, y);
            }
            chunk.minY = newMinY;
            chunk.maxY = newMaxY;
        }

        Data::GrassField::QuantizedBlade blade;
        blade.qx = QuantizeGrass01(worldPos.x / chunkSize_ - static_cast<float>(key.first));
        blade.qz = QuantizeGrass01(worldPos.z / chunkSize_ - static_cast<float>(key.second));
        blade.qy = QuantizeGrassHeight(chunk.minY, chunk.maxY, worldPos.y);
        chunk.blades.push_back(blade);
    }

    void GrassField::PlaceAround(const glm::vec3& center)
    {
        static std::mt19937 engine(std::random_device{}());
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);

        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const float minNormalY = std::cos(maxSlopeDeg_ * DX_PI_F / 180.0f);

        int placed = 0;
        for (int i = 0; i < bladesPerClick_; ++i)
        {
            const float angle  = unit(engine) * DX_TWO_PI_F;
            const float radius = brushRadius_ * std::sqrt(unit(engine));
            const glm::vec3 origin(center.x + std::cos(angle) * radius,
                                   center.y + rayHeight_,
                                   center.z + std::sin(angle) * radius);

            const auto hit = Physics::Raycast(origin, glm::vec3(0.0f, -1.0f, 0.0f), rayHeight_ * 2.0f, mask);
            if (!IsGrassableHit(hit) || hit.Normal().y < minNormalY)
                continue;

            AddBlade(hit.Position());
            ++placed;
        }

        if (placed > 0)
            ++revision_;
    }

    void GrassField::UpdatePlacement()
    {
        if (!Core::Application::ApplicationBase::GameWindow()->IsPlaying())
        {
            isPlacing_ = false;
            return;
        }

        if (!isPlacing_ || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            return;
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGuizmo::IsOver())
            return;

        int mouseX = 0;
        int mouseY = 0;
        GetMousePoint(&mouseX, &mouseY);

        const VECTOR nearPos = ConvScreenPosToWorldPos(VGet(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.0f));
        const VECTOR farPos  = ConvScreenPosToWorldPos(VGet(static_cast<float>(mouseX), static_cast<float>(mouseY), 1.0f));
        const glm::vec3 origin(nearPos.x, nearPos.y, nearPos.z);
        const glm::vec3 toFar = glm::vec3(farPos.x, farPos.y, farPos.z) - origin;
        const float     distance = glm::length(toFar);
        if (distance <= 0.0f)
            return;

        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const auto hit = Physics::Raycast(origin, toFar / distance, distance, mask);
        if (!IsGrassableHit(hit))
            return;

        PlaceAround(hit.Position());
    }

    std::size_t GrassField::BladeCount() const
    {
        std::size_t count = 0;
        for (const auto& chunk : chunks_ | std::views::values)
            count += chunk.blades.size();
        return count;
    }

    std::vector<Data::GrassField::GrassChunkRecord> GrassField::EncodeChunks() const
    {
        std::vector<Data::GrassField::GrassChunkRecord> records;
        records.reserve(chunks_.size());

        std::vector<unsigned char> bytes;
        for (const auto& [key, chunk] : chunks_)
        {
            if (chunk.blades.empty())
                continue;

            bytes.clear();
            bytes.reserve(chunk.blades.size() * GRASS_BYTES_PER_BLADE);
            for (const auto& blade : chunk.blades)
            {
                PushGrassU16(bytes, blade.qx);
                PushGrassU16(bytes, blade.qz);
                PushGrassU16(bytes, blade.qy);
            }

            Data::GrassField::GrassChunkRecord record;
            record.cx     = key.first;
            record.cz     = key.second;
            record.minY   = chunk.minY;
            record.maxY   = chunk.maxY;
            record.count  = static_cast<std::uint32_t>(chunk.blades.size());
            record.blades = cereal::base64::encode(bytes.data(), bytes.size());
            records.push_back(std::move(record));
        }
        return records;
    }

    void GrassField::DecodeChunks(const std::vector<Data::GrassField::GrassChunkRecord>& records)
    {
        chunks_.clear();
        for (const auto& record : records)
        {
            const std::string bytes = cereal::base64::decode(record.blades);
            const std::size_t count = (std::min)(static_cast<std::size_t>(record.count), bytes.size() / GRASS_BYTES_PER_BLADE);

            Data::GrassField::GrassChunk& chunk = chunks_[{record.cx, record.cz}];
            chunk.minY = record.minY;
            chunk.maxY = record.maxY;
            chunk.blades.resize(count);
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t offset = i * GRASS_BYTES_PER_BLADE;
                chunk.blades[i].qx = ReadGrassU16(bytes, offset);
                chunk.blades[i].qz = ReadGrassU16(bytes, offset + 2);
                chunk.blades[i].qy = ReadGrassU16(bytes, offset + 4);
            }
        }
        ++revision_;
    }

    void GrassField::SaveOwnFile() const
    {
        // 配置は Play 中に行うが、ツールバーの Save は Play 中に出ないので、このアセットだけを保存する
        const auto* file = FindGrassFieldFile(Core::Application::ApplicationBase::AssetsDirectory(), this);
        if (!file)
        {
            LogError("GrassField: 保存先のファイルが見つかりません: " + GetContentPath());
            return;
        }
        file->OnSave();
        Log("GrassField: 保存しました: " + GetContentPath());
    }

    void GrassField::OnDrawGui()
    {
        UpdatePlacement();

        if (!ImGui::CollapsingHeader("Grass Field", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::SeparatorText("Placement");
        const bool isPlayMode = Core::Application::ApplicationBase::GameWindow()->IsPlayMode();
        ImGui::BeginDisabled(!isPlayMode);
        ImGui::Checkbox("Placement Mode", &isPlacing_);
        ImGui::EndDisabled();
        if (!isPlayMode)
            ImGui::TextDisabled("Play 中のみ配置できます");
        else if (isPlacing_)
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Grassable の上をクリックすると草が生えます");

        ImGui::DragInt  ("Blades Per Click", &bladesPerClick_, 1.0f, 1, 10000);
        ImGui::DragFloat("Brush Radius",     &brushRadius_,    1.0f, 1.0f, 100000.0f);
        ImGui::DragFloat("Max Slope (deg)",  &maxSlopeDeg_,    0.5f, 0.0f, 90.0f);
        ImGui::DragFloat("Ray Height",       &rayHeight_,      1.0f, 1.0f, 100000.0f);

        ImGui::SeparatorText("Shape");
        bool isShapeChanged = false;
        isShapeChanged |= ImGui::DragFloatRange2("Height", &heightMin_, &heightMax_, 0.5f, 0.1f, 100000.0f);
        isShapeChanged |= ImGui::DragFloatRange2("Width",  &widthMin_,  &widthMax_,  0.1f, 0.01f, 10000.0f);
        isShapeChanged |= ImGui::SliderFloat("Bend Amount",     &bendAmount_,     0.0f, 1.0f);
        isShapeChanged |= ImGui::SliderFloat("Color Variation", &colorVariation_, 0.0f, 1.0f);
        if (isShapeChanged)
            ++revision_;

        ImGui::SeparatorText("Color");
        baseColor_.DrawColorEdit("Base Color");
        tipColor_ .DrawColorEdit("Tip Color");
        ImGui::SliderFloat("Ambient",    &ambient_, 0.0f, 1.0f);

        // 向き・速さ・周波数は WindZone(シーンに1つ)が持つ。ここにあるのは草の振幅だけ
        ImGui::SeparatorText("Wind");
        ImGui::DragFloat("Strength", &windStrength_, 0.1f, 0.0f, 10000.0f);

        ImGui::SeparatorText("Render");
        ImGui::DragFloat("Max Draw Distance", &maxDrawDistance_, 10.0f, 0.0f, 1000000.0f);
        ImGui::BeginDisabled(!chunks_.empty());
        ImGui::DragFloat("Chunk Size", &chunkSize_, 10.0f, 100.0f, 100000.0f);
        ImGui::EndDisabled();

        ImGui::SeparatorText("Data");
        const std::size_t bladeCount = BladeCount();
        ImGui::Text("Blades: %zu  Chunks: %zu", bladeCount, chunks_.size());
        ImGui::Text("Estimated Size: %.1f KB", static_cast<double>(bladeCount * GRASS_BASE64_PER_BLADE) / 1024.0);

        if (ImGui::Button("Clear"))
        {
            chunks_.clear();
            ++revision_;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Grass Field"))
            SaveOwnFile();
    }
}
