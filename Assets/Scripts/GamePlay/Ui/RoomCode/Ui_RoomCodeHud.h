#pragma once
#include <string>

#include "cereal/types/string.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    /**
     * @brief 非公開の部屋にいる間、画面の隅に部屋番号を出す。
     * 番号は中継サーバーが決めて CustomNetworkRunner が持っているので、変わったときだけ書き直す。
     * 公開部屋・LAN・通信していないときは何も出さない
     */
    class RoomCodeHud final : public Component::ComponentBase,
                              public LifeCycleCallback::IStartable,
                              public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart() override;
        void OnUpdate() override;

        void Show(const std::string& code);
        /** @brief 482913 を 482 913 と読みやすく区切る */
        [[nodiscard]] std::string FormatCode(const std::string& code) const;

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) codeText_;
        // 何桁ごとに区切って読みやすくするか (0 で区切らない)
        [[serialize(0)]] int codeGroupSize_ = 3;
        [[serialize(0)]] std::string codeGroupSeparator_ = " ";

        std::string shownCode_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(codeText_));
            archive(CEREAL_NVP(codeGroupSize_));
            archive(CEREAL_NVP(codeGroupSeparator_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(codeText_));
            if (version >= 0) archive(CEREAL_NVP(codeGroupSize_));
            if (version >= 0) archive(CEREAL_NVP(codeGroupSeparator_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::RoomCodeHud, 0);
