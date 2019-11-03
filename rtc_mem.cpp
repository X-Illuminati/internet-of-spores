#include "project_config.h"

#include <Arduino.h>
#include <Esp.h>

#include "persistent.h"
#include "rtc_mem.h"


/* Global Data Structures */
uint32_t rtc_mem[RTC_MEM_MAX];


/* Functions */
// read rtc user memory and print some details to serial
// increments the boot count and returns true if the RTC memory was OK
bool load_rtc_memory(void)
{
  bool retval = true;

  ESP.rtcUserMemoryRead(0, rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[RTC_MEM_CHECK] + rtc_mem[RTC_MEM_BOOT_COUNT] != preinit_magic) {
    Serial.println(String("Preinit magic doesn't compute, reinitializing (0x") + String(preinit_magic, HEX) + ")");
    invalidate_rtc();
    retval = false;
  }
  rtc_mem[RTC_MEM_BOOT_COUNT]++;

  Serial.printf("[%llu] ", uptime());
  Serial.print("setup: reset reason=");
  Serial.print(ESP.getResetReason());
  Serial.print(", boot count=");
  Serial.print(rtc_mem[RTC_MEM_BOOT_COUNT]);
#if (EXTRA_DEBUG != 0)
  flags_time_t *flags = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];
  Serial.print(", flags=0x");
  Serial.print((uint8_t)flags->flags, HEX);
  Serial.print(", RTC_SIZE=");
  Serial.print(RTC_MEM_MAX);
#endif

  Serial.print(", num readings=");
  Serial.println(rtc_mem[RTC_MEM_NUM_READINGS]);

  if (RTC_MEM_MAX > 128)
    Serial.println("*************************\nWARNING RTC_MEM_MAX > 128\n*************************");

  return retval;
}

// clear/reinitialize rtc memory
void invalidate_rtc(void)
{
  flags_time_t *timestruct = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];
  uint16_t clock_cal;

  memset(rtc_mem, 0, sizeof(rtc_mem));
  ESP.rtcUserMemoryWrite(0, rtc_mem, sizeof(rtc_mem));

  clock_cal = persistent_read(PERSISTENT_CLOCK_CALIB, DEFAULT_SLEEP_CLOCK_ADJ);
  if (clock_cal > 0)
    timestruct->clock_cal = clock_cal;
  else
    timestruct->clock_cal = DEFAULT_SLEEP_CLOCK_ADJ;
}

// return the uptime in ms (added to the RTC stored time)
uint64_t uptime(void)
{
  uint64_t uptime_ms;
  flags_time_t *timestruct = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];

  // boot timestamp + current millis()
  uptime_ms = timestruct->millis;
  uptime_ms += millis();
  return uptime_ms;
}

// helper for saving rtc memory
void save_rtc(uint64_t sleep_time_us)
{
  flags_time_t *timestruct = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];
  uint64_t backup_millis = timestruct->millis; //store the current uptime value in case we aren't sleeping

  // update the stored millis including some overhead for the write, suspend, and wake
  timestruct->millis += millis() + sleep_time_us/1000 + timestruct->clock_cal;

  // update the header checksum
  rtc_mem[RTC_MEM_CHECK] = preinit_magic - rtc_mem[RTC_MEM_BOOT_COUNT];

  // store the array to RTC memory
  ESP.rtcUserMemoryWrite(0, rtc_mem, sizeof(rtc_mem));

  // restore the old uptime in case we aren't sleeping (millis won't be reset in that case)
  timestruct->millis = backup_millis;
}

// helper for saving rtc memory before entering deep sleep
void deep_sleep(uint64_t time_us)
{
  save_rtc(time_us);

#if TETHERED_MODE
  ESP.deepSleep(time_us, RF_NO_CAL);
#else
  ESP.deepSleep(time_us, RF_DISABLED);
#endif
}

