#ifndef _RTC_MEM_H_
#define _RTC_MEM_H_

#include "project_config.h"
#include "sensors.h"


/* Global Configurations */
#define RTC_DATA_TIMEBASE_SHIFT (8)
#define RTC_DATA_TIMEBASE_MASK  (-1LL<<RTC_DATA_TIMEBASE_SHIFT)


/* Types and Enums */
// Macro to calculate the number of words that a
// data structure will take up in RTC memory
#define NUM_WORDS(x) (sizeof(x)/sizeof(uint32_t))

// Structure to combine uptime with device flags
typedef struct flags_time_s {
  uint64_t flags      :5;
  uint64_t fail_count :3;  //keep track of wifi connection failures
  uint64_t clock_cal  :16; //calibration for clock drift during suspend in ms
  uint64_t millis     :40; //uptime in ms tracked over suspend cycles
} flags_time_t;

// Flag bits
#define FLAG_BIT_CONNECT_NEXT_WAKE (1 << 0)

// Structure to combine sensor readings with type and timestamp
typedef struct sensor_reading_s {
  sensor_type_t type  :8;
  int32_t       value :24;
} sensor_reading_t;

// Fields for each of the 32-bit fields in RTC Memory
enum rtc_mem_fields_e {
  RTC_MEM_CHECK = 0,       // Magic/Header CRC
  RTC_MEM_BOOT_COUNT,      // Number of accumulated wakeups since last power loss
  RTC_MEM_FLAGS_TIME,      // Timestamp for start of boot, this is 64-bits so it needs 2 fields
  RTC_MEM_FLAGS_TIME_END = RTC_MEM_FLAGS_TIME + NUM_WORDS(flags_time_t) - 1,
  RTC_MEM_DATA_TIMEBASE,   // Timestamp (upper 32 bits) from which sensor readings are stored as offsets
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
extern const uint32_t preinit_magic; //keep this in the .ino so it gets recompiled on every build

/* Function Prototypes */
bool load_rtc_memory(void);
void invalidate_rtc(void);

uint64_t uptime(void);
void save_rtc(uint64_t sleep_time_us=0);
void deep_sleep(uint64_t time_us);

void store_reading(sensor_type_t type, int32_t val);
void clear_readings(unsigned int num=NUM_STORAGE_SLOTS);
void dump_readings(void);

#endif /* _RTC_MEM_H_ */
