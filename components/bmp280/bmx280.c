
#include "bmx280.h"
#include "esp_log.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"

// [BME280] Register address of humidity least significant byte.
#define BMX280_REG_HUMI_LSB 0xFE
// [BME280] Register address of humidity most significant byte.
#define BMX280_REG_HUMI_MSB 0xFD

// Register address of temperature fraction significant byte.
#define BMX280_REG_TEMP_XSB 0xFC
// Register address of temperature least significant byte.
#define BMX280_REG_TEMP_LSB 0xFB
// Register address of temperature most significant byte.
#define BMX280_REG_TEMP_MSB 0xFA

// Register address of pressure fraction significant byte.
#define BMX280_REG_PRES_XSB 0xF9
// Register address of pressure least significant byte.
#define BMX280_REG_PRES_LSB 0xF8
// Register address of pressure most significant byte.
#define BMX280_REG_PRES_MSB 0xF7

// Register address of sensor configuration.
#define BMX280_REG_CONFIG 0xF5
// Register address of sensor measurement control.
#define BMX280_REG_MESCTL 0xF4
// Register address of sensor status.
#define BMX280_REG_STATUS 0xF3
// [BME280] Register address of humidity control.
#define BMX280_REG_HUMCTL 0xF2

// [BME280] Register address of calibration constants. (high bank)
#define BMX280_REG_CAL_HI 0xE1
// Register address of calibration constants. (low bank)
#define BMX280_REG_CAL_LO 0x88

// Register address for sensor reset.
#define BMX280_REG_RESET 0xE0
// Chip reset vector.
#define BMX280_RESET_VEC 0xB6

// Register address for chip identification number.
#define BMX280_REG_CHPID 0xD0
// Value of REG_CHPID for BME280
#define BME280_ID 0x60
// Value of REG_CHPID for BMP280 (Engineering Sample 1)
#define BMP280_ID0 0x56
// Value of REG_CHPID for BMP280 (Engineering Sample 2)
#define BMP280_ID1 0x57
// Value of REG_CHPID for BMP280 (Production) мой вариант
#define BMP280_ID2 0x58

#define I2C_MASTER_FREQ_HZ 400000 // дублируется в ssd1306_i2c.c
#define I2C_TICKS_TO_WAIT 100     //  дублируется в ssd1306.c

#define TAG "BMP280"
extern i2c_master_bus_handle_t bus_handle;

/**
 * Macro that identifies a chip id as BME280 or BMP280
 * @note Only use when the chip is verified to be either a BME280 or BMP280.
 * @see bmx280_verify
 * @param chip_id The chip id.
 */
#define bmx280_isBME(chip_id) ((chip_id) == BME280_ID)
/**
 * Macro to verify a the chip id matches with the expected values.
 * @note Use when the chip needs to be verified as a BME280 or BME280.
 * @see bmx280_isBME
 * @param chip_id The chip id.
 */
#define bmx280_verify(chip_id) (((chip_id) == BME280_ID) || ((chip_id) == BMP280_ID2) || ((chip_id) == BMP280_ID1) || ((chip_id) == BMP280_ID0))

/**
 * Returns false if the sensor was not found.
 * @param bmx280 The driver structure.
 */
#define bmx280_validate(bmx280) (!(bmx280->_address == 0xDE && bmx280->chip_id == 0xAD))

// Добавление устройства bmp280 на шину
void i2c_bus_add_bmp280(BMP280_t *dev, i2c_port_t i2c_num)
{
    ESP_LOGI(TAG, "New i2c driver is used");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_BMP280_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t i2c_dev_handle;
    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &i2c_dev_handle);
    if (err != ESP_OK)
    {
        dev->_i2c_dev_handle = NULL;
        return;
    }

    dev->_address = I2C_BMP280_ADDRESS;
    dev->_i2c_num = i2c_num;
    dev->_i2c_dev_handle = i2c_dev_handle;
    dev->chip_id = BMP280_ID2; // мой вариант
}

esp_err_t bmx280_init(BMP280_t *bmx280)
{

    esp_err_t error = i2c_master_probe(bus_handle, bmx280->_address, -1);
    if (error == ESP_OK)
    {

        error = bmx280_reset(bmx280);

        // Give the sensor 10 ms delay to reset.
        vTaskDelay(pdMS_TO_TICKS(10));

        // get chip id
        uint8_t chipid = 0;
        error = bmx280_read(bmx280, BMX280_REG_CHPID, &chipid, 1);
        if (error == ESP_OK)
        {
            ESP_LOGI(TAG, "Chip Id : 0x%02x", chipid);
        }

        // Read calibration data.
        bmx280_calibrate(bmx280);

        ESP_LOGI("bmx280", "Dumping calibration...");
        ESP_LOG_BUFFER_HEX("bmx280", &bmx280->cmps, sizeof(bmx280->cmps));
    }

    return error;
}

