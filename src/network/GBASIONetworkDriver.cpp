#include "GBASIONetworkDriver.h"
#include <iostream>

static bool NetworkDriverInit(struct GBASIODriver* driver) {
    struct GBASIONetworkDriver* netDriver = (struct GBASIONetworkDriver*)driver;
    netDriver->localData = 0xFFFF;
    netDriver->remoteData = 0xFFFF;
    netDriver->dataReceived = false;
    MutexInit(&netDriver->mutex);
    ConditionInit(&netDriver->cond);

    // Wire up WebRTC data receive callback
    if (netDriver->transport) {
        netDriver->transport->SetDataReceivedCallback([netDriver](const uint8_t* data, size_t size) {
            if (size >= sizeof(uint16_t)) {
                MutexLock(&netDriver->mutex);
                netDriver->remoteData = *reinterpret_cast<const uint16_t*>(data);
                netDriver->dataReceived = true;
                ConditionWake(&netDriver->cond);
                MutexUnlock(&netDriver->mutex);

                // Wake the core thread if it's sleeping
                if (netDriver->thread) {
                    mCoreThreadStopWaiting(netDriver->thread);
                }
            }
        });
    }
    return true;
}

static void NetworkDriverDeinit(struct GBASIODriver* driver) {
    struct GBASIONetworkDriver* netDriver = (struct GBASIONetworkDriver*)driver;
    ConditionDeinit(&netDriver->cond);
    MutexDeinit(&netDriver->mutex);
}

static bool NetworkDriverStart(struct GBASIODriver* driver) {
    struct GBASIONetworkDriver* netDriver = (struct GBASIONetworkDriver*)driver;
    
    // We send our local data via WebRTC
    if (netDriver->transport) {
        uint16_t dataToSend = netDriver->localData;
        netDriver->transport->SendData(reinterpret_cast<const uint8_t*>(&dataToSend), sizeof(dataToSend));
    }

    // Now we wait for the remote data to arrive
    MutexLock(&netDriver->mutex);
    while (!netDriver->dataReceived) {
        if (netDriver->thread) {
            // Signal the mGBA core to pause while we wait for network
            mCoreThreadWaitFromThread(netDriver->thread);
        }
        ConditionWait(&netDriver->cond, &netDriver->mutex);
    }
    netDriver->dataReceived = false;
    MutexUnlock(&netDriver->mutex);

    return true;
}

static void NetworkDriverFinishMultiplayer(struct GBASIODriver* driver, uint16_t data[4]) {
    struct GBASIONetworkDriver* netDriver = (struct GBASIONetworkDriver*)driver;
    // In a 2-player setup, Player 1 is data[0], Player 2 is data[1].
    // For simplicity, we just swap local and remote based on our ID.
    // Real implementation would assign node IDs based on who hosted.
    data[0] = netDriver->localData;
    data[1] = netDriver->remoteData;
    data[2] = 0xFFFF;
    data[3] = 0xFFFF;
}

static uint16_t NetworkDriverWriteSIOCNT(struct GBASIODriver* driver, uint16_t value) {
    // Basic passthrough for standard Multiplayer mode SIOCNT
    return value;
}

static bool NetworkDriverHandlesMode(struct GBASIODriver* driver, enum GBASIOMode mode) {
    return mode == GBA_SIO_MULTI;
}

void GBASIONetworkDriverCreate(struct GBASIONetworkDriver* netDriver, WebRTCTransport* transport, struct mCoreThread* thread) {
    netDriver->d.init = NetworkDriverInit;
    netDriver->d.deinit = NetworkDriverDeinit;
    netDriver->d.start = NetworkDriverStart;
    netDriver->d.finishMultiplayer = NetworkDriverFinishMultiplayer;
    netDriver->d.handlesMode = NetworkDriverHandlesMode;
    netDriver->d.writeSIOCNT = NetworkDriverWriteSIOCNT;

    netDriver->transport = transport;
    netDriver->thread = thread;
}
