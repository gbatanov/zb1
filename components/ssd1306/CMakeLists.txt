idf_component_register(SRCS 
						"ssd1306.c"
						"ssd1306_i2c.c"  
						PRIV_REQUIRES driver 
						INCLUDE_DIRS ".")
