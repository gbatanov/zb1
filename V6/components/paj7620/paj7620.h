#ifndef PAJ7620_H
#define PAJ7620_H

#include "light_driver.h"
#include "sdkconfig.h"

extern const char *TAG;
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
#define PAJ_WAVE 0x3

#define CHANGE_BANK_ADDR 0xEF
#define BANK0 0x00
#define BANK1 0x01

// Set the mode at Bank 1
#define MODE_ADDR 0x65
#define NORMAL_MODE 0xB7 // normal (far) mode 120 fps
#define GAMING_MODE 0x12 // gaming (near) mode 240 fps

#define GESTURE_DURATION CONFIG_GESTURE_DURATION

void gesture_task(void *arg);

#endif