#include "project_config.h"

#include <Arduino.h>
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <WiFiManager.h>

#include "connectivity.h"
#include "persistent.h"
#include "rtc_mem.h"


/* Global Data Structures */
String config_hint_node_name;
String config_hint_report_host_name;
String config_hint_report_host_port;
String config_hint_clock_calib;
String config_hint_temp_calib;
String config_hint_humidity_calib;
String config_hint_pressure_calib;
const char* config_label_node_name        = "><label for=\"" PERSISTENT_NODE_NAME "\">Friendly Name for this Sensor</label";
const char* config_label_report_host_name = "><label for=\"" PERSISTENT_REPORT_HOST_NAME "\">Custom Report Server</label";
const char* config_label_report_host_port = "><label for=\"" PERSISTENT_REPORT_HOST_PORT "\">Custom Report Server Port Number</label";
const char* config_label_clock_calib      = "><label for=\"" PERSISTENT_CLOCK_CALIB "\">Clock Drift Compensation</label";
const char* config_label_temp_calib       = "><label for=\"" PERSISTENT_TEMP_CALIB "\">Temperatue Offset Calibration</label";
const char* config_label_humidity_calib   = "><label for=\"" PERSISTENT_HUMIDITY_CALIB "\">Humidity Offset Calibration</label";
const char* config_label_pressure_calib   = "><label for=\"" PERSISTENT_PRESSURE_CALIB "\">Pressure Offset Calibration</label";
String nodename;
unsigned long server_shutdown_timeout;

/* Function Prototypes */
static String format_u64(uint64_t val);
static String json_header(void);
static bool transmit_readings(WiFiClient& client);
static bool update_firmware(WiFiClient& client);


/* Functions */
// helper to format a uint64_t as a String object
static String format_u64(uint64_t val)
{
  char llu[21];
  llu[20]='\0';
  sprintf(llu, "%llu", val);
  return String(llu);
}

// initializer called from the preinit() function
void connectivity_preinit(void)
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}

// initializer called from setup()
void connectivity_init(void)
{
  nodename = persistent_read(PERSISTENT_NODE_NAME);
  if (nodename == NULL) {
    nodename = String(DEFAULT_NODE_BASE_NAME);
    nodename += String(ESP.getChipId());
  }
}

// shutdown wifi
void connectivity_disable(void)
{
  WiFi.mode(WIFI_OFF);
}

// connect to the stored WiFi AP and return the status
bool connect_wifi(void)
{
  wl_status_t wifi_status;

  Serial.println("Connecting to AP");
  WiFiManager wifi_manager;
  wifi_manager.setConfigPortalTimeout(1);
  wifi_manager.autoConnect();

  wifi_status = WiFi.status();
#if (EXTRA_DEBUG != 0)
  Serial.print("WiFi status ");
  switch(wifi_status) {
    case WL_NO_SHIELD: Serial.println("WL_NO_SHIELD"); break;
    case WL_IDLE_STATUS: Serial.println("WL_IDLE_STATUS"); break;
    case WL_NO_SSID_AVAIL: Serial.println("WL_NO_SSID_AVAIL"); break;
    case WL_SCAN_COMPLETED: Serial.println("WL_SCAN_COMPLETED"); break;
    case WL_CONNECTED: Serial.println("WL_CONNECTED"); break;
    case WL_CONNECT_FAILED: Serial.println("WL_CONNECT_FAILED"); break;
    case WL_CONNECTION_LOST: Serial.println("WL_CONNECTION_LOST"); break;
    case WL_DISCONNECTED: Serial.println("WL_DISCONNECTED"); break;
    default: Serial.println(wifi_status); break;
  }
#endif

  return (wifi_status == WL_CONNECTED);
}

