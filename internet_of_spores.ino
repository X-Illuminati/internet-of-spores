#include "project_config.h"

#include <ESP8266WiFi.h>
#ifdef IOTAPPSTORY
#include <IOTAppStory.h>
#else
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#endif

#include "rtc_mem.h"
#include "sensors.h"


/* Global Data Structures */
#ifdef IOTAPPSTORY
IOTAppStory IAS(COMPDATE, MODEBUTTON);
#endif
String nodename = NODE_BASE_NAME;
unsigned long server_shutdown_timeout;


/* Functions */
void preinit()
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}

// helper to format a uint64_t as a String object
String format_u64(uint64_t val)
{
  char llu[21];
  llu[20]='\0';
  sprintf(llu, "%llu", val);
  return String(llu);
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

// helper to run the WiFi configuration mode
void enter_config_mode(void)
{
#ifdef IOTAPPSTORY
    invalidate_rtc(); //invalidate existing RTC memory, IAS will overwrite it...
    IAS.begin('P');
    IAS.runConfigServer();
#else
    WiFiManager wifi_manager;
    WiFiManagerParameter custom_report_host("report_host", "report server hostname/IP", "", 40, "><label for=\"report_host\">Custom Report Server</label");
    WiFiManagerParameter custom_report_port("report_port", "report server port number", "2880", 40, "><label for=\"report_port\">Custom Report Server Port Number</label");
    WiFiManagerParameter clock_drift_adj("clock_drift", "clock drift (ms)", String(SLEEP_OVERHEAD_MS).c_str(), 6, "><label for=\"clock_drift\">Clock Drift Compensation</label");
    WiFiManagerParameter temp_adj("temp_adj", "temperature calibration (°C)", "0.0", 10, "><label for=\"temp_adj\">Temperatue Offset Calibration</label");
    WiFiManagerParameter humidity_adj("humidity_adj", "humidity calibration (%)", "0.0", 10, "><label for=\"humidity_adj\">Humidity Offset Calibration</label");
    WiFiManagerParameter pressure_adj("pressure_adj", "pressure calibration (kPa)", "0.0", 10, "><label for=\"pressure_adj\">Pressure Offset Calibration</label");

    wifi_manager.addParameter(&custom_report_host);
    wifi_manager.addParameter(&custom_report_port);
    wifi_manager.addParameter(&clock_drift_adj);
    wifi_manager.addParameter(&temp_adj);
    wifi_manager.addParameter(&humidity_adj);
    wifi_manager.addParameter(&pressure_adj);
    wifi_manager.setConfigPortalTimeout(CONFIG_SERVER_MAX_TIME);

    Serial.println("Starting config server");
    Serial.print("Connect to AP ");
    Serial.println(nodename);
    if (wifi_manager.startConfigPortal(nodename.c_str())) {
      // TODO save config settings
    }
#endif
}

// helper to connect to the WiFi and return the status
wl_status_t connect_wifi(void)
{
  wl_status_t retval;

#ifdef IOTAPPSTORY
  IAS.begin('P');
#else
  Serial.println("Connecting to AP");
  WiFiManager wifi_manager;
  wifi_manager.setConfigPortalTimeout(1);
  wifi_manager.autoConnect();
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
  bool rtc_config_valid = false;

  pinMode(LED_BUILTIN, INPUT);

  Serial.begin(SERIAL_SPEED);
  delay(2);
  Serial.println();

  sensors_init();

  nodename += String(ESP.getChipId());

  rtc_config_valid = load_rtc_memory();

#ifdef IOTAPPSTORY
  IAS.preSetDeviceName(nodename);
  IAS.preSetAutoUpdate(false);
  IAS.setCallHome(false);
#endif

  // We can detect a "double press" of the reset button as a regular Ext Reset
  // This is because we spend most of our time asleep and a single press will
  // generally appear as a deep-sleep wakeup.
  // We will use this to enter the WiFi config mode.
  // However, we must ignore it on the first boot after reprogramming or inserting
  // the battery -- so only pay attention if the RTC memory checksum is OK.
  if ((ESP.getResetInfoPtr()->reason == REASON_EXT_SYS_RST) && rtc_config_valid) {
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
