idf_component_register(SRCS 
						"ssd1306.c"
						"ssd1306_i2c.c"  
						PRIV_REQUIRES driver 
						INCLUDE_DIRS "." "/Users/gbatanov/work/esp-idf/v6.0/esp-idf/components/esp_driver_i2c/include" "/Users/gbatanov/work/esp-idf/v6.0/esp-idf/components/esp_driver_gpio/include"
						)