// run the WiFi configuration mode
void enter_config_mode(void)
{
  WiFiManager wifi_manager;

  config_hint_node_name        = persistent_read(PERSISTENT_NODE_NAME,        nodename);
  config_hint_report_host_name = persistent_read(PERSISTENT_REPORT_HOST_NAME, "report server hostname/IP");
  config_hint_report_host_port = persistent_read(PERSISTENT_REPORT_HOST_PORT, "report server port number");
  config_hint_clock_calib      = persistent_read(PERSISTENT_CLOCK_CALIB,      "clock drift (ms)");
  config_hint_temp_calib       = persistent_read(PERSISTENT_TEMP_CALIB,       "temperature calibration (°C)");
  config_hint_humidity_calib   = persistent_read(PERSISTENT_HUMIDITY_CALIB,   "humidity calibration (%)");
  config_hint_pressure_calib   = persistent_read(PERSISTENT_PRESSURE_CALIB,   "pressure calibration (kPa)");

  WiFiManagerParameter custom_node_name(  PERSISTENT_NODE_NAME,        config_hint_node_name.c_str(),        NULL, 31, config_label_node_name);
  WiFiManagerParameter custom_report_host(PERSISTENT_REPORT_HOST_NAME, config_hint_report_host_name.c_str(), NULL, 40, config_label_report_host_name);
  WiFiManagerParameter custom_report_port(PERSISTENT_REPORT_HOST_PORT, config_hint_report_host_port.c_str(), NULL,  5, config_label_report_host_port);
  WiFiManagerParameter clock_drift_adj(   PERSISTENT_CLOCK_CALIB,      config_hint_clock_calib.c_str(),      NULL,  5, config_label_clock_calib);
  WiFiManagerParameter temp_adj(          PERSISTENT_TEMP_CALIB,       config_hint_temp_calib.c_str(),       NULL,  6, config_label_temp_calib);
  WiFiManagerParameter humidity_adj(      PERSISTENT_HUMIDITY_CALIB,   config_hint_humidity_calib.c_str(),   NULL,  6, config_label_humidity_calib);
  WiFiManagerParameter pressure_adj(      PERSISTENT_PRESSURE_CALIB,   config_hint_pressure_calib.c_str(),   NULL,  8, config_label_pressure_calib);

  wifi_manager.addParameter(&custom_node_name);
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
    const char* value;
    value = custom_node_name.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_NODE_NAME, value);
    value = custom_report_host.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_REPORT_HOST_NAME, value);
    value = custom_report_port.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_REPORT_HOST_PORT, value);
    value = clock_drift_adj.getValue();
    if (value && value[0]) {
      flags_time_t *timestruct = (flags_time_t*) &rtc_mem[RTC_MEM_FLAGS_TIME];
      String strValue = value;
      int clock_cal = strValue.toInt();
      if (clock_cal > 0) {
        persistent_write(PERSISTENT_CLOCK_CALIB, strValue);
        timestruct->clock_cal = clock_cal;
      } else {
        timestruct->clock_cal = DEFAULT_SLEEP_CLOCK_ADJ;
        persistent_write(PERSISTENT_CLOCK_CALIB, String(DEFAULT_SLEEP_CLOCK_ADJ));
      }
    }
    value = temp_adj.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_TEMP_CALIB, value);
    value = humidity_adj.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_HUMIDITY_CALIB, value);
    value = pressure_adj.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_PRESSURE_CALIB, value);
  }
}

// manage the uploading of the readings to the report server
void upload_readings(void)
{
  WiFiClient client;
  String report_host_name = persistent_read(PERSISTENT_REPORT_HOST_NAME, String(DEFAULT_REPORT_HOST_NAME));
  uint16_t report_host_port = persistent_read(PERSISTENT_REPORT_HOST_PORT, (int)DEFAULT_REPORT_HOST_PORT);
  String response;
  unsigned long timeout;

  Serial.print("Connecting to report server ");
  Serial.print(report_host_name);
  Serial.print(":");
  Serial.println(report_host_port);
  if (!client.connect(report_host_name, report_host_port)) {
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
          response = client.readStringUntil(0);

          Serial.print("Response from report server: ");
          Serial.println(response);
          if (response.startsWith("OK")) {
            clear_readings();
            response.replace("OK,","");
          }

#if !DEVELOPMENT_BUILD
          if (response.startsWith("update")) {
            Serial.println("accepted firmware update command");
            if (!update_firmware(client))
              Serial.println("error: update failed");
          }
#endif
        }
      }
    }
  }

  client.stop();
  delay(10);
}

