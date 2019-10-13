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
String config_hint_battery_calib;
const char* config_label_node_name        = "><label for=\"" PERSISTENT_NODE_NAME "\">Friendly Name for this Sensor</label";
const char* config_label_report_host_name = "><label for=\"" PERSISTENT_REPORT_HOST_NAME "\">Custom Report Server</label";
const char* config_label_report_host_port = "><label for=\"" PERSISTENT_REPORT_HOST_PORT "\">Custom Report Server Port Number</label";
const char* config_label_clock_calib      = "><label for=\"" PERSISTENT_CLOCK_CALIB "\">Clock Drift Compensation</label";
const char* config_label_temp_calib       = "><label for=\"" PERSISTENT_TEMP_CALIB "\">Temperatue Offset Calibration</label";
const char* config_label_humidity_calib   = "><label for=\"" PERSISTENT_HUMIDITY_CALIB "\">Humidity Offset Calibration</label";
const char* config_label_pressure_calib   = "><label for=\"" PERSISTENT_PRESSURE_CALIB "\">Pressure Offset Calibration</label";
const char* config_label_battery_calib    = "><label for=\"" PERSISTENT_BATTERY_CALIB "\">Battery Offset Calibration</label";
String nodename;
unsigned long server_shutdown_timeout;

/* Function Prototypes */
static String format_u64(uint64_t val);
static String json_header(void);
static int transmit_readings(WiFiClient& client, float calibrations[4]);
#if !DEVELOPMENT_BUILD
static bool update_firmware(WiFiClient& client);
#endif


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
  wl_status_t wifi_status = WL_DISCONNECTED;
  unsigned long timeout;

  Serial.println("Connecting to AP");
  WiFi.mode(WIFI_STA);
  WiFi.reconnect();

  //make our own "waitForConnectResult" so we can have a timeout shorter than 250 seconds
  timeout = millis();
  while ((millis()-timeout) < WIFI_CONNECT_TIMEOUT) {
    wifi_status = WiFi.status();
    if (wifi_status != WL_DISCONNECTED)
      break;
    delay(100);
  }

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
  config_hint_battery_calib    = persistent_read(PERSISTENT_BATTERY_CALIB,    "battery calibration (V)");

  WiFiManagerParameter custom_node_name(  PERSISTENT_NODE_NAME,        config_hint_node_name.c_str(),        NULL, 31, config_label_node_name);
  WiFiManagerParameter custom_report_host(PERSISTENT_REPORT_HOST_NAME, config_hint_report_host_name.c_str(), NULL, 40, config_label_report_host_name);
  WiFiManagerParameter custom_report_port(PERSISTENT_REPORT_HOST_PORT, config_hint_report_host_port.c_str(), NULL,  5, config_label_report_host_port);
  WiFiManagerParameter clock_drift_adj(   PERSISTENT_CLOCK_CALIB,      config_hint_clock_calib.c_str(),      NULL,  5, config_label_clock_calib);
  WiFiManagerParameter temp_adj(          PERSISTENT_TEMP_CALIB,       config_hint_temp_calib.c_str(),       NULL,  6, config_label_temp_calib);
  WiFiManagerParameter humidity_adj(      PERSISTENT_HUMIDITY_CALIB,   config_hint_humidity_calib.c_str(),   NULL,  6, config_label_humidity_calib);
  WiFiManagerParameter pressure_adj(      PERSISTENT_PRESSURE_CALIB,   config_hint_pressure_calib.c_str(),   NULL,  8, config_label_pressure_calib);
  WiFiManagerParameter battery_adj(       PERSISTENT_BATTERY_CALIB,    config_hint_battery_calib.c_str(),    NULL,  6, config_label_battery_calib);

  wifi_manager.addParameter(&custom_node_name);
  wifi_manager.addParameter(&custom_report_host);
  wifi_manager.addParameter(&custom_report_port);
  wifi_manager.addParameter(&clock_drift_adj);
  wifi_manager.addParameter(&temp_adj);
  wifi_manager.addParameter(&humidity_adj);
  wifi_manager.addParameter(&pressure_adj);
  wifi_manager.addParameter(&battery_adj);
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
    value = battery_adj.getValue();
    if (value && value[0])
      persistent_write(PERSISTENT_BATTERY_CALIB, value);
  }
}

// manage the uploading of the readings to the report server
void upload_readings(void)
{
  WiFiClient client;
  String report_host_name;
  String response;
  float calibrations[4];
  unsigned long timeout;
  int xmit_status;
  uint16_t report_host_port;
  bool update_flag = false;

  report_host_name =  persistent_read(PERSISTENT_REPORT_HOST_NAME, String(DEFAULT_REPORT_HOST_NAME));
  report_host_port = persistent_read(PERSISTENT_REPORT_HOST_PORT, (int)DEFAULT_REPORT_HOST_PORT);
  calibrations[0] = persistent_read(PERSISTENT_TEMP_CALIB, DEFAULT_TEMP_CALIB);
  calibrations[1] = persistent_read(PERSISTENT_HUMIDITY_CALIB, DEFAULT_HUMIDITY_CALIB);
  calibrations[2] = persistent_read(PERSISTENT_PRESSURE_CALIB, DEFAULT_PRESSURE_CALIB);
  calibrations[3] = persistent_read(PERSISTENT_BATTERY_CALIB, DEFAULT_BATTERY_CALIB);

  Serial.print("Connecting to report server ");
  Serial.print(report_host_name);
  Serial.print(":");
  Serial.println(report_host_port);
  if (!client.connect(report_host_name, report_host_port)) {
    Serial.println("Connection Failed");
  } else {
    // This will send a string to the server
    Serial.println("Sending data to report server");
    while (rtc_mem[RTC_MEM_NUM_READINGS] > 0) {
      xmit_status = transmit_readings(client, calibrations);
      if (xmit_status <= 0) {
        break;
      } else {
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
            clear_readings(xmit_status);
            response.replace("OK,","");
          }

          // no real error handling, just remove flag and check for update
          if (response.startsWith("error")) {
            response.replace("error,","");
            client.stop();  // don't try to send any more readings
          }

          if (response.startsWith("update"))
            update_flag = true;
        } else {
          // some error occurred and we got no response...
          client.stop();
        }
      }
    }

    if (update_flag) {
      Serial.println("accepted firmware update command");
#if !DEVELOPMENT_BUILD
      if (!update_firmware(client))
        Serial.println("error: update failed");
#endif
    }
  }

  client.stop();
  delay(10);
}

