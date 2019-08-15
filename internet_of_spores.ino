#include <ESP8266WiFi.h>
#include <LOLIN_HP303B.h>
#include <IOTAppStory.h>                // IotAppStory.com library

#define SERIAL_SPEED 115200

// HP303B Opject
LOLIN_HP303B HP303BPressureSensor;

#define COMPDATE __DATE__ __TIME__
#define MODEBUTTON 0                    // Button pin on the esp for selecting modes. D3 for the Wemos!
IOTAppStory IAS(COMPDATE, MODEBUTTON);  // Initialize IotAppStory

//#define INCLUDE_TEMP

#define PREINIT_MAGIC 0xAA559876
#define NUM_STORAGE_SLOTS 10

typedef enum RTC_MEM {
  RTC_MEM_CHECK         = 0,
  RTC_MEM_BOOT_COUNT    = 1,
  RTC_MEM_MILLIS        = 2,
  RTC_MEM_FLAGS         = 3,
  RTC_MEM_NUM_READINGS  = 4,
  RTC_MEM_FIRST_READING = 5,

  //keep last
  RTC_MEM_DATA,
  RTC_MEM_MAX = RTC_MEM_DATA + NUM_STORAGE_SLOTS,
} RTC_MEM_T;
uint32_t rtc_mem[RTC_MEM_MAX];

void preinit()
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}

// helper to return the uptime in ms
uint64_t uptime(void)
{
  uint64_t uptime_ms;
  uptime_ms = rtc_mem[RTC_MEM_FLAGS] & 0xFF;
  uptime_ms <<= 32;
  uptime_ms += rtc_mem[RTC_MEM_MILLIS];
  uptime_ms += millis();
  return uptime_ms;
}

// helper to print the uptime since the function prototype is tricky
void serial_print_uptime(void)
{
  char llu[21];
  llu[20]='\0';
  sprintf(llu, "%llu", uptime());
  Serial.print(llu);
}

// helper to store pressure readings in the rtc mem ring buffer
void store_reading(int32_t val)
{
  int slot = rtc_mem[RTC_MEM_FIRST_READING] + rtc_mem[RTC_MEM_NUM_READINGS];
  if (slot >= NUM_STORAGE_SLOTS)
    slot -= NUM_STORAGE_SLOTS;

  if (rtc_mem[RTC_MEM_NUM_READINGS] == NUM_STORAGE_SLOTS)
    rtc_mem[RTC_MEM_FIRST_READING]++;
  else
    rtc_mem[RTC_MEM_NUM_READINGS]++;

  if (rtc_mem[RTC_MEM_FIRST_READING] >= NUM_STORAGE_SLOTS)
    rtc_mem[RTC_MEM_FIRST_READING]=0;

  //Serial.print("would store "); Serial.print(val); Serial.print(" @"); Serial.print(slot); Serial.print("("); Serial.print(RTC_MEM_DATA+slot); Serial.println(")");
  rtc_mem[RTC_MEM_DATA+slot] = val;
}

// helper to print the stored pressure readings from the rtc mem ring buffer
void dump_readings(void)
{
  Serial.print("[");

  for (unsigned long i=0; i < rtc_mem[RTC_MEM_NUM_READINGS]; i++)
  {
    int slot = rtc_mem[RTC_MEM_FIRST_READING] + i;
    if (slot >= NUM_STORAGE_SLOTS)
      slot -= NUM_STORAGE_SLOTS;

    if (i != 0)
      Serial.print(",");
    Serial.print((int32_t)rtc_mem[RTC_MEM_DATA+slot]);
  }

  Serial.println("]");
}

// helper for saving rtc memory before entering deep sleep
void deep_sleep(uint64_t time_us)
{
  uint64_t uptime_ms = uptime() + time_us/1000 + 120; //overhead guess (esp/stopwatch): 100=~.988, 200=1.008, 150=1.001 135=1.00082 130=1.00058 122=1.000078
  rtc_mem[RTC_MEM_MILLIS] = uptime_ms & 0xFFFFFFFF;
  rtc_mem[RTC_MEM_FLAGS] = (uptime_ms  >> 32) & 0xFF;
  rtc_mem[RTC_MEM_CHECK] = PREINIT_MAGIC - rtc_mem[RTC_MEM_BOOT_COUNT];
  ESP.rtcUserMemoryWrite(0, rtc_mem, sizeof(rtc_mem));
  ESP.deepSleep(time_us, RF_DISABLED);
}

void setup(void)
{
  Serial.begin(SERIAL_SPEED);
  delay(2);
  Serial.println();

  ESP.rtcUserMemoryRead(0, rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[RTC_MEM_CHECK] + rtc_mem[RTC_MEM_BOOT_COUNT] != PREINIT_MAGIC)
    memset(rtc_mem, 0, sizeof(rtc_mem));
  rtc_mem[RTC_MEM_BOOT_COUNT]++;

  Serial.print("["); serial_print_uptime(); Serial.print("] ");
  Serial.print("setup: boot count=");
  Serial.print(rtc_mem[RTC_MEM_BOOT_COUNT]);
  Serial.print(", num readings=");
  Serial.print(rtc_mem[RTC_MEM_NUM_READINGS]);
  Serial.print(", first reading=");
  Serial.println(rtc_mem[RTC_MEM_FIRST_READING]);

  //IAS.begin('P');

  //Call begin to initialize HP303BPressureSensor
  //The parameter 0x76 is the bus address. The default address is 0x77 and does not need to be given.
  //HP303BPressureSensor.begin(Wire, 0x76);
  //Use the commented line below instead of the one above to use the default I2C address.
  //if you are using the Pressure 3 click Board, you need 0x76
  HP303BPressureSensor.begin();
}

void loop(void)
{
  int32_t pressure;
  int16_t oversampling = 4;
  //OVERSAMPLING TABLE
  // Val | Time (ms) | Noise (counts)
  //   0 |      15.6 | 12
  //   1 |      17.6 | 6
  //   2 |      20.6 | 4
  //   3 |      25.5 | 3
  //   4 |      39.6 | 2
  //   5 |      65.6 | 2
  //   6 |     116.6 | 2
  //   7 |     218.7 | 1

  int16_t ret;

#ifdef INCLUDE_TEMP
  int32_t temperature;

  Serial.print("["); serial_print_uptime(); Serial.print("] ");

  //lets the HP303B perform a Single temperature measurement with the last (or standard) configuration
  //The result will be written to the paramerter temperature
  //ret = HP303BPressureSensor.measureTempOnce(temperature);
  //the commented line below does exactly the same as the one above, but you can also config the precision
  //oversampling can be a value from 0 to 7
  //the HP303B will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
  //measurements with higher precision take more time, consult datasheet for more information
  ret = HP303BPressureSensor.measureTempOnce(temperature, oversampling);

  if (ret != 0)
  {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C (");
    Serial.print(((float)temperature * 9/5)+32.0);
    Serial.println(" °F)");
  }
#endif

  Serial.print("["); serial_print_uptime(); Serial.print("] ");
  //Pressure measurement behaves like temperature measurement
  //ret = HP303BPressureSensor.measurePressureOnce(pressure);
  ret = HP303BPressureSensor.measurePressureOnce(pressure, oversampling);
  if (ret != 0)
  {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.print(" Pa (");
    Serial.print((float)pressure/3386.39);
    Serial.println(" in Hg)");
    store_reading(pressure);
  }

  dump_readings();


  //Wait some time
  Serial.print("["); serial_print_uptime(); Serial.println("]");
  deep_sleep(15000000);
}