// transmit the readings formatted as a json string
static bool transmit_readings(WiFiClient& client)
{
  unsigned result;
  String json;

  json = json_header();
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
    float temp_cal = persistent_read(PERSISTENT_TEMP_CALIB, DEFAULT_TEMP_CALIB);
    float humidity_cal = persistent_read(PERSISTENT_HUMIDITY_CALIB, DEFAULT_HUMIDITY_CALIB);
    float pressure_cal = persistent_read(PERSISTENT_PRESSURE_CALIB, DEFAULT_PRESSURE_CALIB);

    for (unsigned i=0; i < rtc_mem[RTC_MEM_NUM_READINGS]; i++) {
      sensor_reading_t *reading;
      const char* type;
      int slot;
      float calibrated_reading;

      // find the slot indexed into the ring buffer
      slot = rtc_mem[RTC_MEM_FIRST_READING] + i;
      if (slot >= NUM_STORAGE_SLOTS)
        slot -= NUM_STORAGE_SLOTS;

      reading = (sensor_reading_t*) &rtc_mem[RTC_MEM_DATA+slot*NUM_WORDS(sensor_reading_t)];
      calibrated_reading = reading->reading/1000.0;

      // determine the sensor type
      switch(reading->type) {
        case SENSOR_TEMPERATURE:
          type=typestrings[1];
          calibrated_reading += temp_cal;
        break;

        case SENSOR_HUMIDITY:
          type=typestrings[2];
          calibrated_reading += humidity_cal;
        break;

        case SENSOR_PRESSURE:
          type=typestrings[3];
          calibrated_reading += pressure_cal;
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
      json += calibrated_reading;
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

// helper to generate a json-formatted header that can have
// commands or data appended to it
static String json_header(void)
{
  String json;

  //format header
  json = "{\"version\":1,";
  json += "\"timestamp\":"+format_u64(uptime())+",";
  json += "\"node\":\""+nodename+"\",";
  json += "\"firmware\":\""+String(preinit_magic, HEX)+"\",";

  return json;
}

// helper to fetch a firmware update from the server and apply it
bool update_firmware(WiFiClient& client)
{
  String json = json_header();
  unsigned len;
  bool status = false;

  json += "\"command\":\"update\",";
  json += "\"arg\":\"internet_of_spores\"}";

  len = client.print(json);
  if (len != json.length())
    Serial.println("warning: update command not fully transmitted");

  String filesize = client.readStringUntil('\n');
  String md5sum = client.readStringUntil('\n');
  len = filesize.toInt();
  if (len == 0)
    Serial.println("Recieved 0 byte len response - check server logs");

  if (len > 0) {
    Serial.println("Receiving firmware update...");
    Serial.print("File Size = ");  Serial.println(filesize);
    Serial.print("MD5 = ");  Serial.println(md5sum);
    if (!Update.setMD5(md5sum.c_str())) {
      Serial.println("error: unable to set md5sum");
    } else {
      // At this point we are committed - at the end of this block
      // we will reset the processor.
      invalidate_rtc();

      Update.onProgress(
        [](size_t count, size_t total)
        {
          Serial.printf("progress: %06zd / %06zd\n", count, total);
        }
      );
      status = ESP.updateSketch(client, len, false, false);
      Serial.print("Update result: "); Serial.println(status?"OK":"failed");

      // I think we need to reset regardless of the status because
      // the update process may store a bootloader command in the
      // first 128 bytes of RTC user memory. However, it isn't clear
      // that it actually does this...
      client.stop();
      delay(10);
      ESP.reset();
    }
  }

  //this function only returns false
  return status;
}
