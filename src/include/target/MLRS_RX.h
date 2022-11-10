#if defined(RX_MINI)
    #ifndef DEVICE_NAME
        #define DEVICE_NAME          "MLRS Rx Mini"
    #endif
    #define TARGET_TX_MLRS_MINI
#else
    #ifndef DEVICE_NAME
        #define DEVICE_NAME          "MLRS E28 Rx"
    #endif
     #define TARGET_RX_MLRS
#endif

#define USE_SX1280_DCDC

#if defined(RX_MINI)
    // GPIO pin definitions
    #define GPIO_PIN_NSS            PA4
    #define GPIO_PIN_DIO1           PB11
    #define GPIO_PIN_MOSI           PA7
    #define GPIO_PIN_MISO           PA6
    #define GPIO_PIN_SCK            PA5
    #define GPIO_PIN_RST            PB12
    #define GPIO_PIN_TX_ENABLE      PB13
    #define GPIO_PIN_RX_ENABLE      PB0
    #define GPIO_PIN_RCSIGNAL_RX    PA3  // UART2
    #define GPIO_PIN_RCSIGNAL_TX    PA2  // UART2
    #define GPIO_PIN_BUTTON         PC13 // active low
    #define GPIO_PIN_LED_RED        PB4  // Right Red LED
    #define GPIO_LED_RED_INVERTED   1
    #define GPIO_PIN_LED_GREEN      PB5  // Left Green LED
    #define GPIO_LED_GREEN_INVERTED 1
    #define GPIO_PIN_DEBUG_RX       PB7 // UART1 (usb)
    #define GPIO_PIN_DEBUG_TX       PB6  // UART1 (usb)
#else
    // GPIO pin definitions
    #define GPIO_PIN_NSS            PB0
    #define GPIO_PIN_DIO1           PB3
    #define GPIO_PIN_MOSI           PA7
    #define GPIO_PIN_MISO           PA6
    #define GPIO_PIN_SCK            PA5
    #define GPIO_PIN_RST            PB5
    #define GPIO_PIN_TX_ENABLE      PB12
    #define GPIO_PIN_RX_ENABLE      PB8
    #define GPIO_PIN_RCSIGNAL_RX    PA3  // UART2
    #define GPIO_PIN_RCSIGNAL_TX    PA2  // UART2
    #define GPIO_PIN_BUZZER         PB7
    #define GPIO_PIN_BUTTON         PA15 // active low
    #define GPIO_PIN_LED_RED        PC13 // Right Red LED
    #define GPIO_LED_RED_INVERTED   1
    #define GPIO_PIN_LED_GREEN      PC14 // Left Green LED
    #define GPIO_LED_GREEN_INVERTED 1
    #define GPIO_PIN_DEBUG_RX       PA10 // UART1 (usb)
    #define GPIO_PIN_DEBUG_TX       PA9  // UART1 (usb)
    #define GPIO_PIN_UART2TX_INVERT PB9  // XOR chip
#endif

// Power output
#define MinPower                PWR_10mW
#define HighPower               PWR_100mW
#define MaxPower                PWR_250mW
#define POWER_OUTPUT_VALUES     {-15,-11,-7,-1,6}
