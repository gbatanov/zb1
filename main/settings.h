#define USE_ZIGBEE
//#define USE_BMP280
#define USE_DISPLAY

#ifdef USE_DISPLAY
#ifndef USE_I2C
#define USE_I2C
#endif
#endif

#ifdef USE_BMP280
#ifndef USE_I2C
#define USE_I2C
#endif
#endif