esp_err_t bmx280_configure(BMP280_t *bmx280, bmx280_config_t *cfg)
{

    if (bmx280 == NULL || cfg == NULL)
        return ESP_ERR_INVALID_ARG;
    if (!bmx280_validate(bmx280))
        return ESP_ERR_INVALID_STATE;

    // Always set ctrl_meas first.
    uint8_t num = (cfg->t_sampling << 5) | (cfg->p_sampling << 2) | BMX280_MODE_SLEEP;
    esp_err_t err = bmx280_write(bmx280, BMX280_REG_MESCTL, &num, sizeof num);

    if (err)
        return err;

    // We can set cfg now.
    num = (cfg->t_standby << 5) | (cfg->iir_filter << 2);
    err = bmx280_write(bmx280, BMX280_REG_CONFIG, &num, sizeof num);

    if (err)
        return err;

    return ESP_OK;
}

/**
 * Read from sensor.
 * @param bmx280 Driver Sturcture.
 * @param addr Register address.
 * @param dout Data to read.
 * @param size The number of bytes to read.
 * @returns Error codes.
 */
esp_err_t bmx280_read(BMP280_t *dev, uint8_t reg_addr, uint8_t *dout, size_t size)
{

    esp_err_t err;
    err = i2c_master_transmit_receive(dev->_i2c_dev_handle, (uint8_t *)&reg_addr, 1, dout, size, I2C_TICKS_TO_WAIT);

    if (err == ESP_OK && false)
    {
        for (int i = 0; i < size; i++)
        {
            ESP_LOGI(TAG, "dout %02x", *(dout + i));
        }
    }
    return err;
}

esp_err_t bmx280_write(BMP280_t *bmx280, uint8_t reg_addr, const uint8_t *din, size_t size)
{
    uint8_t write_buf[256] = {0};
    write_buf[0] = reg_addr;

    esp_err_t err;
//    ESP_LOGI(TAG, "write regaddr: %02x ", write_buf[0]);
    for (int i = 0; i < size; i++)
    {
        write_buf[i + 1] = *(din + i);
 //       ESP_LOGI(TAG, "write  byte: 0x%02x", write_buf[i + 1]);
    }
    err = i2c_master_transmit(bmx280->_i2c_dev_handle, write_buf, size + 1, I2C_TICKS_TO_WAIT);

    return err;
}

// сброс датчика
esp_err_t bmx280_reset(BMP280_t *bmx280)
{
    const static uint8_t din[] = {BMX280_RESET_VEC};
    return bmx280_write(bmx280, BMX280_REG_RESET, din, sizeof din);
}

esp_err_t bmx280_calibrate(BMP280_t *bmx280)
{
    // Honestly, the best course of action is to read the high and low banks
    // into a buffer, then put them in the calibration values. Makes code
    // endian agnostic, and overcomes struct packing issues.
    // Also the BME280 high bank is weird.
    //
    // Write and pray to optimizations is my new motto.

    ESP_LOGI(TAG, "Reading out calibration values...");

    esp_err_t err;
    uint8_t buf[26];

    // Low Bank
    err = bmx280_read(bmx280, BMX280_REG_CAL_LO, buf, sizeof buf);

    if (err != ESP_OK)
        return err;

    ESP_LOGI(TAG, "Read Low Bank.");

    bmx280->cmps.T1 = buf[0] | (buf[1] << 8);
    bmx280->cmps.T2 = buf[2] | (buf[3] << 8);
    bmx280->cmps.T3 = buf[4] | (buf[5] << 8);
    bmx280->cmps.P1 = buf[6] | (buf[7] << 8);
    bmx280->cmps.P2 = buf[8] | (buf[9] << 8);
    bmx280->cmps.P3 = buf[10] | (buf[11] << 8);
    bmx280->cmps.P4 = buf[12] | (buf[13] << 8);
    bmx280->cmps.P5 = buf[14] | (buf[15] << 8);
    bmx280->cmps.P6 = buf[16] | (buf[17] << 8);
    bmx280->cmps.P7 = buf[18] | (buf[19] << 8);
    bmx280->cmps.P8 = buf[20] | (buf[21] << 8);
    bmx280->cmps.P9 = buf[22] | (buf[23] << 8);
    ESP_LOGI(TAG, "Calibration T1 %d T2 %d T3 %d", bmx280->cmps.T1, bmx280->cmps.T2, bmx280->cmps.T3);
    return ESP_OK;
}

