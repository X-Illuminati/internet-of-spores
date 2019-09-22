#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

#include <Esp.h>

/* Set ADC mode to read VCC voltage level (battery) */
ADC_MODE(ADC_VCC)

/* Global Configurations */
#ifdef IOTAPPSTORY
#define COMPDATE                (__DATE__ __TIME__)
#define MODEBUTTON              (0) /* unused */
#else
#define SERIAL_SPEED            (115200)
#endif
#define EXTRA_DEBUG             (0)
#define SLEEP_TIME_US           (15000000)
#define SLEEP_OVERHEAD_MS       (120) /* guess at the overhead time for entering and exitting sleep */
#define BUILD_UNIQUE_ID         (__TIME__[3]*1000+__TIME__[4]*100+__TIME__[6]*10+__TIME__[7])
#define PREINIT_MAGIC           (0xAA559876 ^ BUILD_UNIQUE_ID)
#define NUM_STORAGE_SLOTS       (59)
#define HIGH_WATER_SLOT         (NUM_STORAGE_SLOTS-12)
#define SHT30_ADDR              (0x45)
#define REPORT_HOST_NAME        ("boxy")
#define REPORT_HOST_PORT        (2880)
#define REPORT_RESPONSE_TIMEOUT (2000)
#define NODE_BASE_NAME          ("spores-")
#define CONFIG_SERVER_MAX_TIME  (600 /*seconds*/)

#endif /* _PROJECT_CONFIG_H_ */
