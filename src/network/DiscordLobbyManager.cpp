#include "DiscordLobbyManager.h"
#include <iostream>

DiscordLobbyManager::DiscordLobbyManager() {}

DiscordLobbyManager::~DiscordLobbyManager() {
    if (core) {
        delete core;
        core = nullptr;
    }
}

bool DiscordLobbyManager::Initialize() {
    discord::Result result = discord::Core::Create(CLIENT_ID, DiscordCreateFlags_Default, &core);
    if (result != discord::Result::Ok) {
        std::cerr << "Failed to initialize Discord SDK. Error code: " << static_cast<int>(result) << std::endl;
        return false;
    }
    
    // Set up lobby activity updates
    core->LobbyManager().OnLobbyUpdate.Connect([this](int64_t lobbyId) {
        std::cout << "Lobby updated: " << lobbyId << std::endl;
    });

    core->LobbyManager().OnMemberConnect.Connect([this](int64_t lobbyId, int64_t userId) {
        std::cout << "User " << userId << " connected to lobby " << lobbyId << std::endl;
        // If we are host, we read the guest's Answer from metadata here
    });

    core->LobbyManager().OnMemberUpdate.Connect([this](int64_t lobbyId, int64_t userId) {
        std::cout << "User " << userId << " updated in lobby " << lobbyId << std::endl;
        // Check if the other user has posted their SDP metadata
        discord::User currentUser;
        core->UserManager().GetCurrentUser(&currentUser);
        if (userId != currentUser.GetId()) {
            char sdpData[4096];
            discord::Result res = core->LobbyManager().GetMemberMetadataValue(lobbyId, userId, "webrtc_sdp", sdpData);
            if (res == discord::Result::Ok && sdpCallback) {
                sdpCallback(std::string(sdpData));
            }
        }
    });

    return true;
}

void DiscordLobbyManager::RunCallbacks() {
    if (core) {
        core->RunCallbacks();
    }
}

void DiscordLobbyManager::CreateLobby() {
    if (!core) return;

    discord::LobbyTransaction txn;
    txn.SetCapacity(2);
    txn.SetType(discord::LobbyType::Public);

    core->LobbyManager().CreateLobby(txn, [this](discord::Result result, discord::Lobby const& lobby) {
        if (result == discord::Result::Ok) {
            std::cout << "Created lobby with ID: " << lobby.GetId() << std::endl;
            currentLobbyId = lobby.GetId();
            
            // Set our activity to the lobby so others can join
            discord::Activity activity{};
            activity.GetParty().GetSize().SetCurrentSize(1);
            activity.GetParty().GetSize().SetMaxSize(2);
            activity.GetParty().SetId(std::to_string(lobby.GetId()).c_str());
            activity.GetSecrets().SetJoin(lobby.GetSecret());
            
            core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
                std::cout << "Activity updated: " << static_cast<int>(result) << std::endl;
            });
        }
    });
}

void DiscordLobbyManager::JoinLobby(int64_t lobbyId, const std::string& secret) {
    if (!core) return;

    core->LobbyManager().ConnectLobby(lobbyId, secret.c_str(), [this](discord::Result result, discord::Lobby const& lobby) {
        if (result == discord::Result::Ok) {
            std::cout << "Joined lobby: " << lobby.GetId() << std::endl;
            currentLobbyId = lobby.GetId();
        } else {
            std::cerr << "Failed to join lobby." << std::endl;
        }
    });
}

void DiscordLobbyManager::SendSDP(const std::string& sdpData) {
    if (!core || currentLobbyId == 0) return;

    discord::User currentUser;
    core->UserManager().GetCurrentUser(&currentUser);
    
    discord::LobbyMemberTransaction txn;
    txn.SetMetadata("webrtc_sdp", sdpData.c_str());

    core->LobbyManager().UpdateMember(currentLobbyId, currentUser.GetId(), txn, [](discord::Result result) {
        if (result == discord::Result::Ok) {
            std::cout << "Successfully sent WebRTC SDP via Discord metadata." << std::endl;
        }
    });
}
