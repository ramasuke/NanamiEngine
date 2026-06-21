#pragma once
#include "Network_INetworkSyncParameter.h"
#include "Id/Network_SyncParameter_Id.h"
#include "../../Packet/ByteBuffer/Packet_ByteBuffer.h"
#include "../NetworkObjectBase.h"
#include "../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Core::Network
{
    void RegisterSyncParam(INetworkSyncParameter& assignParam, ParameterId id);
    void DeRegisterParamId(ParameterId id);
    void SyncSendParam(const INetworkSyncParameter& param);

    /**
     * ネットワーク上で同期させるT型を作成する型
     * @tparam T 同期させるParameterの型, 条件としてserialize可能でなければならない。
     * IDはNetworkAwake(NetworkObjectId, uint32_t&)が呼ばれた時点でobjectId+localIndexから確定する。
     */
    template <typename T>
    class SyncParameter final : public INetworkSyncParameter, public NetworkObjectBase
    {
    public:
        explicit SyncParameter(T parameter = T())
            : parameter_(parameter)
        {
        }
        ~SyncParameter() override
        {
            if (id_.Value() != UINT64_MAX)
                DeRegisterParamId(id_);
        }
        SyncParameter(const SyncParameter&) = delete;
        SyncParameter& operator=(const SyncParameter&) = delete;
        SyncParameter(SyncParameter&&) = delete;
        SyncParameter& operator=(SyncParameter&&) = delete;

        [[nodiscard]] const T& Get() const { return parameter_; }
        [[nodiscard]] ParameterId GetId() const override { return id_; }

        void Set(const T& value)
        {
            parameter_ = value;
            SyncSendParam(*this);
        }

        void WriteTo(ByteBuffer& buffer) const override
        {
            buffer.Write(id_.Value());
            buffer.Write(parameter_);
        }

    private:
        void NetworkAwake(const NetworkObjectId objectId, uint32_t& localIndex) override
        {
            const uint64_t rawId = static_cast<uint64_t>(objectId.Value()) << 32 | localIndex++;
            id_ = ParameterId(rawId);
            RegisterSyncParam(*this, id_);
            NetworkObjectBase::NetworkAwake(objectId, localIndex);
        }
        
        void SyncReceiveParam(const ByteBuffer& buffer, size_t& offset) override
        {
            parameter_ = buffer.Read<T>(offset);
        }

        ParameterId id_;
        T parameter_;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("parameter_", parameter_);
            if (ImGui::Button("Send Change Parameter Packet"))
            {
                SyncSendParam(*this);
            }
        }

        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(parameter_);
        }
#pragma endregion
    };
}
