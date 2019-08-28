#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LOLIN_HP303B.h>
#include <GP2YDustSensor.h>
#include "sht30.h"

/* Configurations for IOT App Story */
#define DEBUG_LVL         (2)
#define SERIAL_SPEED      (115200)
#define INC_CONFIG        (false)
#define MAX_WIFI_RETRIES  (2)
#define USEMDNS           (false)
#include <IOTAppStory.h>

/* Global Configurations */
#define COMPDATE          (__DATE__ __TIME__)
#define MODEBUTTON        (0)                    // Button pin on the esp for selecting modes. D3 for the Wemos!
#define SLEEP_TIME_US     (15000000)
//old guesses (before some changes): 100=~.988, 200=1.008, 150=1.001 135=1.00082 130=1.00058 122=1.000078 120=0.999952
#define SLEEP_OVERHEAD_MS (121) //overhead guess (esp/stopwatch): 121=1.000780?, 120=1.000691, 119=1.001416
#define PREINIT_MAGIC     (0xAA559876)
#define NUM_STORAGE_SLOTS (60)
#define GP2Y_LED_PIN      (D5) // Sharp Dust/particle sensor Led Pin
#define GP2Y_VO_PIN       (A0) // Sharp Dust/particle analog out pin used for reading
#define SHT30_ADDR        (0x45)

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

// Labels for the different types of sensor readings
typedef enum sensor_type_e {
  SENSOR_UNKNOWN,
  SENSOR_TEMPERATURE,
  SENSOR_HUMIDITY,
  SENSOR_PRESSURE,
  SENSOR_PARTICLE,
} sensor_type_t;

// Structure to combine sensor readings with type and timestamp
typedef struct sensor_reading_s {
  sensor_type_t  type      :3;
  uint64_t       timestamp :40;
  int32_t        reading   :21;
} sensor_reading_t;

// Fields for each of the 32-bit fields in RTC Memory
enum rtc_mem_fields_e {
  RTC_MEM_CHECK = 0,
  RTC_MEM_BOOT_COUNT,
  RTC_MEM_FLAGS_TIME, //64-bit field
  RTC_MEM_FLAGS_TIME_END = RTC_MEM_FLAGS_TIME + NUM_WORDS(flags_time_t) - 1,
  RTC_MEM_NUM_READINGS,
  RTC_MEM_FIRST_READING,

  //array of sensor readings
  RTC_MEM_DATA,
  RTC_MEM_DATA_END = RTC_MEM_DATA + NUM_STORAGE_SLOTS*NUM_WORDS(sensor_reading_t) - 1,

  //keep last
  RTC_MEM_MAX
};

/* Global Data Structures */
IOTAppStory IAS(COMPDATE, MODEBUTTON);
LOLIN_HP303B HP303BPressureSensor;
GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1014AU0F, GP2Y_LED_PIN, GP2Y_VO_PIN);
uint32_t rtc_mem[RTC_MEM_MAX]; //array for accessing RTC Memory

/* Functions */
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
  flags_time_t *timestruct = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];

  // boot timestamp + current millis()
  uptime_ms = timestruct->millis;
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

// helpers to store pressure readings in the rtc mem ring buffer
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
  reading->reading = val;
  reading->timestamp = uptime();
}

// helper to print the stored pressure readings from the rtc mem ring buffer
void dump_readings(void)
{
  const char typestrings[][11] = {
    "UNKNOWN",
    "TEMP (C)",
    "HUMI (%)",
    "PRES (kPa)",
    "PART (?)",
  };
  char formatted[47];
  formatted[46]=0;

  Serial.println("slot |     timestamp | type       |    value");
  Serial.println("-----+---------------+------------+---------");


  for (unsigned i=0; i < rtc_mem[RTC_MEM_NUM_READINGS]; i++) {
    sensor_reading_t *reading;
    const char* type;
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

      case SENSOR_UNKNOWN:
      default:
        type=typestrings[0];
      break;
    }

    // format an output row with the data, type, and timestamp
    snprintf(formatted, 45, "%4u | %13llu | %-10s | %+8.3f", i, reading->timestamp, type, reading->reading/1000.0);
    Serial.println(formatted);
  }
}

// helper for saving rtc memory before entering deep sleep
void deep_sleep(uint64_t time_us)
{
  flags_time_t *timestruct = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];

  // update the stored millis including some overhead for the write, suspend, and wake
  timestruct->millis += millis() + time_us/1000 + SLEEP_OVERHEAD_MS;

  // update the header checksum
  rtc_mem[RTC_MEM_CHECK] = PREINIT_MAGIC - rtc_mem[RTC_MEM_BOOT_COUNT];

  // store the array to RTC memory and enter suspend
  ESP.rtcUserMemoryWrite(0, rtc_mem, sizeof(rtc_mem));
  ESP.deepSleep(time_us, RF_DISABLED);
}

