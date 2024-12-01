#define USE_ZIGBEE

#define V1

// #define USE_BMP280
#define USE_DISPLAY

#ifdef V1
#ifdef USE_BMP280
#undef USE_BMP280
#endif
#ifdef USE_DISPLAY
#undef USE_DISPLAY
#endif
#endif

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
