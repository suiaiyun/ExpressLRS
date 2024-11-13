#if defined(TARGET_RX)

#include "SerialAirPort.h"
#include "device.h"
#include "common.h"

// Variables / constants for Airport //
FIFO<AP_MAX_BUF_LEN> apInputBuffer;
FIFO<AP_MAX_BUF_LEN> apOutputBuffer;


uint32_t SerialAirPort::sendRCFrame(bool frameAvailable, bool frameMissed, uint32_t *channelData)
{
    return DURATION_IMMEDIATELY;
}

int SerialAirPort::getMaxSerialReadSize()
{
    return AP_MAX_BUF_LEN - apInputBuffer.size();
}

void SerialAirPort::processBytes(uint8_t *bytes, u_int16_t size)
{
    if (connectionState == connected)
    {
        apInputBuffer.atomicPushBytes(bytes, size);
    }
}

void SerialAirPort::sendQueuedData(uint32_t maxBytesToSend)
{
    auto size = apOutputBuffer.size();
    if (size != 0)
    {
        uint8_t buf[size];
        apOutputBuffer.lock();
        apOutputBuffer.popBytes(buf, size);
        apOutputBuffer.unlock();
        _outputPort->write(buf, size);
    }
}
#endif
