#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

/* Global Configurations */
#define VCC_CAL_MODE            (0)
#if VCC_CAL_MODE
#define DEVELOPMENT_BUILD       (1)
#define TETHERED_MODE           (1)
#else
#define DEVELOPMENT_BUILD       (0)
#define TETHERED_MODE           (0)
#endif

#define SERIAL_SPEED            (115200)
#define CONFIG_SERVER_MAX_TIME  (120 /* seconds without client */)
#define BUILD_UNIQUE_ID         (__TIME__[3]*1000+__TIME__[4]*100+__TIME__[6]*10+__TIME__[7])
#define PREINIT_MAGIC           (0xAA559876 ^ BUILD_UNIQUE_ID)
#define SHT30_ADDR              (0x45)
#define REPORT_RESPONSE_TIMEOUT (2000)
#define WIFI_CONNECT_TIMEOUT    (30000)
#define LOW_BATTERY_MV          (2850)
/* num of connection failures after which the next failure will result in
   displaying the connection error message on the EPD_1in9 display
   3 seems reasonable, since it will have been 7xSLEEP_TIME_US since we were
   ready to transmit and the next display update will come in 8xSLEEP_TIME_US
   at which point the current display will be very out-of-date */
#define DISP_CONNECT_FAIL_COUNT (3)

#if TETHERED_MODE
  #define PPD42_PIN_DET         (D5)
  #define PPD42_PIN_1_0         (D7)
  #define PPD42_PIN_2_5         (D6)
  #define FIRMWARE_NAME         "iotsp-tethered"
#else
  #define FIRMWARE_NAME         "iotsp-battery"
#endif
  #define EPD_BUSY_PIN          (D7)
  #define EPD_RST_PIN           (D6)

#if DEVELOPMENT_BUILD
  #define EXTRA_DEBUG           (1)
  #define DISABLE_FW_UPDATE     (1)
  #define SLEEP_TIME_US         (10000000ULL)
  #define NUM_STORAGE_SLOTS     (27)
  #if TETHERED_MODE
    #define HIGH_WATER_SLOT     (1)
  #else
    #define HIGH_WATER_SLOT     (17)
  #endif
#else
  #define EXTRA_DEBUG           (0)
  #define DISABLE_FW_UPDATE     (0)
  #define SLEEP_TIME_US         (60000000ULL)
  #define NUM_STORAGE_SLOTS     (119)
  #if TETHERED_MODE
    #define HIGH_WATER_SLOT     (1)
  #else
    #define HIGH_WATER_SLOT     (NUM_STORAGE_SLOTS-18)
  #endif
#endif

//persistent storage file names
#define PERSISTENT_NODE_NAME        "node_name"
#define PERSISTENT_REPORT_HOST_NAME "report_host_name"
#define PERSISTENT_REPORT_HOST_PORT "report_host_port"
#define PERSISTENT_CLOCK_CALIB      "clock_drift_calibration"
#define PERSISTENT_TEMP_CALIB       "temperature_calibration"
#define PERSISTENT_HUMIDITY_CALIB   "humidity_calibration"
#define PERSISTENT_PRESSURE_CALIB   "pressure_calibration"
#define PERSISTENT_BATTERY_CALIB    "battery_calibration"

#define DEFAULT_NODE_BASE_NAME      "iotsp-"
#define DEFAULT_REPORT_HOST_NAME    "192.168.0.1"
#define DEFAULT_REPORT_HOST_PORT    (2880)
#define DEFAULT_SLEEP_CLOCK_ADJ     (120)
#define DEFAULT_TEMP_CALIB          (0.0f)
#define DEFAULT_HUMIDITY_CALIB      (0.0f)
#define DEFAULT_PRESSURE_CALIB      (0.0f)
#define DEFAULT_BATTERY_CALIB       (0.0f)

#endif /* _PROJECT_CONFIG_H_ */
