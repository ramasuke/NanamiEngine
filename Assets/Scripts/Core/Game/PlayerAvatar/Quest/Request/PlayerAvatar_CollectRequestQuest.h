#pragma once
#include "PlayerAvatar_RequestQuestBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "../../../../../../Data/Item/Data_ItemData.h"

namespace GameCore::PlayerAvatar::Quest::Request
{
    /**
     * @brief 収集依頼。受注してから item_ を requiredCount_ 個拾ったら達成。
     * 納品ではないので、拾ったアイテムはそのまま手元に残る
     */
    class CollectRequestQuest final : public RequestQuestBase
    {
    private:
        [[nodiscard]] int CurrentRecord(const Record::IRecordBook& records) const override;
        [[nodiscard]] rxcpp::observable<int> ObserveRecord(const Record::IRecordBook& records) const override;
        void DoDrawGui() override;
        [[nodiscard]] Guid ItemGuid() const;

        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::ItemData) item_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<RequestQuestBase>(this));
            archive(CEREAL_NVP(item_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<RequestQuestBase>(this));
            if (version >= 0) archive(CEREAL_NVP(item_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Quest::Request::CollectRequestQuest, 0);
#pragma endregion
