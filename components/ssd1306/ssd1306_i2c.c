#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "esp_log.h"

#include "ssd1306.h"

#define TAG "I2C"


#define I2C_MASTER_FREQ_HZ 400000 // I2C clock of SSD1306 can run at 400 kHz max.

extern i2c_master_bus_handle_t bus_handle;

// Добавление устройства SSD1306 на шину
void i2c_bus_add_ssd1306(SSD1306_t *dev, i2c_port_t i2c_num)
{
	ESP_LOGI(TAG, "New i2c driver is used");
	ESP_LOGW(TAG, "Will not install i2c master driver");

	i2c_device_config_t dev_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = I2C_SSD1306_ADDRESS,
		.scl_speed_hz = I2C_MASTER_FREQ_HZ,
	};
	i2c_master_dev_handle_t i2c_dev_handle;
	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &i2c_dev_handle));

	dev->_address = I2C_SSD1306_ADDRESS;
	dev->_flip = false;
	dev->_i2c_num = i2c_num;
	dev->_i2c_dev_handle = i2c_dev_handle;
}

