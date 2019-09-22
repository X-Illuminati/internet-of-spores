#ifndef _RTC_MEM_H_
#define _RTC_MEM_H_

#include "project_config.h"
#ifdef IOTAPPSTORY
#include <IOTAppStory.h>
#endif
#include "sensors.h"


/* Types and Enums */
// Macro to calculate the number of words that a
// data structure will take up in RTC memory
#define NUM_WORDS(x) (sizeof(x)/sizeof(uint32_t))

// Structure to combine uptime with device flags
typedef struct flags_time_s {
  uint64_t flags    :8;
  uint64_t reserved :16;
  uint64_t millis   :40; //uptime in ms tracked over suspend cycles
} flags_time_t;

// Structure to combine sensor readings with type and timestamp
typedef struct sensor_reading_s {
  sensor_type_t  type      :3;
  uint64_t       timestamp :40;
  int32_t        reading   :21;
} sensor_reading_t;

// Fields for each of the 32-bit fields in RTC Memory
enum rtc_mem_fields_e {
  RTC_MEM_CHECK = 0,       // Magic/Header CRC
  RTC_MEM_BOOT_COUNT,      // Number of accumulated wakeups since last power loss
  RTC_MEM_FLAGS_TIME,      // Timestamp for start of boot, this is 64-bits so it needs 2 fields
  RTC_MEM_FLAGS_TIME_END = RTC_MEM_FLAGS_TIME + NUM_WORDS(flags_time_t) - 1,
#ifdef IOTAPPSTORY
  RTC_MEM_IOTAPPSTORY = 4, // Memory used by IOTAppStory internally
  RTC_MEM_IOTAPPSTORY_END = RTC_MEM_IOTAPPSTORY + NUM_WORDS(rtcMemDef) - 1,
#endif
  RTC_MEM_NUM_READINGS,    // Number of occupied slots
  RTC_MEM_FIRST_READING,   // Slot that has the oldest reading

  //array of sensor readings
  RTC_MEM_DATA,
  RTC_MEM_DATA_END = RTC_MEM_DATA + NUM_STORAGE_SLOTS*NUM_WORDS(sensor_reading_t) - 1,

  //keep last
  RTC_MEM_MAX
};


/* Global Data Structures */
extern uint32_t rtc_mem[RTC_MEM_MAX]; //array for accessing RTC Memory


/* Function Prototypes */
bool load_rtc_memory(void);
void invalidate_rtc(void);

uint64_t uptime(void);

void store_reading(sensor_type_t type, int32_t val);
void clear_readings(void);
void dump_readings(void);

#endif /* _RTC_MEM_H_ */
