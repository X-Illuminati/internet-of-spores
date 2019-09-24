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
void preinit(void)
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!

  connectivity_preinit();
}

// Read from all of the attached sensors and store the readings
void take_readings(void)
{
  bool sht30_ok;

  //read temp/humidity from SHT30
  sht30_ok = read_sht30();

  //read temp/humidity from HP303B
  read_hp303b(!sht30_ok);

  read_vcc();
}

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
  } else {
    take_readings();
    dump_readings();

    if (rtc_mem[RTC_MEM_NUM_READINGS] >= HIGH_WATER_SLOT) {
      // time to connect and upload our readings
      connectivity_init();
      if (connect_wifi())
        upload_readings();
    }
  }

  connectivity_disable();
  deep_sleep(SLEEP_TIME_US);
}

void loop(void)
{
  //should never get here
  connectivity_disable();
  deep_sleep(SLEEP_TIME_US);
}
