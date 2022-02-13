#include <stddef.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// define macros to use the GPIO pins for the LED and button
#define GPIO_LED 2
#define GPIO_LED_PIN_SEL (1ULL << GPIO_LED)
#define GPIO_BUTTON 5
#define GPIO_BUTTON_PIN_SEL (1ULL << GPIO_BUTTON)
#define ESP_INTR_FLAG_DEFAULT 0

static void button_handler(void *arg);

// hardware init function
static void init_hw(void)
{
    // gpio_config_t is the struct used to set config parameters for a GPIO pin
    gpio_config_t io_conf;

    // configure LED as an output
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_LED_PIN_SEL;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // configure button as input along with its button-pressed handling
    // interrupt service routine (ISR)
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_BUTTON_PIN_SEL;
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // trigger the ISR handler when button signal is LOW
    io_conf.pull_up_en = 1;                // use the internal pull-up resistor - button press = LOW
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);         // initialize the ISR service
    gpio_isr_handler_add(GPIO_BUTTON, button_handler, NULL); // add the button handler function tot he ISR service
}

static TickType_t next = 0;
static bool led_state = false;

/*
    IRAM_ATTR instructs the compiler to place this block of code in
    the internal Random Access Memory (RAM) of the ESP32.

    This is required because this function is an ISR handler
    so needs to be short & quick. Otherwise the code will be loaded
    from flash memory, which is slower and could cause the application
    to crash.
*/
static void IRAM_ATTR button_handler(void *arg)
{
    /*
        xTaskGetTickCountFromISR is a function from the FreeRTOS task.h library.
        It returns the count of ticks since the FreeRTOS scheduler was started.

        We need this to prevent the button-debouncing effect, which would cause
        flickering of the LED when the button is pressed.

        It puts a short time buffer between 2 button presses to ensure they are 2 separate,
        intentional button presses instead of the debouncing effect.
    */
    TickType_t now = xTaskGetTickCountFromISR();

    if (now > next)
    {
        led_state = !led_state;
        gpio_set_level(GPIO_LED, led_state);
        next = now + 500 / portTICK_PERIOD_MS; // 500ms
    }
}

void app_main()
{
    init_hw();

    vTaskSuspend(NULL); // suspend main taks to prevent application exit
}