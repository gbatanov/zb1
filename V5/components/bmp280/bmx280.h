#ifndef _BMX280_H_
#define _BMX280_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <limits.h>
#include "driver/i2c_master.h"
#include "bmx280_bits.h"
#include "sdkconfig.h"

#define I2C_BMP280_ADDRESS 0x76

    typedef struct
    {
        i2c_port_t _i2c_num;
        i2c_master_dev_handle_t _i2c_dev_handle;
        uint8_t _address;
        // Chip ID of sensor
        uint8_t chip_id;
        // Compensation data
        struct
        {
            uint16_t T1;
            int16_t T2;
            int16_t T3;
            uint16_t P1;
            int16_t P2;
            int16_t P3;
            int16_t P4;
            int16_t P5;
            int16_t P6;
            int16_t P7;
            int16_t P8;
            int16_t P9;
        } cmps;
        // Storage for a variable proportional to temperature.
        int32_t t_fine;
    } BMP280_t;
#include "bmx280_bits.h"

    void i2c_bus_add_bmp280(BMP280_t *dev, i2c_port_t i2c_num);
    esp_err_t bmx280_reset(BMP280_t *dev);
    esp_err_t bmx280_calibrate(BMP280_t *dev);
    void bmx280_close(BMP280_t *bmx280);
    esp_err_t bmx280_init(BMP280_t *bmx280);
    esp_err_t bmx280_configure(BMP280_t *bmx280, bmx280_config_t *cfg);
    esp_err_t bmx280_setMode(BMP280_t *bmx280, bmx280_mode_t mode);
    esp_err_t bmx280_getMode(BMP280_t *bmx280, bmx280_mode_t *mode);
    bool bmx280_isSampling(BMP280_t *bmx280);
    esp_err_t bmx280_write(BMP280_t *bmx280, uint8_t reg_addr, const uint8_t *din, size_t size);
    esp_err_t bmx280_read(BMP280_t *dev, uint8_t reg_addr, uint8_t *dout, size_t size);

    esp_err_t bmx280_readout(BMP280_t *bmx280, int32_t *temperature, uint32_t *pressure, uint32_t *humidity);

    static inline void bmx280_readout2float(int32_t *tin, uint32_t *pin, uint32_t *hin, float *tout, float *pout, float *hout)
    {
        if (tin && tout)
            *tout = (float)*tin * 0.01f;
        if (pin && pout)
            *pout = (float)*pin * (1.0f / 256.0f);
        if (hin && hout)
            *hout = (*hin == UINT32_MAX) ? -1.0f : (float)*hin * (1.0f / 1024.0f);
    }

    static inline esp_err_t bmx280_readoutFloat(BMP280_t *bmx280, float *temperature, float *pressure, float *humidity)
    {
        int32_t t;
        uint32_t p, h;
        esp_err_t err = bmx280_readout(bmx280, &t, &p, &h);

        if (err == ESP_OK)
        {
            bmx280_readout2float(&t, &p, &h, temperature, pressure, humidity);
        }

        return err;
    }

#ifdef __cplusplus
};
#endif

#endif
