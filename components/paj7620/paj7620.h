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

// #define _I2C_NUMBER(num) I2C_NUM_##num
// #define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */

// #define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL        /*!< gpio number for I2C master clock */
// #define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA        /*!< gpio number for I2C master data  */
// #define I2C_MASTER_NUM I2C_NUM                         /*!< I2C port number for master dev */
// #define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
// #define I2C_MASTER_TX_BUF_DISABLE 0                    /*!< I2C master doesn't need buffer */
// #define I2C_MASTER_RX_BUF_DISABLE 0                    /*!< I2C master doesn't need buffer */
#define I2C_MASTER_FREQ_HZ 400000 // I2C clock of PAJ7620 can run at 400 kHz max.??

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */

#define GESTURE_SENSOR_ADDR CONFIG_GESTURE_SENSOR_ADDR

#define SHIFT(n, shift) (n << shift)

// Gesture detection interrupt flags.
#define PAJ_INT_FLAG1 0x43
#define PAJ_RIGHT SHIFT(1, 0)
#define PAJ_LEFT SHIFT(1, 1)
#define PAJ_UP SHIFT(1, 2)
#define PAJ_DOWN SHIFT(1, 3)
#define PAJ_FORWARD SHIFT(1, 4)
#define PAJ_BACKWARD SHIFT(1, 5)
#define PAJ_CLOCKWISE SHIFT(1, 6)
#define PAJ_COUNT_CLOCKWISE SHIFT(1, 7)
#define PAJ_WAVE 0x3

#define CHANGE_BANK_ADDR 0xEF
#define BANK0 0x00
#define BANK1 0x01

// Set the mode at Bank 1
#define MODE_ADDR 0x65
#define NORMAL_MODE 0xB7 // normal (far) mode 120 fps
#define GAMING_MODE 0x12 // gaming (near) mode 240 fps

#define GESTURE_DURATION CONFIG_GESTURE_DURATION

void i2c_test_task(void *arg);

#endif