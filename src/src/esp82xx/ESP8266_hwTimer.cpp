#include "HwTimer.h"
#include <user_interface.h>

void HwTimer::init()
{
    noInterrupts();
    timer1_attachInterrupt(HwTimer::callback);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); //5MHz ticks
    HwTimer::setTime(HwTimer::HWtimerInterval);
    interrupts();
}

void HwTimer::stop()
{
    timer1_detachInterrupt();
}

void HwTimer::pause()
{
    timer1_detachInterrupt();
}

void HwTimer::setTime(uint32_t time)
{
    timer1_write(time * 5);
}