// helper to read and store values from the SHT30 temperature and humidity sensor
bool read_sht30(void)
{
  sht30_data_t data;
  int ret;
  ret = sht30_get(SHT30_ADDR, SHT30_RPT_HIGH, &data);
  Serial.println();
  if (ret != 0) {
    Serial.print("Error Reading SHT30 ret=");
    Serial.println(ret);
    return false;
  } else {
    store_reading(SENSOR_TEMPERATURE, sht30_parse_temp_c(data)*1000.0 + 0.5);
    store_reading(SENSOR_HUMIDITY, sht30_parse_humidity(data)*1000.0 + 0.5);
    Serial.print("Temperature: ");
    Serial.print(sht30_parse_temp_c(data), 3);
    Serial.print("°C (");
    Serial.print(sht30_parse_temp_f(data), 3);
    Serial.println("°F)");
    Serial.print("Humidity: ");
    Serial.println(sht30_parse_humidity(data), 3);
  }

  return true;
}

// helper to read and store values from the HP303B barametric pressure sensor
void read_hp303b(bool measure_temp)
{
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
  int16_t oversampling = 4;
  int16_t ret;
  int32_t pressure;

  // Call begin to initialize HP303BPressureSensor
  // The default address is 0x77 and does not need to be given.
  HP303BPressureSensor.begin();

  if (measure_temp) {
    int32_t temperature;
    //lets the HP303B perform a Single temperature measurement with the last (or standard) configuration
    //The result will be written to the paramerter temperature
    //ret = HP303BPressureSensor.measureTempOnce(temperature);
    //the commented line below does exactly the same as the one above, but you can also config the precision
    //oversampling can be a value from 0 to 7
    //the HP303B will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
    //measurements with higher precision take more time, consult datasheet for more information
    ret = HP303BPressureSensor.measureTempOnce(temperature, oversampling);

    if (ret != 0) {
      //Something went wrong.
      //Look at the library code for more information about return codes
      Serial.print("Error Reading HP303B (temperature) ret=");
      Serial.println(ret);
    } else {
      store_reading(SENSOR_TEMPERATURE, temperature*1000);
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C (");
      Serial.print(((float)temperature * 9/5)+32.0);
      Serial.println(" °F)");
    }
  }

  //Pressure measurement behaves like temperature measurement
  //ret = HP303BPressureSensor.measurePressureOnce(pressure);
  ret = HP303BPressureSensor.measurePressureOnce(pressure, oversampling);
  if (ret != 0) {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("Error Reading HP303B (pressure) ret=");
    Serial.println(ret);
  } else {
    store_reading(SENSOR_PRESSURE, pressure);
    Serial.print("Pressure: ");
    Serial.print(pressure/1000.0, 3);
    Serial.print(" kPa (");
    Serial.print((float)pressure/3386.39);
    Serial.println(" in Hg)");
  }
}

void read_gp2y(void)
{
  uint16_t val;
  dustSensor.begin();
  val=dustSensor.getDustDensity();
  store_reading(SENSOR_PARTICLE, ((int)val)*1000);
  Serial.print("Dust density: ");
  Serial.print(val);
  Serial.println(" ng/m³");
}

void setup(void)
{
  bool sht30_ok;
  int gp2y_ok;

  Wire.begin();
  Serial.begin(SERIAL_SPEED);
  delay(2);
  Serial.println();

  ESP.rtcUserMemoryRead(0, rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[RTC_MEM_CHECK] + rtc_mem[RTC_MEM_BOOT_COUNT] != PREINIT_MAGIC)
    memset(rtc_mem, 0, sizeof(rtc_mem));
  rtc_mem[RTC_MEM_BOOT_COUNT]++;

  pinMode(GP2Y_LED_PIN, INPUT);
  gp2y_ok = digitalRead(GP2Y_LED_PIN);

  Serial.print("["); serial_print_uptime(); Serial.print("] ");
  Serial.print("setup: boot count=");
  Serial.print(rtc_mem[RTC_MEM_BOOT_COUNT]);
  Serial.print(", num readings=");
  Serial.print(rtc_mem[RTC_MEM_NUM_READINGS]);
  Serial.print(", GP2Y status=");
  Serial.println(gp2y_ok);
  if (RTC_MEM_MAX > 128)
    Serial.println("*************************\nWARNING RTC_MEM_MAX > 128\n*************************");

  IAS.preSetDeviceName("spores");
  //IAS.begin('P');

  //read temp/humidity from SHT30
  sht30_ok = read_sht30();

  //read temp/humidity from HP303B
  read_hp303b(!sht30_ok);

  if (gp2y_ok) {
    read_gp2y();
  }

  dump_readings();

  Serial.print("["); serial_print_uptime(); Serial.println("]...");
  deep_sleep(SLEEP_TIME_US);
}

void loop(void)
{
  //should never get here
  deep_sleep(SLEEP_TIME_US);
}
