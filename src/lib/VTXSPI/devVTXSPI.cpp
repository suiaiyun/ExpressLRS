#if defined(GPIO_PIN_SPI_VTX_NSS)

#include "devVTXSPI.h"
#include "targets.h"
#include "common.h"
#include "helpers.h"
#include "hwTimer.h"
#include "logging.h"
#include <SPI.h>
#if defined(PLATFORM_ESP32)
#include <pwmWrite.h>
#endif

#define SYNTHESIZER_REGISTER_A                  0x00
#define SYNTHESIZER_REGISTER_B                  0x01
#define SYNTHESIZER_REGISTER_C                  0x02
#define RF_VCO_DFC_CONTROL_REGISTER             0x03
#define VCO_CONTROL_REGISTER                    0x04
#define VCO_CONTROL_REGISTER_CONT               0x05
#define AUDIO_MODULATOR_CONTROL_REGISTER        0x06
#define PRE_DRIVER_AND_PA_CONTROL_REGISTER      0x07
#define STATE_REGISTER                          0x0F

#define SYNTH_REG_A_DEFAULT                     0x0190

#define POWER_AMP_ON                            0b00000100111110111111
#define POWER_AMP_OFF                           0x00
// ESP32 DAC pins are 0-255
#define MIN_DAC                                 1 // Testing required.
#define MAX_DAC                                 250 // Absolute max is 255.  But above 250 does nothing.
// PWM is 0-4095
#define MIN_PWM                                 1000 // Testing required.
#define MAX_PWM                                 3600 // Absolute max is 4095.  But above 3500 does nothing.

#define VPD_BUFFER                              5

#define READ_BIT                                0x00
#define WRITE_BIT                               0x01

#define RTC6705_BOOT_DELAY                      350
#define RTC6705_PLL_SETTLE_TIME_MS              500 // ms - after changing frequency turn amps back on after this time for clean switching
#define VTX_POWER_INTERVAL_MS                   20

#define BUF_PACKET_SIZE                         4 // 25b packet in 4 bytes

uint8_t vtxSPIBandChannelIdx = 255;
static uint8_t vtxSPIBandChannelIdxCurrent = 255;
uint8_t vtxSPIPowerIdx = 0;
static uint8_t vtxSPIPowerIdxCurrent = 0;
uint8_t vtxSPIPitmode = 1;
static uint8_t RfAmpVrefState = 0;
static uint16_t vtxSPIPWM = MAX_PWM;
static uint16_t vtxMinPWM = MAX_PWM;
static uint16_t vtxMaxPWM = MIN_PWM;
static uint16_t VpdSetPoint = 0;
static uint16_t Vpd = 0;

#define VPD_SETPOINT_0_MW                       0
#define VPD_SETPOINT_YOLO_MW                    2000
#if defined(TARGET_UNIFIED_RX)
const uint16_t *VpdSetPointArray25mW = nullptr;
const uint16_t *VpdSetPointArray100mW = nullptr;
#else
uint16_t VpdSetPointArray25mW[] = VPD_VALUES_25MW;
uint16_t VpdSetPointArray100mW[] = VPD_VALUES_100MW;
#endif

uint16_t VpdFreqArray[] = {5650, 5750, 5850, 5950};
uint8_t VpdSetPointCount =  ARRAY_SIZE(VpdFreqArray);

static const uint16_t freqTable[48] = {
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // B
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // E
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // F
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // R
    5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613  // L
};

#if defined(PLATFORM_ESP32)
static Pwm pwm;
#endif
static SPIClass *vtxSPI;
static void rtc6705WriteRegister(uint32_t regData)
{
    // When sharing the SPI Bus control of the NSS pin is done by us
    if (GPIO_PIN_SPI_VTX_SCK == GPIO_PIN_SCK)
    {
        vtxSPI->setBitOrder(LSBFIRST);
        digitalWrite(GPIO_PIN_SPI_VTX_NSS, LOW);
    }

    #if defined(PLATFORM_ESP32)
        vtxSPI->transferBits(regData, nullptr, 25);
    #else
        uint8_t buf[BUF_PACKET_SIZE];
        memcpy(buf, (byte *)&regData, BUF_PACKET_SIZE);
        vtxSPI->transfer(buf, BUF_PACKET_SIZE);
    #endif

    if (GPIO_PIN_SPI_VTX_SCK == GPIO_PIN_SCK)
    {
        digitalWrite(GPIO_PIN_SPI_VTX_NSS, HIGH);
        vtxSPI->setBitOrder(MSBFIRST);
    }
}

