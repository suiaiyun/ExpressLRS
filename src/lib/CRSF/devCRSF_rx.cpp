#include "targets.h"
#include "devCRSF.h"

#ifdef CRSF_RX_MODULE
extern CRSF crsf;

static volatile bool sendFrame = false;
extern void HandleUARTin();

void ICACHE_RAM_ATTR crsfRCFrameAvailable()
{
    #if defined(PLATFORM_ESP32)
    sendFrame = true;
    #else
    crsf.sendRCFrameToFC();
    #endif
}

static int start()
{
    return DURATION_IMMEDIATELY;
}

static int timeout()
{
    #if defined(PLATFORM_ESP32)
    if (sendFrame)
    {
        sendFrame = false;
        crsf.sendRCFrameToFC();
    }
    #endif
    crsf.RXhandleUARTout();
    HandleUARTin();

#if defined(PLATFORM_ESP32)
    // Flush the LOGGING_UART here so all serial IO is done in the same context
    // and does not crash the ESP32. Doing it from ISR and the 2 cores will cause
    // it to crash! So we use a buffer stream and flush here.
    LOGGING_UART.flush();
#endif
    return DURATION_IMMEDIATELY;
}

device_t CRSF_device = {
    .initialize = nullptr,
    .start = start,
    .event = nullptr,
    .timeout = timeout
};
#endif