void bmx280_close(BMP280_t *bmx280)
{
    free(bmx280);
}
esp_err_t bmx280_setMode(BMP280_t *bmx280, bmx280_mode_t mode)
{
    uint8_t ctrl_mes = 0;
    esp_err_t err;

    if ((err = bmx280_read(bmx280, BMX280_REG_MESCTL, &ctrl_mes, 1)) != ESP_OK)
    {
        ESP_LOGI(TAG, "control_mes: get error");
        return err;
    }
//    ESP_LOGI(TAG, "control_mes: %d", ctrl_mes);
//   ESP_LOGI(TAG, "set mode: %d", mode);

    ctrl_mes = (ctrl_mes & 0b11111100) | (uint8_t)mode;
 //   ESP_LOGI(TAG, "set control_mes: %02x", ctrl_mes);

    return bmx280_write(bmx280, BMX280_REG_MESCTL, &ctrl_mes, 1);
}

esp_err_t bmx280_getMode(BMP280_t *bmx280, bmx280_mode_t *mode)
{
    uint8_t ctrl_mes = 0;
    esp_err_t err;

    if ((err = bmx280_read(bmx280, BMX280_REG_MESCTL, &ctrl_mes, 1)) != ESP_OK)
        return err;

    ctrl_mes &= 0b00000011;

    switch (ctrl_mes)
    {
    case (BMX280_MODE_FORCE + 1): // 2 - ложное значение, приводим к FORCE
        *mode = BMX280_MODE_FORCE;
        break;
    default:
        *mode = ctrl_mes;
        break;
    }

    return ESP_OK;
}

bool bmx280_isSampling(BMP280_t *bmx280)
{
    uint8_t status = 0;
    if (bmx280_read(bmx280, BMX280_REG_STATUS, &status, 1) == ESP_OK)
        return (status & (1 << 3)) != 0;
    else
        return false;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t BME280_compensate_T_int32(BMP280_t *bmx280, int32_t adc_T)
{
     int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)bmx280->cmps.T1 << 1))) * ((int32_t)bmx280->cmps.T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmx280->cmps.T1)) * ((adc_T >> 4) - ((int32_t)bmx280->cmps.T1))) >> 12) * ((int32_t)bmx280->cmps.T3)) >> 14;
    bmx280->t_fine = var1 + var2;
    T = (bmx280->t_fine * 5 + 128) >> 8;
    return T;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t BME280_compensate_P_int64(BMP280_t *bmx280, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)bmx280->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmx280->cmps.P6;
    var2 = var2 + ((var1 * (int64_t)bmx280->cmps.P5) << 17);
    var2 = var2 + (((int64_t)bmx280->cmps.P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmx280->cmps.P3) >> 8) + ((var1 * (int64_t)bmx280->cmps.P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmx280->cmps.P1) >> 33;
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmx280->cmps.P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmx280->cmps.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmx280->cmps.P7) << 4);
    return (uint32_t)p;
}

esp_err_t bmx280_readout(BMP280_t *bmx280, int32_t *temperature, uint32_t *pressure, uint32_t *humidity)
{
    if (bmx280 == NULL)
        return ESP_ERR_INVALID_ARG;
    if (!bmx280_validate(bmx280))
        return ESP_ERR_INVALID_STATE;

    uint8_t buffer[3] = {0, 0, 0};

    esp_err_t error;

    // считываем сразу 3 байта - MSB LSB XSB 0x80 0x00 0x00 - это сброшенное состояние
    if ((error = bmx280_read(bmx280, BMX280_REG_TEMP_MSB, buffer, 3)) != ESP_OK)
        return error;
 //   ESP_LOGI(TAG, "Temperature MSB: %02X LSB:%02X XSB:%02X ", buffer[0], buffer[1], buffer[2]);

    *temperature = BME280_compensate_T_int32(bmx280,
                                             (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4));

    if (pressure)
    {
        if ((error = bmx280_read(bmx280, BMX280_REG_PRES_MSB, buffer, 3)) != ESP_OK)
            return error;

        *pressure = BME280_compensate_P_int64(bmx280,
                                              (buffer[0] << 12) | (buffer[1] << 4) | (buffer[0] >> 4));
    }

    *humidity = UINT32_MAX;

    return ESP_OK;
}
