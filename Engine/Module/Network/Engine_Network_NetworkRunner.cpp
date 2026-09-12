#include "Engine_Network_NetworkRunner.h"

#include "../../Core/Network/Engine_Network_INetworkSystem.h"
#include "../../Core/Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "../../Core/Coroutine/Awaitable/WaitForObservable/Coroutine_WaitForObservable.h"
#include "../../Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../Exception/Engine_Module_Exception.h"
#include "../Log/NanamiEngine_Module_Log.h"

Network::NetworkRunnerBase* Network::NetworkRunnerBase::s_instance_ = nullptr;

namespace NanamiEngine::Module::Network
{
    NetworkRunnerBase::NetworkRunnerBase()
    {
        s_instance_ = this;
    }

    NetworkRunnerBase::~NetworkRunnerBase()
    {
        if (s_instance_ == this) s_instance_ = nullptr;
    }

    NetworkRunnerBase& NetworkRunnerBase::Instance()
    {
        assert(s_instance_ != nullptr);
        return *s_instance_;
    }

    Core::Network::DefaultPacketDispatcher& NetworkRunnerBase::DefaultDispatcher()
    {
        assert(defaultPacketDispatcher_, "defaultPacketDispatcher is null");
        return defaultPacketDispatcher_.value();
    }

    void NetworkRunnerBase::Initialize()
    {
        Core::Application::Configuration::NetworkConfiguration::Load();
        networkSystem_ = DoCreateUseNetworkSystem();
        defaultPacketDispatcher_.emplace(*networkSystem_, networkSystem_->GetInstanceRegistry());
        DoInitialize();
    }

    Core::Network::PlayerId NetworkRunnerBase::GetPlayerId() const
    {
        return PlayerIdProvider().GetPlayerId();
    }

    Core::Network::PlayerId NetworkRunnerBase::OwnerOf(const Core::Network::NetworkObjectId id) const
    {
        return networkSystem_->GetInstanceRegistry().OwnerOf(id);
    }

    bool NetworkRunnerBase::IsLocallyOwned(const Core::Network::NetworkObjectId id) const
    {
        const auto owner = OwnerOf(id);
        // オフライン時は双方 Invalid になるので、Invalid 同士の一致で権威が立たないよう明示的に弾く
        return owner != Core::Network::PlayerId::Invalid() && owner == GetPlayerId();
    }

    void NetworkRunnerBase::OnUpdate()
    {
        networkSystem_->Update();
        DispatchPollPackets();
        defaultPacketDispatcher_->Update();
    }

    void NetworkRunnerBase::DispatchPollPackets()
    {
        const auto packets = networkSystem_->PollPackets();
        for (auto& packet : packets)
        {
            try
            {
                defaultPacketDispatcher_->DispatchReceivedPacket(packet);
                DoDispatchReceivedPacket(packet);
            }
            catch (const Exception::PacketDeserializeException& exception)
            {
                // ペイロードが壊れている・改ざんされているパケットは 1 つだけ捨てて次へ進む
                LogWarning("NetworkRunner: パケットの処理に失敗しました: " + std::string(exception.what()));
            }
        }
    }

    void NetworkRunnerBase::SendNetworkPacket(const Core::Network::Packet& packet)
    {
        PacketSender().Send(packet);
    }

    Core::Network::IPacketSender& NetworkRunnerBase::PacketSender() const
    {
        return *networkSystem_;
    }

    Core::Network::IPlayerIdProvider& NetworkRunnerBase::PlayerIdProvider() const
    {
        return *networkSystem_;
    }

    void NetworkRunnerBase::BasedOnDrawgui()
    {
        if (networkSystem_)
        {
            ImGui::Text(("playerId: " + networkSystem_->GetPlayerId().ToString()).c_str());

            // Spawn したピアと現在の所有者が異なるオブジェクト(離脱者から移譲されたもの)
            const auto overrides = networkSystem_->GetInstanceRegistry().CollectOwnerOverrides();
            ImGui::Text("owner overrides: %d", static_cast<int>(overrides.size()));
            for (const auto& [id, owner] : overrides)
                ImGui::BulletText("id %s -> owner %s", id.ToString().c_str(), owner.ToString().c_str());
        }
        ImGuiHelper::OnDrawInputField("sampleSpawnPrefab_", sampleSpawnPrefab_);
        if (ImGui::Button("Sample Spawn Prefab"))
        {
            Spawn(*sampleSpawnPrefab_.get(), glm::vec3(0.0f, 0.0f, 0.0f), glm::quat());
        }
    }
    
    void NetworkRunnerBase::Spawn(Asset::PrefabGameObjectFile& prefabFile, const glm::vec3 position, const glm::quat rotation)
    {
        defaultPacketDispatcher_->Spawn().DispatchSendPacket(prefabFile, position, rotation);
    }

    Coroutine::Task<void> NetworkRunnerBase::OnConnectedAsync()
    {
        co_await Coroutine::WaitForObservable(defaultPacketDispatcher_->ReceivedAssignPlayerId().OnAssignedPlayerId());
    }
}
