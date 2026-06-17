#ifndef GBA_SIO_NETWORK_DRIVER_H
#define GBA_SIO_NETWORK_DRIVER_H

#include <mgba/gba/interface.h>
#include <mgba/internal/gba/sio.h>
#include <mgba/core/thread.h>
#include "WebRTCTransport.h"

// Forward declaration
struct GBASIONetworkDriver;

#ifdef __cplusplus
extern "C" {
#endif

// C interface for mGBA to attach our C++ network driver
void GBASIONetworkDriverCreate(struct GBASIONetworkDriver* driver, WebRTCTransport* transport, struct mCoreThread* thread);

#ifdef __cplusplus
}
#endif

struct GBASIONetworkDriver {
    struct GBASIODriver d;
    WebRTCTransport* transport;
    struct mCoreThread* thread;
    
    uint16_t localData;
    uint16_t remoteData;
    bool dataReceived;
    
    // Mutex for thread-safe waking
    Mutex mutex;
    Condition cond;
};

#endif // GBA_SIO_NETWORK_DRIVER_H
