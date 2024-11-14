#ifndef DEVICE_NAME
#define DEVICE_NAME          "ElfinRC Rx"
#endif

#define TARGET_RX_MLRS

// GPIO pin definitions
#define GPIO_PIN_NSS            PB0
#define GPIO_PIN_DIO1           PB3
#define GPIO_PIN_MOSI           PA7
#define GPIO_PIN_MISO           PA6
#define GPIO_PIN_SCK            PA5
#define GPIO_PIN_RST            PB5
#define GPIO_PIN_BUSY           PB4
#define GPIO_PIN_TX_ENABLE      PB12
#define GPIO_PIN_RX_ENABLE      PB8

#define GPIO_PIN_RCSIGNAL_RX    PA3  // UART2
#define GPIO_PIN_RCSIGNAL_TX    PA2  // UART2

#define GPIO_PIN_BUTTON         PA15 // active low
#define GPIO_PIN_LED_RED        PC13 // Right Red LED
#define GPIO_PIN_LED_GREEN      PC14 // Left Green LED
#define GPIO_PIN_DEBUG_RX       PB11 // UART3 (debug)
#define GPIO_PIN_DEBUG_TX       PB10 // UART3 (debug)

#define GPIO_PIN_UART2TX_INVERT PB9  // XOR chip

// Power output
#define MinPower                PWR_10mW
#define HighPower               PWR_250mW
#define MaxPower                PWR_500mW
#define POWER_OUTPUT_VALUES     {-15,-11,-7,-1,6}