static void rtc6705ResetSynthRegA()
{
  uint32_t regData = SYNTHESIZER_REGISTER_A | (WRITE_BIT << 4) | (SYNTH_REG_A_DEFAULT << 5);
  rtc6705WriteRegister(regData);
}

static void rtc6705SetFrequency(uint32_t freq)
{
    rtc6705ResetSynthRegA();

    VTxOutputMinimum(); // Set power to zero for clear channel switching

    uint32_t f = 25 * freq;
    uint32_t SYN_RF_N_REG = f / 64;
    uint32_t SYN_RF_A_REG = f % 64;

    uint32_t regData = SYNTHESIZER_REGISTER_B | (WRITE_BIT << 4) | (SYN_RF_A_REG << 5) | (SYN_RF_N_REG << 12);
    rtc6705WriteRegister(regData);
}

static void rtc6705SetFrequencyByIdx(uint8_t idx)
{
    rtc6705SetFrequency((uint32_t)freqTable[idx]);
}

static void rtc6705PowerAmpOn()
{
    uint32_t regData = PRE_DRIVER_AND_PA_CONTROL_REGISTER | (WRITE_BIT << 4) | (POWER_AMP_ON << 5);
    rtc6705WriteRegister(regData);
}

static void RfAmpVrefOn()
{
    if (!RfAmpVrefState) digitalWrite(GPIO_PIN_RF_AMP_VREF, HIGH);

    RfAmpVrefState = 1;
}

static void RfAmpVrefOff()
{
    if (RfAmpVrefState) digitalWrite(GPIO_PIN_RF_AMP_VREF, LOW);

    RfAmpVrefState = 0;
}

static void setPWM()
{
#if defined(PLATFORM_ESP32)
    if (GPIO_PIN_RF_AMP_PWM == 25 || GPIO_PIN_RF_AMP_PWM == 26)
    {
        dacWrite(GPIO_PIN_RF_AMP_PWM, vtxSPIPWM >> 4);
    }
    else
    {
        pwm.write(GPIO_PIN_RF_AMP_PWM, vtxSPIPWM);
    }
#else
    analogWrite(GPIO_PIN_RF_AMP_PWM, vtxSPIPWM);
#endif
}

void VTxOutputMinimum()
{
    RfAmpVrefOff();

    vtxSPIPWM = vtxMaxPWM;
    setPWM();
}

static void VTxOutputIncrease()
{
    if (vtxSPIPWM > vtxMinPWM) vtxSPIPWM -= 1;
    setPWM();
}

static void VTxOutputDecrease()
{
    if (vtxSPIPWM < vtxMaxPWM) vtxSPIPWM += 1;
    setPWM();
}

static uint16_t LinearInterpVpdSetPointArray(const uint16_t VpdSetPointArray[])
{
    uint16_t newVpd = 0;
    uint16_t f = freqTable[vtxSPIBandChannelIdxCurrent];

    if (f <= VpdFreqArray[0])
    {
        newVpd = VpdSetPointArray[0];
    }
    else if (f >= VpdFreqArray[VpdSetPointCount - 1])
    {
        newVpd = VpdSetPointArray[VpdSetPointCount - 1];
    }
    else
    {
        for (uint8_t i = 0; i < (VpdSetPointCount - 1); i++)
        {
            if (f < VpdFreqArray[i + 1])
            {
                newVpd = VpdSetPointArray[i] + ((VpdSetPointArray[i + 1]-VpdSetPointArray[i])/(VpdFreqArray[i + 1]-VpdFreqArray[i])) * (f - VpdFreqArray[i]);
            }
        }
    }

    return newVpd;
}

static void SetVpdSetPoint()
{
    switch (vtxSPIPowerIdx)
    {
    case 1: // 0 mW
        VpdSetPoint = VPD_SETPOINT_0_MW;
        return;

    case 2: // 25 mW
        VpdSetPoint = LinearInterpVpdSetPointArray(VpdSetPointArray25mW);
        return;

    case 3: // 100 mW
        VpdSetPoint = LinearInterpVpdSetPointArray(VpdSetPointArray100mW);
        return;

    default: // YOLO mW
        VpdSetPoint = VPD_SETPOINT_YOLO_MW;
        return;
    }
}

static void checkOutputPower()
{
    if (vtxSPIPitmode)
    {
        VTxOutputMinimum();
    }
    else
    {
        RfAmpVrefOn();

        uint16_t VpdReading = analogRead(GPIO_PIN_RF_AMP_VPD); // WARNING - Max input 1.0V !!!!

        Vpd = (8 * Vpd + 2 * VpdReading) / 10; // IIR filter

        if (Vpd < (VpdSetPoint - VPD_BUFFER))
        {
            VTxOutputIncrease();
        }
        else if (Vpd > (VpdSetPoint + VPD_BUFFER))
        {
            VTxOutputDecrease();
        }
    }
}

