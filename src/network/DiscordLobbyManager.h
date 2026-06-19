#ifndef DISCORD_LOBBY_MANAGER_H
#define DISCORD_LOBBY_MANAGER_H

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include "../../third-party/discord_game_sdk/cpp/discord.h"

class DiscordLobbyManager {
public:
    static DiscordLobbyManager& GetInstance() {
        static DiscordLobbyManager instance;
        return instance;
    }

    // Initialize the Discord Game SDK Core
    bool Initialize();

    // Must be called in the main game loop
    void RunCallbacks();

    // Host a new lobby
    void CreateLobby();

    // Join an existing lobby
    void JoinLobby(int64_t lobbyId, const std::string& secret);

    // Set a callback to receive the WebRTC SDP offer/answer from the other player
    void SetSDPCallback(std::function<void(const std::string&)> callback) {
        sdpCallback = callback;
    }

    // Send our SDP string via Lobby Metadata
    void SendSDP(const std::string& sdpData);

private:
    DiscordLobbyManager();
    ~DiscordLobbyManager();

    discord::Core* core = nullptr;
    int64_t currentLobbyId = 0;
    
    std::function<void(const std::string&)> sdpCallback;

    std::thread callbackThread;
    std::atomic<bool> running{false};

    // Real Discord Client ID
    const int64_t CLIENT_ID = 1516933107578572892LL; 
};

#endif // DISCORD_LOBBY_MANAGER_H
