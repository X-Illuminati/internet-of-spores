#include "project_config.h"

#include <Arduino.h>
#include <Esp.h>
#include <ESP8266WiFi.h>
#include <MD5Builder.h>
#include <Updater.h>
#include <WiFiManager.h>
#if EXTRA_DEBUG
#include <user_interface.h>
#endif

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
String config_hint_sleep_time_ms;
String config_hint_high_water_slot;
WiFiManagerParameter* custom_node_name;
WiFiManagerParameter* custom_report_host;
WiFiManagerParameter* custom_report_port;
WiFiManagerParameter* clock_drift_adj;
WiFiManagerParameter* temp_adj;
WiFiManagerParameter* humidity_adj;
WiFiManagerParameter* pressure_adj;
WiFiManagerParameter* battery_adj;
WiFiManagerParameter* custom_sleep_time_ms;
WiFiManagerParameter* custom_high_water_slot;

const char* config_label_node_name        = "><label for=\"" PERSISTENT_NODE_NAME "\">Friendly Name for this Sensor</label";
const char* config_label_report_host_name = "><label for=\"" PERSISTENT_REPORT_HOST_NAME "\">Custom Report Server</label";
const char* config_label_report_host_port = "><label for=\"" PERSISTENT_REPORT_HOST_PORT "\">Custom Report Server Port Number</label";
const char* config_label_clock_calib      = "><label for=\"" PERSISTENT_CLOCK_CALIB "\">Clock Drift Compensation</label";
const char* config_label_temp_calib       = "><label for=\"" PERSISTENT_TEMP_CALIB "\">Temperatue Offset Calibration</label";
const char* config_label_humidity_calib   = "><label for=\"" PERSISTENT_HUMIDITY_CALIB "\">Humidity Offset Calibration</label";
const char* config_label_pressure_calib   = "><label for=\"" PERSISTENT_PRESSURE_CALIB "\">Pressure Offset Calibration</label";
const char* config_label_battery_calib    = "><label for=\"" PERSISTENT_BATTERY_CALIB "\">Battery Offset Calibration</label";
const char* config_label_sleep_time_ms    = "><label for=\"" PERSISTENT_SLEEP_TIME_MS "\">Custom Sleep Period Between Measurements (ms)</label";
const char* config_label_high_water_slot  = "><label for=\"" PERSISTENT_HIGH_WATER_SLOT "\">Upload After #Measurements</label";

String nodename;
unsigned long server_shutdown_timeout;

/* Function Prototypes */
static String format_u64(uint64_t val);
static bool try_connect(float power_level);
static String json_header(void);
static int transmit_readings(WiFiClient& client, float calibrations[4]);
static bool update_config(WiFiClient& client);
#if !DISABLE_FW_UPDATE
static bool update_firmware(WiFiClient& client);
#endif
static bool send_command(WiFiClient& client, String& command);


/* Functions */
// helper to format a uint64_t as a String object
static String format_u64(uint64_t val)
{
  char llu[21];
  llu[20]='\0';
  sprintf(llu, "%llu", val);
  return String(llu);
}

#if !TETHERED_MODE
// initializer called from the preinit() function
void connectivity_preinit(void)
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}
#endif

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

