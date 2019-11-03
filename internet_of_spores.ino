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
{
  bool sht30_ok;
  bool hp303b_ok;

  read_vcc(false);

  //read temp/humidity from SHT30
  sht30_ok = read_sht30(false);

  if (sht30_ok) {
    delay(100);
    sht30_ok = read_sht30(false);
  }

  //read temp/humidity from HP303B
  hp303b_ok = read_hp303b(!sht30_ok);

  if (!hp303b_ok)
    delay(100);

  if (sht30_ok)
    read_sht30(true);

  read_vcc(true);

  store_uptime();
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
  connectivity_disable();
  deep_sleep(SLEEP_TIME_US);
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
#if TETHERED_MODE
  unsigned long loop_start = millis();
  bool connect_failed = false;
#endif

  take_readings();
  dump_readings();

  if (rtc_mem[RTC_MEM_NUM_READINGS] >= HIGH_WATER_SLOT) {
    // time to connect and upload our readings

#if !TETHERED_MODE
    //this is normally done in setup() but deferred in battery mode
    connectivity_init();
#endif

    if (connect_wifi())
      upload_readings();
#if TETHERED_MODE
    else
      connect_failed = true;
#endif
  }

#if TETHERED_MODE
  // in tethered mode, sleep time is more of a suggestion if other
  // activities don't take longer
  tethered_sleep(loop_start, connect_failed);
#else
  connectivity_disable();
  deep_sleep(SLEEP_TIME_US);
#endif
}
