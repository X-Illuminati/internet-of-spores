#include <ESP8266WiFi.h>
#ifdef IOTAPPSTORY
#include <IOTAppStory.h>
#else
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#endif
#include <Wire.h>
#include <LOLIN_HP303B.h>
#include "sht30.h"
#include "adc.h"


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
#define PREINIT_MAGIC           (0xAA559876)
#define NUM_STORAGE_SLOTS       (59)
#define HIGH_WATER_SLOT         (NUM_STORAGE_SLOTS-12)
#define SHT30_ADDR              (0x45)
#define REPORT_HOST_NAME        ("boxy")
#define REPORT_HOST_PORT        (2880)
#define REPORT_RESPONSE_TIMEOUT (2000)
#define NODE_BASE_NAME          ("spores-")

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
  SENSOR_BATTERY_VOLTAGE,
} sensor_type_t;

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
#ifdef IOTAPPSTORY
IOTAppStory IAS(COMPDATE, MODEBUTTON);
#else
ESP8266WebServer config_server(80);
#endif
LOLIN_HP303B HP303BPressureSensor;
String nodename = NODE_BASE_NAME;
uint32_t rtc_mem[RTC_MEM_MAX]; //array for accessing RTC Memory
unsigned long server_shutdown_timeout;

void handleRoot() {
  config_server.send(200, "text/html", "<h1>You are connected</h1>");
}

void handleReboot() {
  config_server.send(200, "text/html", "<h1>Rebooting</h1>");
  server_shutdown_timeout = millis() + 200;
}

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

// helper to format a uint64_t as a String object
String format_u64(uint64_t val)
{
  char llu[21];
  llu[20]='\0';
  sprintf(llu, "%llu", val);
  return String(llu);
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

// helper to reset the rtc mem ring buffer
void clear_readings(void)
{
  // determine the next free slot in the RTC memory ring buffer
  rtc_mem[RTC_MEM_FIRST_READING]=0;
  rtc_mem[RTC_MEM_NUM_READINGS]=0;
}

// helper to print the stored pressure readings from the rtc mem ring buffer
void dump_readings(void)
{
#if (EXTRA_DEBUG != 0)
  const char typestrings[][11] = {
    "UNKNOWN",
    "TEMP (C)",
    "HUMI (%)",
    "PRES (kPa)",
    "PART (?)",
    "BATT (V)",
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

      case SENSOR_BATTERY_VOLTAGE:
        type=typestrings[5];
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
  Serial.println("[" + format_u64(uptime()) + "] dump complete");
#endif
}

// helper to transmit the readings formatted as a json string
bool transmit_readings(WiFiClient& client)
{
  unsigned result;
  String json;

  //format preamble
  json  = "{\"version\":1,\"timestamp\":";
  json += format_u64(uptime());
  json += ",\"node\":\""+nodename+"\",";
  json += "\"measurements\":[";

  //format measurements
  {
    const char typestrings[][12] = {
      "unknown",
      "temperature",
      "humidity",
      "pressure",
      "particles",
      "battery",
    };

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
  
        case SENSOR_BATTERY_VOLTAGE:
          type=typestrings[5];
        break;
  
        case SENSOR_UNKNOWN:
        default:
          type=typestrings[0];
        break;
      }
      json += "{\"timestamp\":";
      json += format_u64(reading->timestamp);
      json +=",\"type\":\"";
      json += type;
      json += "\",\"value\":";
      json += reading->reading/1000.0;
      json += "}";
      if ((i+1) < rtc_mem[RTC_MEM_NUM_READINGS])
        json += ",";
    }
  }

  //terminate the json objects
  json += "]}";

#if (EXTRA_DEBUG != 0)
  Serial.println("Transmitting to report server:");
  Serial.println(json);
#endif

  //transmit the results and verify the number of bytes written
  result = client.print(json);
  return (result == json.length());
}

// helper to manage uploading the readings to the report server
void upload_readings(void)
{
  WiFiClient client;
  String response;
  unsigned long timeout;

  Serial.print("Connecting to report server ");
  Serial.print(REPORT_HOST_NAME);
  Serial.print(":");
  Serial.println(REPORT_HOST_PORT);
  if (!client.connect(REPORT_HOST_NAME, REPORT_HOST_PORT)) {
    Serial.println("Connection Failed");
  } else {
    // This will send a string to the server
    Serial.println("Sending data to report server");
    if (client.connected()) {
      if (transmit_readings(client)) {
        // wait for response to be available
        timeout = millis();
        while (client.available() == 0) {
          if (millis() - timeout > REPORT_RESPONSE_TIMEOUT) {
            Serial.println("Timeout waiting for response!");
            break;
          }
          yield();
        }

        if (client.available()) {
          // Read response from the report server
          response = String("");
          while (client.available()) {
            char ch = static_cast<char>(client.read());
            response += ch;
          }
    
          Serial.print("Response from report server: ");
          Serial.println(response);
          if (response == "OK")
            clear_readings();
        }
      }
    }
  }

  client.stop();
  delay(10);
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
#if (EXTRA_DEBUG != 0)
    Serial.print("Temperature: ");
    Serial.print(sht30_parse_temp_c(data), 3);
    Serial.print("°C (");
    Serial.print(sht30_parse_temp_f(data), 3);
    Serial.println("°F)");
    Serial.print("Humidity: ");
    Serial.println(sht30_parse_humidity(data), 3);
#endif
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
#if (EXTRA_DEBUG != 0)
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C (");
      Serial.print(((float)temperature * 9/5)+32.0);
      Serial.println(" °F)");
#endif
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
#if (EXTRA_DEBUG != 0)
    Serial.print("Pressure: ");
    Serial.print(pressure/1000.0, 3);
    Serial.print(" kPa (");
    Serial.print((float)pressure/3386.39);
    Serial.println(" in Hg)");
#endif
  }
}

