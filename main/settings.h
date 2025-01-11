#define V0TAG "GSB_ZB_0"
#define V0_LOG

//#define USE_ZIGBEE
//#define USE_MOTION_SENSOR
#define USE_SONAR
//#define USE_ISR_BUTTON
//#define USE_TIMER
//#define V1
// USE_TEMP_CHIP перекрывает использование USE_BMP280
// использую либо то, либо другое
#define USE_TEMP_CHIP

// #define USE_BMP280
// #define USE_DISPLAY

//#ifdef V1
#ifdef USE_BMP280
#undef USE_BMP280
#endif
#ifdef USE_DISPLAY
#undef USE_DISPLAY
#endif
//#endif

// USE_TEMP_CHIP перекрывает использование USE_BMP280
#ifdef USE_TEMP_CHIP
#ifdef USE_BMP280
#undef USE_BMP280
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
