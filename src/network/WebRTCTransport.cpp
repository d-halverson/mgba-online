#include "WebRTCTransport.h"
#include <iostream>

WebRTCTransport::WebRTCTransport() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    
    // Set up standard public Google STUN servers for NAT traversal
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    config.iceServers.emplace_back("stun:stun1.l.google.com:19302");
}

WebRTCTransport::~WebRTCTransport() {
    if (dataChannel) dataChannel->close();
    if (peerConnection) peerConnection->close();
}

void WebRTCTransport::InitializeAsHost() {
    peerConnection = std::make_shared<rtc::PeerConnection>(config);
    
    peerConnection->onLocalDescription([this](rtc::Description description) {
        if (onLocalDescription) {
            onLocalDescription(std::string(description));
        }
    });

    peerConnection->onLocalCandidate([this](rtc::Candidate candidate) {
        // In a real robust setup, ICE candidates are sent separately.
        // For simplicity with Discord metadata (which has size limits), 
        // we can rely on SDP exchange which contains enough ICE info for many direct connections.
    });

    // We must create the datachannel before creating the offer
    rtc::DataChannelInit dcConfig;
    dcConfig.ordered = false;    // Unordered for lowest latency (UDP-like)
    dcConfig.maxRetransmits = 0; // Unreliable for lowest latency

    dataChannel = peerConnection->createDataChannel("gba-link", dcConfig);
    SetupDataChannel();

    peerConnection->createDataChannel("dummy"); // Workaround for some libdatachannel versions needing a reliable channel
}

void WebRTCTransport::InitializeAsGuest(const std::string& remoteOffer) {
    peerConnection = std::make_shared<rtc::PeerConnection>(config);

    peerConnection->onLocalDescription([this](rtc::Description description) {
        if (onLocalDescription) {
            onLocalDescription(std::string(description));
        }
    });

    peerConnection->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc) {
        if (dc->label() == "gba-link") {
            dataChannel = dc;
            SetupDataChannel();
        }
    });

    peerConnection->setRemoteDescription(rtc::Description(remoteOffer));
}

void WebRTCTransport::AcceptAnswer(const std::string& remoteAnswer) {
    if (peerConnection) {
        peerConnection->setRemoteDescription(rtc::Description(remoteAnswer));
    }
}

void WebRTCTransport::SetupDataChannel() {
    if (!dataChannel) return;

    dataChannel->onOpen([]() {
        std::cout << "WebRTC DataChannel opened! GBA Link established." << std::endl;
    });

    dataChannel->onMessage([this](rtc::binary data) {
        if (onDataReceived) {
            onDataReceived(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        }
    });

    dataChannel->onMessage([this](std::string msg) {
        // We only expect binary messages for GBA link
    });

    dataChannel->onClosed([]() {
        std::cout << "WebRTC DataChannel closed." << std::endl;
    });
}

void WebRTCTransport::SendData(const uint8_t* data, size_t size) {
    if (dataChannel && dataChannel->isOpen()) {
        rtc::binary bin(data, data + size);
        dataChannel->send(bin);
    }
}
