#include "RelayServerSettings.h"

#include <algorithm>
#include <cstdio>
#include "cereal/types/string.hpp"
#include "Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"

namespace GamePlay::Network
{
    constexpr auto RELAY_SERVER_SETTINGS_KEY  = "RelayServer";
    constexpr auto RELAY_SERVER_SETTINGS_PATH = "Network/";

    namespace
    {
        void InputString(const char* label, std::string& value)
        {
            char buffer[256] = {};
            snprintf(buffer, sizeof(buffer), "%s", value.c_str());
            if (ImGui::InputText(label, buffer, sizeof(buffer)))
                value = buffer;
        }
    }

    RelayServerSettings RelayServerSettings::Load()
    {
        return NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath<RelayServerSettings>(
            RELAY_SERVER_SETTINGS_PATH, RELAY_SERVER_SETTINGS_KEY, RelayServerSettings{});
    }

    bool RelayServerSettings::IsEnabled() const
    {
        return useRelayServer && !address.empty() && port != 0 && !appId.empty();
    }

    void RelayServerSettings::OnDrawGui()
    {
        ImGui::Checkbox("Use Relay Server", &useRelayServer);
        InputString("Address", address);

        int portValue = port;
        if (ImGui::InputInt("Port", &portValue))
            port = static_cast<std::uint16_t>(std::clamp(portValue, 1, 65535));

        InputString("App Id", appId);

        if (useRelayServer && !IsEnabled())
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Address / Port / App Id are required (falls back to LAN)");
    }

    REGISTER_LOCAL_PREF_WITH_PATH(RelayServerSettings, RELAY_SERVER_SETTINGS_KEY, RelayServerSettings{}, RELAY_SERVER_SETTINGS_PATH)
}
