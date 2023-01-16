#include "project_config.h"

#include <Arduino.h>
#include <core_esp8266_waveform.h>
#include <Esp.h>
#include <user_interface.h>

#include "connectivity.h"
#include "rtc_mem.h"
#include "sensors.h"


/* Global Data Structures */
const uint32_t preinit_magic = PREINIT_MAGIC;

/* Functions */
#if !TETHERED_MODE
void preinit(void)
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!

  connectivity_preinit();
}
#endif

// Read from all of the attached sensors and store the readings
// Read some of the sensors twice to get an average value
void take_readings(void)
#if VCC_CAL_MODE
{
  for (int i=0; i<9; i++) {
    read_vcc(false);
    delay(100);
  }
  read_vcc(true);

  store_uptime();
}
#else
{
  bool sht30_ok;

  read_vcc(false);

#if TETHERED_MODE
  read_ppd42();
#endif

  //read temp/humidity from SHT30
  sht30_ok = read_sht30(false);

  //read temp/humidity from HP303B
  read_hp303b(!sht30_ok);

  if (sht30_ok)
    read_sht30(true);

  read_vcc(true);

  store_uptime();
}
#endif

bool check_upload_conditions(void)
{
  flags_time_t *flags = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];
  uint32_t boot_count = rtc_mem[RTC_MEM_BOOT_COUNT];

  /*
   * Normal upload condition:
   * If our number of collected readings is above the high water mark, then it
   * is time to start uploading (allowing for a couple of failed connections).
   * Extra conditions:
   * If our boot count is a power of 2 during the first few boots, then we will
   * do a special upload to get some early readings sent to the server.
   * (This way you won't have to wait for 30-45 minutes before the initial
   * readings come in.)
   */
  if (rtc_mem[RTC_MEM_NUM_READINGS] >= HIGH_WATER_SLOT) {
    flags->flags |= FLAG_BIT_NORMAL_UPLOAD_COND;
    return true;
  }
  if ( (0 == (flags->flags & FLAG_BIT_NORMAL_UPLOAD_COND)) &&
    (0==(boot_count & (boot_count - 1))) )
    return true;

  return false;
}

#if TETHERED_MODE
void tethered_sleep(int64_t millis_offset, bool please_reboot)
{
  int64_t sleep_delta = (int64_t)SLEEP_TIME_US - (((int64_t)millis()-millis_offset)*1000);

#if EXTRA_DEBUG
  Serial.printf("[%llu] sleep_delta=%lld\n", uptime(), sleep_delta);
#endif
  if (sleep_delta > 200000LL)
    deep_sleep(sleep_delta);
  else if (please_reboot)
    deep_sleep(100);
  else
    save_rtc(); // it is probably still worth saving RTC mem in case of soft reset, etc.

  if (sleep_delta > 0)
    delay(sleep_delta/1000);
}
#endif /* TETHERED_MODE */

#if !TETHERED_MODE
void battery_sleep(bool connect_failed=false)
{
  uint64_t sleep_delta = SLEEP_TIME_US;
  flags_time_t *flags = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];

  connectivity_disable();

  if (connect_failed) {
    sleep_delta <<= flags->fail_count;
    if (flags->fail_count < 6)
      flags->fail_count++;
#if EXTRA_DEBUG
    Serial.printf("[%llu] sleep_delta=%lld\n", uptime(), sleep_delta);
#endif
  } else {
    flags->fail_count = 0;
  }

  deep_sleep(sleep_delta);
}
#endif /* !TETHERED_MODE */

void setup(void)
{
  bool rtc_config_valid = false;

  pinMode(LED_BUILTIN, INPUT);

  Serial.begin(SERIAL_SPEED);
  delay(2);
  Serial.println();

  sensors_init();
  rtc_config_valid = load_rtc_memory();

  // We can detect a "double press" of the reset button as a regular Ext Reset
  // This is because we spend most of our time asleep and a single press will
  // generally appear as a deep-sleep wakeup.
  // We will use this to enter the WiFi config mode.
  // However, we must ignore it on the first boot after reprogramming or inserting
  // the battery -- so only pay attention if the RTC memory checksum is OK.
  if ((ESP.getResetInfoPtr()->reason == REASON_EXT_SYS_RST) && rtc_config_valid) {
    pinMode(LED_BUILTIN, OUTPUT);
    startWaveform(LED_BUILTIN, 350000, 50000, 0);
    connectivity_init();
    enter_config_mode();
    stopWaveform(LED_BUILTIN);
    pinMode(LED_BUILTIN, INPUT);

#if !TETHERED_MODE
  battery_sleep();
#endif
  } else {
#if TETHERED_MODE
    // defer this if battery-powered
    connectivity_init();
#endif
  }
}

void loop(void)
{
  flags_time_t *flags = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];
#if TETHERED_MODE
  unsigned long loop_start = millis();
#endif
  bool connect_failed = false;

  take_readings();
  dump_readings();

#if !TETHERED_MODE
  if (0 != (flags->flags & FLAG_BIT_CONNECT_NEXT_WAKE))
#else
  if (check_upload_conditions())
#endif
  {
    uint32_t num_readings = rtc_mem[RTC_MEM_NUM_READINGS];

    // time to connect and upload our readings
#if !TETHERED_MODE
    //this is normally done in setup() but deferred in battery mode
    connectivity_init();
#endif

    if (connect_wifi())
      upload_readings();

    //we failed to make progress uploading readings
    //factor this into sleep time decisions
    if (rtc_mem[RTC_MEM_NUM_READINGS] == num_readings)
      connect_failed = true;
  }

#if !TETHERED_MODE
  if (!connect_failed) {
    if (check_upload_conditions()) {
      // we have collected enough readings, time to upload them
      // due to bugs/changes in deep sleep operation, the workaround is to do a
      // non-rf-disabled sleep and then transmit on the next wakeup
      flags->flags |= FLAG_BIT_CONNECT_NEXT_WAKE;
    } else {
      flags->flags &= ~FLAG_BIT_CONNECT_NEXT_WAKE;
    }
  }
#endif

#if TETHERED_MODE
  // in tethered mode, sleep time is more of a suggestion if other
  // activities don't take longer
  tethered_sleep(loop_start, connect_failed);
#else
  battery_sleep(connect_failed);
#endif
}
