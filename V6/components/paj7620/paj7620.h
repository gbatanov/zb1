#ifndef PAJ7620_H
#define PAJ7620_H

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "light_driver.h"
#include "sdkconfig.h"

extern const char *TAG;

#define ESP_INTR_FLAG_DEFAULT 0

#ifndef I2C_NUM
#define I2C_NUM I2C_NUM_0
#endif
// #define I2C_NUM_0 I2C_NUM
#define I2C_GESTURE_ADDRESS CONFIG_GESTURE_SENSOR_ADDR
#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_TICKS_TO_WAIT 100

#define DATA_LENGTH 512                  // Data buffer length of test buffer
#define RW_TEST_LENGTH 128               // Data length for r/w test, [0,DATA_LENGTH]
#define DELAY_TIME_BETWEEN_ITEMS_MS 2000 // delay time between different test items

#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQ_HZ // I2C clock of PAJ7620 can run at 400 kHz max
#define GESTURE_SENSOR_ADDR CONFIG_GESTURE_SENSOR_ADDR

#define GESTURE_MODE 0
#define PROXIMITY_MODE 1
#define NORMAL_SPEED_MODE 0
#define GAMING_SPEED_MODE 1


// Gesture detection interrupt flags.
#define PAJ_INT_FLAG1 0x43
#define PAJ_RIGHT 0x1
#define PAJ_LEFT 0x2
#define PAJ_UP 0x4
#define PAJ_DOWN 0x8
#define PAJ_FORWARD 0x10
#define PAJ_BACKWARD 0x20
#define PAJ_CLOCKWISE 0x40
#define PAJ_COUNT_CLOCKWISE 0x80
#define PAJ_WAVE 0x0100

#define PAJ7620_ADDR_BASE 0x00
#define PAJ7620_ADDR_PS_APPROACH_STATE (PAJ7620_ADDR_BASE + 0x6B)
#define PAJ7620_ADDR_S_AVE_Y_BRIGHTNESS (PAJ7620_ADDR_BASE + 0x6C)

#define CHANGE_BANK_ADDR 0xEF
#define BANK0 0x00
#define BANK1 0x01

// Set the R_IDLE_TIME at Bank 1
#define R_IDLE_TIME 0x65  // в доке это  R_IDLE_TIME[7:0] default 0xB4
#define NORMAL_SPEED 0xB7 // normal (far) mode 120 fps
#define GAMING_SPEED 0x12 // gaming (near) mode 240 fps

#define GESTURE_DURATION CONFIG_GESTURE_DURATION

typedef struct
{
    i2c_master_dev_handle_t dev_handle;
    uint32_t intPin; // пин, на который повешено прерывание
    uint8_t mode;
    uint8_t speed;
    bool available;
} Dev_PAJ7620;

void gesture_task(void *arg);
esp_err_t paj7620_init(Dev_PAJ7620 *dev);
void i2c_bus_add_paj7620(Dev_PAJ7620 *devPaj7620);

#endif