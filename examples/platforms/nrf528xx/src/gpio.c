/**
 * @file
 *   This file implements the system abstraction for GPIO and GPIOTE.
 *
 */

#define BUTTON_GPIO_PORT 0x50000300UL
#define BUTTON_PIN 11 // button #1

#define GPIO_LOGIC_HI 0
#define GPIO_LOGIC_LOW 1

#define NRFX_TWIM_ENABLED 1
#define NRFX_TWI_ENABLED 1
#define TWI_ENABLED 1
#define TWI0_ENABLED 1
#define TWI0_USE_EASY_DMA 1
#define NRFX_TWIM0_ENABLED 1

#define LED_GPIO_PORT 0x50000300UL
#define LED_1_PIN 13 // turn on to indicate leader role
#define LED_2_PIN 14 // turn on to indicate router role
#define LED_3_PIN 15 // turn on to indicate child role
#define LED_4_PIN 16 // turn on to indicate UDP receive

/* TWI instance ID. */
#define TWIM_INSTANCE_ID     0

#define ARDUINO_SCL_PIN             27    // SCL signal pin
#define ARDUINO_SDA_PIN             26    // SDA signal pin

/* Header for the functions defined here */
#include "openthread-system.h"

#include <string.h>

/* Header to access an OpenThread instance */
#include <openthread/instance.h>

/* Headers for lower-level nRF52840 functions */
#include "platform-nrf5.h"
#include "hal/nrf_gpio.h"
#include "hal/nrf_gpiote.h"
#include "nrfx/drivers/include/nrfx_gpiote.h"
#include "hal/nrf_temp.h"
#include "nrfx/drivers/include/nrfx_twim.h"

void twim_init (void);
uint32_t boardGetChipTemperature(void);
uint32_t boardGetTwiTemperature(void);
int32_t boardInit(void);
void twi_handler(nrfx_twim_evt_t const * p_event, void * p_context);
static void in_pin1_handler(uint32_t pin, nrf_gpiote_polarity_t action);

/* Declaring callback function for button 1. */
static otSysButtonCallback sButtonHandler;
static bool                sButtonPressed;
/* TWI instance. */
static const nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(TWIM_INSTANCE_ID);
/* Buffer for samples read from temperature sensor. */
static uint8_t m_sample;
/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

static uint32_t twiTemp;

uint32_t boardGetChipTemperature(void) {
    // board temp is in .25degree c units
    //convert to milldegrees Farenheit
    uint32_t temp = nrf5TempGet();
    temp = ((temp*9*250)/5)+32000;
  return(temp);
}

uint32_t boardGetTwiTemperature(void) {
    // board temp is in .25degree c units
    //convert to milldegrees Farenheit
    uint32_t temp = twiTemp;
    temp = ((temp*9*250)/5)+32000;
  return(temp);
}

int32_t boardInit(void) {
    twim_init();
    return(1);
}

__STATIC_INLINE void data_handler(uint8_t temp)
{
    twiTemp = temp;
}

/**
 * @brief TWI events handler.
 */
void twi_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRFX_TWIM_EVT_DONE:
            if (p_event->xfer_desc.type == NRFX_TWIM_XFER_RX)
            {
                data_handler(m_sample);
            }
            m_xfer_done = true;
            break;
        default:
            break;
    }
}


void twim_init (void)
{
    nrfx_err_t err_code;

    const nrfx_twim_config_t twim_config = {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_TWIM_FREQ_100K,
       .interrupt_priority = NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY,
       .hold_bus_uninit     = false
    };

    err_code = nrfx_twim_init(&m_twim, &twim_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrfx_twim_enable(&m_twim);
}



/**
 * @brief Function to receive interrupt and call back function
 * set by the application for button 1.
 *
 */
static void in_pin1_handler(uint32_t pin, nrf_gpiote_polarity_t action)
{
    OT_UNUSED_VARIABLE(pin);
    OT_UNUSED_VARIABLE(action);
    sButtonPressed = true;
}

/**
 * @brief Function for configuring: PIN_IN pin for input, PIN_OUT pin for output,
 * and configures GPIOTE to give an interrupt on pin change.
 */

void otSysLedInit(void)
{
    /* Configure GPIO mode: output */
    nrf_gpio_cfg_output(LED_1_PIN);
    nrf_gpio_cfg_output(LED_2_PIN);
    nrf_gpio_cfg_output(LED_3_PIN);
    nrf_gpio_cfg_output(LED_4_PIN);
    
    /* Clear all output first */
    nrf_gpio_pin_write(LED_1_PIN, GPIO_LOGIC_LOW);
    nrf_gpio_pin_write(LED_2_PIN, GPIO_LOGIC_LOW);
    nrf_gpio_pin_write(LED_3_PIN, GPIO_LOGIC_LOW);
    nrf_gpio_pin_write(LED_4_PIN, GPIO_LOGIC_LOW);
    
    /* Initialize gpiote for button(s) input.
     Button event handlers are set in the application (main.c) */
    ret_code_t err_code;
    err_code = nrfx_gpiote_init();
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function to set the mode of an LED.
 */

void otSysLedSet(uint8_t aLed, bool aOn)
{
    switch (aLed)
    {
    case 1:
        nrf_gpio_pin_write(LED_1_PIN, (aOn == GPIO_LOGIC_HI));
        break;
    case 2:
        nrf_gpio_pin_write(LED_2_PIN, (aOn == GPIO_LOGIC_HI));
        break;
    case 3:
        nrf_gpio_pin_write(LED_3_PIN, (aOn == GPIO_LOGIC_HI));
        break;
    case 4:
        nrf_gpio_pin_write(LED_4_PIN, (aOn == GPIO_LOGIC_HI));
        break;
    }
}

/**
 * @brief Function to toggle the mode of an LED.
 */
void otSysLedToggle(uint8_t aLed)
{
    switch (aLed)
    {
    case 1:
        nrf_gpio_pin_toggle(LED_1_PIN);
        break;
    case 2:
        nrf_gpio_pin_toggle(LED_2_PIN);
        break;
    case 3:
        nrf_gpio_pin_toggle(LED_3_PIN);
        break;
    case 4:
        nrf_gpio_pin_toggle(LED_4_PIN);
        break;
    }
}

/**
 * @brief Function to toggle the mode of an LED.
 */
void otSysButtonInit(otSysButtonCallback aCallback)
{
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    in_config.pull                    = NRF_GPIO_PIN_PULLUP;

    ret_code_t err_code;
    err_code = nrfx_gpiote_in_init(BUTTON_PIN, &in_config, in_pin1_handler);
    APP_ERROR_CHECK(err_code);

    sButtonHandler = aCallback;
    sButtonPressed = false;

    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);
}

void otSysButtonProcess(otInstance *aInstance)
{
    if (sButtonPressed)
    {
        sButtonPressed = false;
        sButtonHandler(aInstance);
    }
}