// store sensor reading in the rtc mem ring buffer
void store_reading(sensor_type_t type, int32_t val)
{
  sensor_reading_t *reading;
  int slot;

  // determine the next free slot in the RTC memory ring buffer
  slot = rtc_mem[RTC_MEM_FIRST_READING] + rtc_mem[RTC_MEM_NUM_READINGS];
  if (slot >= NUM_STORAGE_SLOTS)
    slot -= NUM_STORAGE_SLOTS;

  // update the ring buffer metadata
  if (rtc_mem[RTC_MEM_NUM_READINGS] == NUM_STORAGE_SLOTS)
    rtc_mem[RTC_MEM_FIRST_READING]++;
  else
    rtc_mem[RTC_MEM_NUM_READINGS]++;

  if (rtc_mem[RTC_MEM_FIRST_READING] >= NUM_STORAGE_SLOTS)
    rtc_mem[RTC_MEM_FIRST_READING]=0;

  // store the sensor reading in RTC memory at the given slot
  reading = (sensor_reading_t*) &rtc_mem[RTC_MEM_DATA+slot*NUM_WORDS(sensor_reading_t)];
  reading->type = type;
  reading->value = val;
}

// reset the rtc mem ring buffer
void clear_readings(unsigned int num /*defaults to NUM_STORAGE_SLOTS*/)
{
  if (num >= rtc_mem[RTC_MEM_NUM_READINGS]) {
    // simple case - just reset the ring buffer
    rtc_mem[RTC_MEM_FIRST_READING]=0;
    rtc_mem[RTC_MEM_NUM_READINGS]=0;
  } else {
    // increment first reading to "erase" num readings
    rtc_mem[RTC_MEM_FIRST_READING] += num;
    if (rtc_mem[RTC_MEM_FIRST_READING] >= NUM_STORAGE_SLOTS)
      rtc_mem[RTC_MEM_FIRST_READING] -= NUM_STORAGE_SLOTS;

    rtc_mem[RTC_MEM_NUM_READINGS] -= num;
  }
}

// print the stored pressure readings from the rtc mem ring buffer
void dump_readings(void)
{
#if (EXTRA_DEBUG != 0)
  flags_time_t timestamp = {0,0,0};
  const char typestrings[][11] = {
    "UNKNOWN",
    "TEMP (C)",
    "HUMI (%)",
    "PRES (kPa)",
    "PART (?)",
    "BATT (V)",
    "TIME (ms)",
  };
  char formatted[47];
  formatted[46]=0;

  Serial.println("slot | type       |      rawvalue");
  Serial.println("-----+------------+--------------");


  for (unsigned i=0; i < rtc_mem[RTC_MEM_NUM_READINGS]; i++) {
    sensor_reading_t *reading;
    const char* type = typestrings[0];
    int slot;

    // find the slot indexed into the ring buffer
    slot = rtc_mem[RTC_MEM_FIRST_READING] + i;
    if (slot >= NUM_STORAGE_SLOTS)
      slot -= NUM_STORAGE_SLOTS;

    reading = (sensor_reading_t*) &rtc_mem[RTC_MEM_DATA+slot*NUM_WORDS(sensor_reading_t)];

    // determine the sensor type
    switch(reading->type) {
      case SENSOR_TEMPERATURE:
        type=typestrings[1];
      break;

      case SENSOR_HUMIDITY:
        type=typestrings[2];
      break;

      case SENSOR_PRESSURE:
        type=typestrings[3];
      break;

      case SENSOR_PARTICLE:
        type=typestrings[4];
      break;

      case SENSOR_BATTERY_VOLTAGE:
        type=typestrings[5];
      break;

      case SENSOR_TIMESTAMP_L:
        timestamp.millis = reading->value & SENSOR_TIMESTAMP_MASK;
        continue;
      break;

      case SENSOR_TIMESTAMP_H:
        type=typestrings[6];
        timestamp.millis |= ((uint64_t)reading->value & SENSOR_TIMESTAMP_MASK) << SENSOR_TIMESTAMP_SHIFT;
      break;

      case SENSOR_UNKNOWN:
      default:
        type=typestrings[0];
      break;
    }

    // format an output row with the slot, type, and data
    if (reading->type == SENSOR_TIMESTAMP_H)
      snprintf(formatted, 45, "%4u | %-10s | %13llu", i, type, timestamp.millis);
    else
      snprintf(formatted, 45, "%4u | %-10s | %+13.3f", i, type, reading->value/1000.0);
    Serial.println(formatted);
  }
  Serial.printf("[%llu] dump complete\n", uptime());
#endif
}