// helper to attempt a WiFi connection with timeout
static bool try_connect(float power_level)
{
  wl_status_t wifi_status = WL_DISCONNECTED;
  unsigned long timeout;

  if (WiFi.isConnected())
  {
    Serial.println("WiFi status is connected");
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setOutputPower(power_level);
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
  Serial.print("WiFi status (power level ");
  Serial.print(power_level);
  Serial.print(") ");
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

// connect to the stored WiFi AP and return the status
bool connect_wifi(void)
{
  bool retval = false;

  if (WiFi.isConnected())
  {
    Serial.println("WiFi status is connected");
    return true;
  }

  Serial.println("Connecting to AP");
#if EXTRA_DEBUG
  struct station_config configdata;
  Serial.printf("WiFi.persistent=%d WiFi.mode=%X\n", WiFi.getPersistent(), WiFi.getMode());
  if (wifi_station_get_config(&configdata))
    Serial.printf("config: ssid=%.*s, password=%.*s\n", (int)sizeof(configdata.ssid), configdata.ssid, (int)sizeof(configdata.password), configdata.password);
#endif

  //recommended output power 17.5 dBm to reduce noise compared to max power 20.5 dBm
  retval = try_connect(17.5f);

  return retval;
}

static void wifi_manager_save_config_callback(void)
{
  const char* value;
  value = custom_node_name->getValue();
  if (value && value[0]) {
    persistent_write(PERSISTENT_NODE_NAME, value);
    nodename = value;
  }
  value = custom_report_host->getValue();
  if (value && value[0])
    persistent_write(PERSISTENT_REPORT_HOST_NAME, value);
  value = custom_report_port->getValue();
  if (value && value[0])
    persistent_write(PERSISTENT_REPORT_HOST_PORT, value);
  value = clock_drift_adj->getValue();
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

  value = temp_adj->getValue();
  if (value && value[0]) {
    char* endptr = (char*)value;
    float tempf;

    //check for valid float
    tempf = strtof(value, &endptr);
    if ((0==tempf) && (value==endptr)) {
      persistent_write(PERSISTENT_TEMP_CALIB, ""); //erase existing humidity calibration
      tempf = DEFAULT_TEMP_CALIB;
      rtc_mem[RTC_MEM_TEMP_CAL] = *((uint32_t*)&tempf);
    } else {
      if (persistent_write(PERSISTENT_TEMP_CALIB, value))
        rtc_mem[RTC_MEM_TEMP_CAL] = *((uint32_t*)&tempf);
    }
  }

  value = humidity_adj->getValue();
  if (value && value[0]) {
    char* endptr = (char*)value;
    float tempf;

    //check for valid float
    tempf = strtof(value, &endptr);
    if ((0==tempf) && (value==endptr)) {
      persistent_write(PERSISTENT_HUMIDITY_CALIB, ""); //erase existing humidity calibration
      tempf = DEFAULT_HUMIDITY_CALIB;
      rtc_mem[RTC_MEM_HUMIDITY_CAL] = *((uint32_t*)&tempf);
    } else {
      if (persistent_write(PERSISTENT_HUMIDITY_CALIB, value))
        rtc_mem[RTC_MEM_HUMIDITY_CAL] = *((uint32_t*)&tempf);
    }
  }

  value = pressure_adj->getValue();
  if (value && value[0])
    persistent_write(PERSISTENT_PRESSURE_CALIB, value);

  value = battery_adj->getValue();
  if (value && value[0])
    persistent_write(PERSISTENT_BATTERY_CALIB, value);

  value = custom_sleep_time_ms->getValue();
  if (value && value[0]) {
    sleep_params_t *sleep_params = (sleep_params_t*) &rtc_mem[RTC_MEM_SLEEP_PARAMS];
    int temp;
    char* endptr = (char*)value;

    //check for valid integer
    temp = strtol(value, &endptr, 0);
    if ((0==temp) && (value==endptr)) {
      persistent_write(PERSISTENT_SLEEP_TIME_MS, ""); //erase existing sleep time
      sleep_params->sleep_time_ms = (int)DEFAULT_SLEEP_TIME_MS;
    } else {
      if (persistent_write(PERSISTENT_SLEEP_TIME_MS, value)) {
        if (temp < 200)
          temp = 200;
        if (temp > 11200000)
          temp = 11200000;
        sleep_params->sleep_time_ms = temp;
      }
    }
  }

  value = custom_high_water_slot->getValue();
  if (value && value[0]) {
    sleep_params_t *sleep_params = (sleep_params_t*) &rtc_mem[RTC_MEM_SLEEP_PARAMS];
    int temp;
    char* endptr = (char*)value;

    //check for valid integer
    temp = strtol(value, &endptr, 0);
    if ((0==temp) && (value==endptr)) {
      persistent_write(PERSISTENT_HIGH_WATER_SLOT, ""); //erase existing high-water slot
      sleep_params->high_water_slot = DEFAULT_HIGH_WATER_SLOT;
    } else {
      if (persistent_write(PERSISTENT_SLEEP_TIME_MS, value)) {
        if (temp <= 0)
          temp = 1;
        if (temp > NUM_STORAGE_SLOTS)
          temp = NUM_STORAGE_SLOTS;
        sleep_params->high_water_slot = temp;
      }
    }
  }
}

// run the WiFi configuration mode
void enter_config_mode(void)
{
  WiFiManager wifi_manager;

  // there is a bug? somewhere that causes WiFi.disconnect() to lose
  // the stored SSID/password when WiFi.mode is STA.
  WiFi.mode(WIFI_OFF); 

  config_hint_node_name        = persistent_read(PERSISTENT_NODE_NAME,        nodename);
  config_hint_report_host_name = persistent_read(PERSISTENT_REPORT_HOST_NAME, "report server hostname/IP");
  config_hint_report_host_port = persistent_read(PERSISTENT_REPORT_HOST_PORT, "report server port number");
  config_hint_clock_calib      = persistent_read(PERSISTENT_CLOCK_CALIB,      "clock drift (ms)");
  config_hint_temp_calib       = persistent_read(PERSISTENT_TEMP_CALIB,       "temperature calibration (°C)");
  config_hint_humidity_calib   = persistent_read(PERSISTENT_HUMIDITY_CALIB,   "humidity calibration (%)");
  config_hint_pressure_calib   = persistent_read(PERSISTENT_PRESSURE_CALIB,   "pressure calibration (kPa)");
  config_hint_battery_calib    = persistent_read(PERSISTENT_BATTERY_CALIB,    "battery calibration (V)");
  config_hint_sleep_time_ms    = persistent_read(PERSISTENT_SLEEP_TIME_MS,    String("sleep time (") + String((int)DEFAULT_SLEEP_TIME_MS) + String(" ms)"));
  config_hint_high_water_slot  = persistent_read(PERSISTENT_HIGH_WATER_SLOT,  String("#measurements (") + String(DEFAULT_HIGH_WATER_SLOT) + String(")"));

  custom_node_name = new WiFiManagerParameter(  PERSISTENT_NODE_NAME,        config_hint_node_name.c_str(),        NULL, 31, config_label_node_name);
  custom_report_host = new WiFiManagerParameter(PERSISTENT_REPORT_HOST_NAME, config_hint_report_host_name.c_str(), NULL, 40, config_label_report_host_name);
  custom_report_port = new WiFiManagerParameter(PERSISTENT_REPORT_HOST_PORT, config_hint_report_host_port.c_str(), NULL,  5, config_label_report_host_port);
  clock_drift_adj = new WiFiManagerParameter(   PERSISTENT_CLOCK_CALIB,      config_hint_clock_calib.c_str(),      NULL,  5, config_label_clock_calib);
  temp_adj = new WiFiManagerParameter(          PERSISTENT_TEMP_CALIB,       config_hint_temp_calib.c_str(),       NULL,  6, config_label_temp_calib);
  humidity_adj = new WiFiManagerParameter(      PERSISTENT_HUMIDITY_CALIB,   config_hint_humidity_calib.c_str(),   NULL,  6, config_label_humidity_calib);
  pressure_adj = new WiFiManagerParameter(      PERSISTENT_PRESSURE_CALIB,   config_hint_pressure_calib.c_str(),   NULL,  8, config_label_pressure_calib);
  battery_adj = new WiFiManagerParameter(       PERSISTENT_BATTERY_CALIB,    config_hint_battery_calib.c_str(),    NULL,  6, config_label_battery_calib);
  custom_sleep_time_ms = new WiFiManagerParameter(PERSISTENT_SLEEP_TIME_MS,  config_hint_sleep_time_ms.c_str(),    NULL,  8, config_label_sleep_time_ms);
  custom_high_water_slot = new WiFiManagerParameter(PERSISTENT_HIGH_WATER_SLOT, config_hint_high_water_slot.c_str(), NULL, 3, config_label_high_water_slot);

  wifi_manager.addParameter(custom_node_name);
  wifi_manager.addParameter(custom_report_host);
  wifi_manager.addParameter(custom_report_port);
  wifi_manager.addParameter(clock_drift_adj);
  wifi_manager.addParameter(temp_adj);
  wifi_manager.addParameter(humidity_adj);
  wifi_manager.addParameter(pressure_adj);
  wifi_manager.addParameter(battery_adj);
  wifi_manager.addParameter(custom_sleep_time_ms);
  wifi_manager.addParameter(custom_high_water_slot);
  wifi_manager.setConfigPortalTimeout(CONFIG_SERVER_MAX_TIME);
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setSaveConfigCallback(&wifi_manager_save_config_callback);

  Serial.println("Starting config server");
  Serial.println("current config SSID: "+WiFi.SSID());
  struct station_config conf;
  wifi_station_get_config_default(&conf);
  Serial.printf("default config SSID: %.*s\n", sizeof(conf.ssid), conf.ssid);
  Serial.print("Connect to AP ");
  Serial.println(nodename);
  if (wifi_manager.startConfigPortal(nodename.c_str())) {
    Serial.println("Config portal result Success");
  } else {
    Serial.println("Config portal result Failed");
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
  bool config_flag = false;

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

          if (response.startsWith("update")) {
            update_flag = true;
            response.replace("update,","");
          }

          if (response.startsWith("config")) {
            config_flag = true;
            response.replace("config,","");
          }
        } else {
          // some error occurred and we got no response...
          client.stop();
        }
      }
    }

    if (config_flag) {
      Serial.println("accepted config update command");
      if (!update_config(client))
        Serial.println("error: config update failed");
    }

    if (update_flag) {
      Serial.println("accepted firmware update command");
#if !DISABLE_FW_UPDATE
      if (!update_firmware(client))
        Serial.println("error: firmware update failed");
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
  String json;

  if (!client.connected())
    return -1;

  if (rtc_mem[RTC_MEM_NUM_READINGS] > 0) {
    flags_time_t timestamp = {0,0,0,0};
    const char typestrings[7][17] = {
      "unknown",
      "temperature",
      "humidity",
      "pressure",
      "particles 1.0µm",
      "particles 2.5µm",
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

        case SENSOR_PARTICLE_1_0:
          type=typestrings[4];
          calibrated_reading = reading->value * 1000.0;
        break;

        case SENSOR_PARTICLE_2_5:
          type=typestrings[5];
          calibrated_reading = reading->value * 1000.0;
        break;

        case SENSOR_BATTERY_VOLTAGE:
          type=typestrings[6];
          calibrated_reading += calibrations[3];
        break;

        case SENSOR_TIMESTAMP_OFFS:
          timestamp.millis = (rtc_mem[RTC_MEM_DATA_TIMEBASE] << RTC_DATA_TIMEBASE_SHIFT) + (uint64_t)reading->value;
        break;

        case SENSOR_UNKNOWN:
        default:
          type=typestrings[0];
        break;
      }

      if (reading->type == SENSOR_TIMESTAMP_OFFS) {
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

    // send the current calibration values in the last packet
    if ((unsigned)num_slots_read == rtc_mem[RTC_MEM_NUM_READINGS]) {
      json += "\"calibrations\":[";
      json +=  "{\"type\":\"" + String(typestrings[1]) + "\",\"value\":" + String(calibrations[0], 3) + "}";
      json += ",{\"type\":\"" + String(typestrings[2]) + "\",\"value\":" + String(calibrations[1], 3) + "}";
      json += ",{\"type\":\"" + String(typestrings[3]) + "\",\"value\":" + String(calibrations[2], 3) + "}";
      json += ",{\"type\":\"" + String(typestrings[6]) + "\",\"value\":" + String(calibrations[3], 3) + "}";
      json += "],";
    }

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

    // transmit the json command
    if (send_command(client, json))
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

// helper to fetch the config update files from the server store them in SPIFFS
static bool update_config(WiFiClient& client)
{
  const char* filenames[] = {
    PERSISTENT_NODE_NAME,
    PERSISTENT_REPORT_HOST_NAME,
    PERSISTENT_REPORT_HOST_PORT,
    PERSISTENT_CLOCK_CALIB,
    PERSISTENT_TEMP_CALIB,
    PERSISTENT_HUMIDITY_CALIB,
    PERSISTENT_PRESSURE_CALIB,
    PERSISTENT_BATTERY_CALIB,
    PERSISTENT_SLEEP_TIME_MS,
    PERSISTENT_HIGH_WATER_SLOT,
  };
  bool status = true;

  if (!client.connected())
    return false;

  for (size_t i=0; i<(sizeof(filenames)/sizeof(*filenames)); i++) {
    String json;
    unsigned len;

    Serial.print("Retrieving config file: ");
    Serial.println(filenames[i]);

    json = json_header();
    json += "\"command\":\"get_config\",";
    json += "\"arg\":\"";
    json += filenames[i];
    json += "\"}";

    // null-terminate
    json += ";";
    json.setCharAt(json.length()-1, '\0');

    if (!send_command(client, json)) {
      Serial.println("warning: update command not fully transmitted");
      status = false;
      continue;
    }

    String filesize = client.readStringUntil('\n');
    if (0 == filesize.length())
      continue; //normal condition -- server will return empty string
                //if config file is not present

    if ((filesize.charAt(0) < '0') || (filesize.charAt(0) > '9')) {
      Serial.println("warning: invalid response received from server");
      status = false;
      continue;
    }

    len = filesize.toInt();
    if (len > 4096) {
      Serial.print("warning: invalid filesize received: ");
      Serial.println(len);
      status = false;
      continue;
    }

    String md5sum = client.readStringUntil('\n');

    uint8_t *buffer = new uint8_t[len];

    Serial.println("Receiving config update...");
    Serial.print("File Size = ");  Serial.println(filesize);
    Serial.print("MD5 = ");  Serial.println(md5sum);

    if (len != client.readBytes(buffer, len)) {
      Serial.print("warning: short read of file "); Serial.println(filenames[i]);
      status = false;
    } else {
      MD5Builder *md5 = new MD5Builder();
      md5->begin();
      md5->add(buffer, len);
      md5->calculate();
      if (!md5->toString().equals(md5sum)) {
        status=false;
        Serial.println("warning: md5sum mismatch");
#if EXTRA_DEBUG
        Serial.print(md5sum); Serial.print(" != "); Serial.println(md5->toString());
#endif
      } else {
#if EXTRA_DEBUG
        Serial.print(md5sum); Serial.print(" == "); Serial.println(md5->toString());
#endif
        // everything is OK, store in SPIFFS
        if (persistent_write(filenames[i], buffer, len)) {

          // special handling to update the RTC mem value for temp calib
          if (4==i) {
            float tempf;
            const char* nptr = (const char*)buffer;
            char* endptr = (char*)nptr;

            //check for valid float
            tempf = strtof(nptr, &endptr);
            if ((0==tempf) && (nptr==endptr)) {
              persistent_write(filenames[i], ""); //erase existing file
              tempf = DEFAULT_TEMP_CALIB;
            }
            rtc_mem[RTC_MEM_TEMP_CAL] = *((uint32_t*)&tempf);
          }

          // special handling to update the RTC mem value for humidity calib
          if (5==i) {
            float tempf;
            const char* nptr = (const char*)buffer;
            char* endptr = (char*)nptr;

            //check for valid float
            tempf = strtof(nptr, &endptr);
            if ((0==tempf) && (nptr==endptr)) {
              persistent_write(filenames[i], ""); //erase existing file
              tempf = DEFAULT_HUMIDITY_CALIB;
            }
            rtc_mem[RTC_MEM_HUMIDITY_CAL] = *((uint32_t*)&tempf);
          }

          // special handling to update the RTC mem value for custom sleep time
          if (8==i) {
            sleep_params_t *sleep_params = (sleep_params_t*) &rtc_mem[RTC_MEM_SLEEP_PARAMS];
            int temp;
            const char* nptr = (const char*)buffer;
            char* endptr = (char*)nptr;

            //check for valid integer
            temp = strtol(nptr, &endptr, 0);
            if ((0==temp) && (nptr==endptr)) {
              persistent_write(filenames[i], ""); //erase existing file
              sleep_params->sleep_time_ms = (int)DEFAULT_SLEEP_TIME_MS;
            } else {
              if (temp < 200)
                temp = 200;
              if (temp > 11200000)
                temp = 11200000;
              sleep_params->sleep_time_ms = temp;
            }
          }

          // special handling to update the RTC mem value for custom high-water slot
          if (9==i) {
            sleep_params_t *sleep_params = (sleep_params_t*) &rtc_mem[RTC_MEM_SLEEP_PARAMS];
            int temp;
            const char* nptr = (const char*)buffer;
            char* endptr = (char*)nptr;

            //check for valid integer
            temp = strtol(nptr, &endptr, 0);
            if ((0==temp) && (nptr==endptr)) {
              persistent_write(filenames[i], ""); //erase existing file
              sleep_params->high_water_slot = DEFAULT_HIGH_WATER_SLOT;
            } else {
              if (temp <= 0)
                temp = 1;
              if (temp > NUM_STORAGE_SLOTS)
                temp = NUM_STORAGE_SLOTS;
              sleep_params->high_water_slot = temp;
            }
          }

          // inform the server that it can delete the update file
          json = json_header();
          json += "\"command\":\"delete_config\",";
          json += "\"arg\":\"";
          json += filenames[i];
          json += "\"}";

          // null-terminate
          json += ";";
          json.setCharAt(json.length()-1, '\0');

          send_command(client, json);
        } else {
          status = false;
        }
      }

      delete md5;
    }
    delete[] buffer;
  }

  return status;
}

#if !DISABLE_FW_UPDATE
// helper to fetch a firmware update from the server and apply it
static bool update_firmware(WiFiClient& client)
{
  String json = json_header();
  unsigned len;
  bool status = false;

  if (!client.connected())
    return false;

  json += "\"command\":\"update\",";
  json += "\"arg\":\"" FIRMWARE_NAME "\"}";

  // null-terminate
  json += ";";
  json.setCharAt(json.length()-1, '\0');

  if (!send_command(client, json))
    Serial.println("warning: update command not fully transmitted");

  String filesize = client.readStringUntil('\n');
  String md5sum = client.readStringUntil('\n');
  len = filesize.toInt();
  if (len == 0)
    Serial.println("Recieved 0 byte len response - check server logs");

  if (len > 0x300000) {
    Serial.print("error: filesize too large: ");
    Serial.println(len);
    status = false;
    len = 0;
  }

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
#endif /* !DISABLE_FW_UPDATE */

static bool send_command(WiFiClient& client, String& command)
{
  unsigned len;

  // flush any pending data in the client buffers
  client.flush();
  while (client.read() >= 0) {}

  // transmit the command and verify the number of bytes written
  len = client.print(command);
  return (len == command.length());
}
