#ifndef SERIAL_H_
#define SERIAL_H_

#include "FIFO.h"
#include <HardwareSerial.h>

class HwSerial : public HardwareSerial
{
public:
    HwSerial(int uart_nr, int32_t duplex_pin = -1);
    HwSerial(uint32_t _rx, uint32_t _tx, int32_t duplex_pin = -1);
    HwSerial(void *peripheral, int32_t duplex_pin = -1);

    void Begin(uint32_t baud, uint32_t config = SERIAL_8N1);

    void enable_receiver(void);
    void enable_transmitter(void);

    void flush_read()
    {
        HardwareSerial::flush();
        while (read() > 0)
        {
        }
    }

    size_t write(const uint8_t *buff, size_t len)
    {
        size_t ret;
#ifdef PLATFORM_ESP32
        vTaskDelay(1 / portTICK_PERIOD_MS);
#endif
        enable_transmitter();
        ret = HardwareSerial::write(buff, len);
        enable_receiver();
        return ret;
    }

    size_t write(FIFO &fifo)
    {
        size_t ret = 0;
        uint8_t peekVal = fifo.peek(); // check if we have data in the output FIFO that needs to be written
        if (peekVal > 0)
        {
            if (fifo.size() >= peekVal)
            {
                uint8_t OutPktLen = fifo.pop();
                uint8_t OutData[OutPktLen];

                fifo.popBytes(OutData, OutPktLen);
                ret = HwSerial::write(OutData, OutPktLen);
            }
        }
        return ret;
    }

private:
    int32_t duplex_pin;
};

extern HwSerial CrsfSerial;

#endif /* SERIAL_H_ */
