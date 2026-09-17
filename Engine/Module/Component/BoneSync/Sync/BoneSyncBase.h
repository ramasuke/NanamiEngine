#pragma once
#include <string>
#include <vector>
#include "cereal/cereal.hpp"

namespace NanamiEngine::Module::Bone
{
    struct BonePose;

    class BoneSyncBase
    {
    public:
        virtual ~BoneSyncBase() = default;
        [[nodiscard]] const std::string& BoneName() const { return boneName_; }
        virtual void ApplyBonePose(const BonePose& bonePose) = 0;
        virtual void DoDrawGui() = 0;

    private:
        [[serialize(0)]] std::string boneName_;

#pragma region Serialization Function
    public:
        /** @param boneNames ボーン名の選択候補。モデル未ロードで空なら名前を直接入力する */
        void OnDrawGui(const std::vector<std::string>& boneNames);
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(boneName_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(CEREAL_NVP(boneName_));
        }
#pragma endregion
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Bone::BoneSyncBase, 0)