// helper to read the ESP VCC voltage level
void read_vcc(void)
{
  uint16_t val;
  val = ESP.getVcc();

  if (val > 37000)
    Serial.println("Error reading VCC value");
  else
    store_reading(SENSOR_BATTERY_VOLTAGE, val);

#if (EXTRA_DEBUG != 0)
  Serial.print("Battery voltage: ");
  Serial.print(val/1000.0, 3);
  Serial.println("V");
#endif
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

// helper to clear/reinitialize rtc memory
void invalidate_rtc(void)
{
  memset(rtc_mem, 0, sizeof(rtc_mem));
  ESP.rtcUserMemoryWrite(0, rtc_mem, sizeof(rtc_mem));
}

// helper to run the WiFi configuration mode
void enter_config_mode(void)
{
#ifdef IOTAPPSTORY
    invalidate_rtc(); //invalidate existing RTC memory, IAS will overwrite it...
    IAS.begin('P');
    IAS.runConfigServer();
#else
    Serial.println("Starting config server");
    WiFi.softAP(nodename);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("Connect to AP ");
    Serial.print(nodename);
    Serial.print(" and open ");
    Serial.print(myIP);
    Serial.println(" in a web browser");
    config_server.on("/", handleRoot);
    config_server.on("/reboot", handleReboot);
    config_server.begin();
    server_shutdown_timeout = millis() + 600000;
    {
      int counter=0;
      pinMode(LED_BUILTIN, OUTPUT);
      while(millis() < server_shutdown_timeout) {
        if ((counter++ & 0xffff) == 0)
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        config_server.handleClient();
      }
      pinMode(LED_BUILTIN, INPUT);
    }
    config_server.stop();
#endif
}

// helper to connect to the WiFi and return the status
wl_status_t connect_wifi(void)
{
  wl_status_t retval;

#ifdef IOTAPPSTORY
  IAS.begin('P');
#else
  Serial.println("Connecting to Hot for pixel");
  WiFi.mode(WIFI_STA);
  WiFi.reconnect();
  WiFi.waitForConnectResult();
#endif

  retval = WiFi.status();
#if (EXTRA_DEBUG != 0)
  Serial.print("WiFi status ");
  switch(retval) {
    case WL_NO_SHIELD: Serial.println("WL_NO_SHIELD"); break;
    case WL_IDLE_STATUS: Serial.println("WL_IDLE_STATUS"); break;
    case WL_NO_SSID_AVAIL: Serial.println("WL_NO_SSID_AVAIL"); break;
    case WL_SCAN_COMPLETED: Serial.println("WL_SCAN_COMPLETED"); break;
    case WL_CONNECTED: Serial.println("WL_CONNECTED"); break;
    case WL_CONNECT_FAILED: Serial.println("WL_CONNECT_FAILED"); break;
    case WL_CONNECTION_LOST: Serial.println("WL_CONNECTION_LOST"); break;
    case WL_DISCONNECTED: Serial.println("WL_DISCONNECTED"); break;
    default: Serial.println(retval); break;
  }
#endif

  return retval;
}

void setup(void)
{
  pinMode(LED_BUILTIN, INPUT);

  Wire.begin();
  Serial.begin(SERIAL_SPEED);
  delay(2);
  Serial.println();

  ESP.rtcUserMemoryRead(0, rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[RTC_MEM_CHECK] + rtc_mem[RTC_MEM_BOOT_COUNT] != PREINIT_MAGIC) {
    Serial.println("Preinit magic doesn't compute, reinitializing");
    invalidate_rtc();
  }
  rtc_mem[RTC_MEM_BOOT_COUNT]++;

  Serial.print("[" + format_u64(uptime()) + "] ");
  Serial.print("setup: reset reason=");
  Serial.print(ESP.getResetReason());
  Serial.print(", boot count=");
  Serial.print(rtc_mem[RTC_MEM_BOOT_COUNT]);
  Serial.print(", num readings=");
  Serial.println(rtc_mem[RTC_MEM_NUM_READINGS]);

  if (RTC_MEM_MAX > 128)
    Serial.println("*************************\nWARNING RTC_MEM_MAX > 128\n*************************");

  nodename += String(ESP.getChipId());

#ifdef IOTAPPSTORY
  IAS.preSetDeviceName(nodename);
  IAS.preSetAutoUpdate(false);
  IAS.setCallHome(false);
#endif

  if (ESP.getResetInfoPtr()->reason == REASON_EXT_SYS_RST)
  {
    // We can detect a "double press" of the reset button as a regular Ext Reset
    // This is because we spend most of our time asleep and a single press will
    // generally appear as a deep-sleep wakeup.
    // Use this to enter the IOT AppStory Config mode
    enter_config_mode();
  } else {
    take_readings();
    dump_readings();

    if (rtc_mem[RTC_MEM_NUM_READINGS] >= HIGH_WATER_SLOT) {
      // time to connect and upload our readings
      if (WL_CONNECTED == connect_wifi())
        upload_readings();
    }
  }

  WiFi.mode(WIFI_OFF);
  deep_sleep(SLEEP_TIME_US);
}

void loop(void)
{
  //should never get here
  WiFi.mode(WIFI_OFF);
  deep_sleep(SLEEP_TIME_US);
}