static void initialize()
{
    #if defined(TARGET_UNIFIED_RX)
    VpdSetPointArray25mW = VPD_VALUES_25MW;
    VpdSetPointArray100mW = VPD_VALUES_100MW;
    #endif

    if (GPIO_PIN_SPI_VTX_NSS != UNDEF_PIN)
    {
        if (GPIO_PIN_SPI_VTX_SCK != UNDEF_PIN && GPIO_PIN_SPI_VTX_SCK != GPIO_PIN_SCK)
        {
            vtxSPI = new SPIClass();
            #if defined(PLATFORM_ESP32)
            vtxSPI->begin(GPIO_PIN_SPI_VTX_SCK, GPIO_PIN_SPI_VTX_MISO, GPIO_PIN_SPI_VTX_MOSI, GPIO_PIN_SPI_VTX_NSS);
            #else
            vtxSPI->pins(GPIO_PIN_SPI_VTX_SCK, GPIO_PIN_SPI_VTX_MISO, GPIO_PIN_SPI_VTX_MOSI, GPIO_PIN_SPI_VTX_NSS);
            vtxSPI->begin();
            #endif
            vtxSPI->setHwCs(true);
            vtxSPI->setBitOrder(LSBFIRST);
        }
        else
        {
            vtxSPI = &SPI;
            pinMode(GPIO_PIN_SPI_VTX_NSS, OUTPUT);
            digitalWrite(GPIO_PIN_SPI_VTX_NSS, HIGH);
        }

        pinMode(GPIO_PIN_RF_AMP_VREF, OUTPUT);
        digitalWrite(GPIO_PIN_RF_AMP_VREF, LOW);

        #if defined(PLATFORM_ESP8266)
            pinMode(GPIO_PIN_RF_AMP_PWM, OUTPUT);
            analogWriteFreq(10000); // 10kHz
            analogWriteResolution(12); // 0 - 4095
        #else
            // If using a DAC pin then adjust min/max and initial value
            if (GPIO_PIN_RF_AMP_PWM == 25 || GPIO_PIN_RF_AMP_PWM == 26)
            {
                vtxMinPWM = MIN_DAC;
                vtxMaxPWM = MAX_DAC;
                vtxSPIPWM = vtxMaxPWM;
            }
            else
            {
                pwm.writeFrequency(GPIO_PIN_RF_AMP_PWM, 10000); // 10kHz
                pwm.writeResolution(12); // 0 - 4095
            }
        #endif
        setPWM();

        delay(RTC6705_BOOT_DELAY);
    }
}

static int start()
{
    if (GPIO_PIN_SPI_VTX_NSS == UNDEF_PIN)
    {
        return DURATION_NEVER;
    }

    rtc6705SetFrequency(5999); // Boot with VTx set away from standard frequencies.

    rtc6705PowerAmpOn();

    return VTX_POWER_INTERVAL_MS;
}

static int event()
{
    if (GPIO_PIN_SPI_VTX_NSS == UNDEF_PIN)
    {
        return DURATION_NEVER;
    }

    if (vtxSPIBandChannelIdxCurrent != vtxSPIBandChannelIdx)
    {
        return DURATION_IMMEDIATELY;
    }

    return DURATION_IGNORE;
}

static int timeout()
{
    if (GPIO_PIN_SPI_VTX_NSS == UNDEF_PIN)
    {
        return DURATION_NEVER;
    }

    if (!hwTimer::isTick) // Only run spi and analog reads during rx free time.
    {
        return DURATION_IMMEDIATELY;
    }

    if (vtxSPIBandChannelIdxCurrent != vtxSPIBandChannelIdx)
    {
        rtc6705SetFrequencyByIdx(vtxSPIBandChannelIdx);
        vtxSPIBandChannelIdxCurrent = vtxSPIBandChannelIdx;

        INFOLN("VTx set frequency...");

        return RTC6705_PLL_SETTLE_TIME_MS;
    }
    else
    {
        if (vtxSPIPowerIdxCurrent != vtxSPIPowerIdx)
        {
            SetVpdSetPoint();
            vtxSPIPowerIdxCurrent = vtxSPIPowerIdx;
        }

        checkOutputPower();

        return VTX_POWER_INTERVAL_MS;
    }
}

device_t VTxSPI_device = {
    .initialize = initialize,
    .start = start,
    .event = event,
    .timeout = timeout
};

#endif
