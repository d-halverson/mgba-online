#ifndef WEBRTC_TRANSPORT_H
#define WEBRTC_TRANSPORT_H

#include <string>
#include <functional>
#include <memory>
#include <rtc/rtc.hpp>

class WebRTCTransport {
public:
    WebRTCTransport();
    ~WebRTCTransport();

    // Callback when local SDP or ICE candidates are generated and need to be sent via Discord Metadata
    void SetLocalDescriptionCallback(std::function<void(const std::string&)> callback) {
        onLocalDescription = callback;
    }

    // Callback when data is received over the link cable channel
    void SetDataReceivedCallback(std::function<void(const uint8_t*, size_t)> callback) {
        onDataReceived = callback;
    }

    // Initialize as Host (generates Offer)
    void InitializeAsHost();

    // Initialize as Guest using the Host's Offer
    void InitializeAsGuest(const std::string& remoteOffer);

    // Provide the Answer back to the Host
    void AcceptAnswer(const std::string& remoteAnswer);

    // Send data over the link cable
    void SendData(const uint8_t* data, size_t size);

private:
    void SetupDataChannel();

    std::shared_ptr<rtc::PeerConnection> peerConnection;
    std::shared_ptr<rtc::DataChannel> dataChannel;
    
    std::function<void(const std::string&)> onLocalDescription;
    std::function<void(const uint8_t*, size_t)> onDataReceived;

    rtc::Configuration config;
};

#endif // WEBRTC_TRANSPORT_H
