#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ComponentBase.h"
#include "../../LifeCycleCallback/LateUpdate/LateUpdate.h"
#include "../../LifeCycleCallback/PreFixedUpdate/IPreFixedUpdate.h"
#include "BonePose/BonePose.h"
#include "Sync/Transform/TransformSync.h"

namespace NanamiEngine::Module::Component
{
    /** @brief 同じ GameObject の ModelRenderer のボーン姿勢を取り出し、登録された Sync に渡す */
    class BoneSync final : public ComponentBase,
                           public LifeCycleCallback::IPreFixedUpdate,
                           public LifeCycleCallback::ILateUpdatable
    {
    public:
        /** @brief ボーン名からインデックス*/
        [[nodiscard]] int FindBoneIndex(const std::string& boneName) const;
        [[nodiscard]] std::optional<glm::mat4>      GetBoneWorldMatrix(int boneIndex) const;
        [[nodiscard]] std::optional<Bone::BonePose> GetBoneWorldPose  (int boneIndex) const;

    private:
        void OnPreFixedUpdate() override;
        void OnLateUpdate    () override;
        void SyncBones();
        [[nodiscard]] int                      CurrentModelHandle() const;
        [[nodiscard]] std::vector<std::string> GetBoneNames() const;

        [[serialize(0)]] std::vector<std::unique_ptr<Bone::BoneSyncBase>> syncs_;
        mutable std::unordered_map<std::string, int> boneIndexCache_;
        mutable int boneIndexCacheModelHandle_ = -1;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            const std::size_t syncCount = syncs_.size();
            archive(cereal::make_nvp("syncCount", syncCount));
            for (const auto& sync : syncs_)
            {
                archive(cereal::make_nvp("sync", sync));
            }
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            std::size_t syncCount = 0;
            archive(cereal::make_nvp("syncCount", syncCount));
            syncs_.clear();
            syncs_.reserve(syncCount);
            for (std::size_t i = 0; i < syncCount; ++i)
            {
                std::unique_ptr<Bone::BoneSyncBase> sync;
                archive(cereal::make_nvp("sync", sync));
                syncs_.push_back(std::move(sync));
            }
        }
#pragma endregion
    };
}
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::BoneSync, 0)
