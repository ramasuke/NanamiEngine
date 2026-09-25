#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"
#include "../cereal/include/cereal/types/polymorphic.hpp"
#include "vec3.hpp"

namespace NanamiEngine::Module::Asset
{
    class NANAMI_API SoundFile final : public AssetBase, public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit SoundFile(std::string contentPath = "");
        ~SoundFile() override;
        SoundFile(const SoundFile&)            = delete;
        SoundFile& operator=(const SoundFile&) = delete;
        void OnEnableAsset() override;
        [[nodiscard]] const Guid& GetGuid       () const override { return guid_;         }
        [[nodiscard]] int         GetDxLibHandle() const          { return dxLibHandle_;  }
        [[nodiscard]] std::string GetContentPath() const override;

        /** @brief 再生 (バックグラウンド)。loop でループ、restart で再生中でも頭から */
        void Play(bool loop = false, bool restart = true) const;
        void Stop() const;
        [[nodiscard]] bool IsPlaying() const;
        /** @brief 音量 0..255。鳴っている最中にも効く (アセット設定の volume_ は変えない) */
        void SetVolume(int volume) const;
        /** @brief 次の再生 1 回だけの音量 0..255 */
        void SetNextPlayVolume(int volume) const;
        void Set3DPosition(const glm::vec3& position) const;

    private:
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

        std::string contentPath_;
        Guid guid_;
        int dxLibHandle_ = -1;

        int volume_ = 0;
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<AssetBase>(this));
            archive(CEREAL_NVP(contentPath_));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(volume_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<AssetBase>(this));
            if (version >= 0) archive(CEREAL_NVP(contentPath_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
            if (version >= 0) archive(CEREAL_NVP(volume_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SoundFile, 0);
#pragma endregion