// transmit the readings formatted as a json string
// calibrations[0] - temperature offset calibration
// calibrations[1] - humidity offset calibration
// calibrations[2] - pressure offset calibration
// calibrations[3] - battery offset calibration
static int transmit_readings(WiFiClient& client, float calibrations[4])
{
  int num_slots_read = 0;
  int num_measurements = 0;
  unsigned result;
  String json;

  if (!client.connected())
    return -1;

  if (rtc_mem[RTC_MEM_NUM_READINGS] > 0) {
    flags_time_t timestamp = {0,0,0};
    const char typestrings[][12] = {
      "unknown",
      "temperature",
      "humidity",
      "pressure",
      "particles",
      "battery",
    };

    json = json_header();
    json += "\"measurements\":[";

    // format measurements
    for (unsigned i=0; i < rtc_mem[RTC_MEM_NUM_READINGS]; i++) {
      sensor_reading_t *reading;
      const char* type = typestrings[0];
      int slot;
      float calibrated_reading;

      // find the slot indexed into the ring buffer
      slot = rtc_mem[RTC_MEM_FIRST_READING] + i;
      if (slot >= NUM_STORAGE_SLOTS)
        slot -= NUM_STORAGE_SLOTS;

      reading = (sensor_reading_t*) &rtc_mem[RTC_MEM_DATA+slot*NUM_WORDS(sensor_reading_t)];
      calibrated_reading = reading->value/1000.0;
      num_slots_read++;

      // determine the sensor type
      switch(reading->type) {
        case SENSOR_TEMPERATURE:
          type=typestrings[1];
          calibrated_reading += calibrations[0];
        break;

        case SENSOR_HUMIDITY:
          type=typestrings[2];
          calibrated_reading += calibrations[1];
        break;

        case SENSOR_PRESSURE:
          type=typestrings[3];
          calibrated_reading += calibrations[2];
        break;

        case SENSOR_PARTICLE:
          type=typestrings[4];
        break;

        case SENSOR_BATTERY_VOLTAGE:
          type=typestrings[5];
          calibrated_reading += calibrations[3];
        break;

        case SENSOR_TIMESTAMP_L:
          // collect timestamp low bits then loop to get timestamp high bits
          timestamp.millis = reading->value & SENSOR_TIMESTAMP_MASK;
          continue;
        break;

        case SENSOR_TIMESTAMP_H:
          //collect timestamp high bits then break out of the loop to send the message
          timestamp.millis |= ((uint64_t)reading->value & SENSOR_TIMESTAMP_MASK) << SENSOR_TIMESTAMP_SHIFT;
        break;

        case SENSOR_UNKNOWN:
        default:
          type=typestrings[0];
        break;
      }

      if (reading->type == SENSOR_TIMESTAMP_H) {
        if (num_measurements > 0)
          break;
        else
          continue;
      }

      json += "{\"type\":\"";
      json += type;
      json += "\",\"value\":";
      json += String(calibrated_reading, 3);
      json += "},";
      num_measurements++;
    }

    // add a bonus "uptime" reading
    if ((unsigned)num_slots_read == rtc_mem[RTC_MEM_NUM_READINGS]) {
      json += "{\"type\":\"uptime\",";
      json += "\"value\":"+String(uptime()/1000.0, 3);
      json += "}";
    } else if (json.endsWith("},")) {
      json.remove(json.length()-1); // remove the extraneous comma
    }

    // close off the array of readings
    json += "],";

    // append a timestamp
    json += "\"time_offset\":-";
    json += format_u64(uptime()-timestamp.millis);

    // terminate the json object
    json += "}";
    // null-terminate
    json += ";";
    json.setCharAt(json.length()-1, '\0');
  }

  if (num_measurements > 0) {
#if (EXTRA_DEBUG != 0)
    Serial.println("Transmitting to report server:");
    Serial.println(json);
#endif

    // transmit the results and verify the number of bytes written
    result = client.print(json);
    if (result == json.length())
      return num_slots_read;
    else
      return -1;
  }

  return num_slots_read; // no measurements available
}

// helper to generate a json-formatted header that can have
// commands or data appended to it
static String json_header(void)
{
  String json;

  //format header
  json = "{\"version\":2,";
  json += "\"node\":\""+nodename+"\",";
  json += "\"firmware\":\""+String(preinit_magic, HEX)+"\",";

  return json;
}

#if !DEVELOPMENT_BUILD
// helper to fetch a firmware update from the server and apply it
static bool update_firmware(WiFiClient& client)
{
  String json = json_header();
  unsigned len;
  bool status = false;

  if (!client.connected())
    return false;

  json += "\"command\":\"update\",";
  json += "\"arg\":\"internet_of_spores\"}";

  // null-terminate
  json += ";";
  json.setCharAt(json.length()-1, '\0');

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
#endif /* !DEVELOPMENT_BUILD */
