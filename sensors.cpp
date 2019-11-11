#include "project_config.h"

#include <Arduino.h>
#include <Esp.h>
#include <LOLIN_HP303B.h>
#include <Wire.h>

#include "pulse2.h"
#include "rtc_mem.h"
#include "sensors.h"
#include "sht30.h"


/* Global Data Structures */
LOLIN_HP303B HP303BPressureSensor;
#if TETHERED_MODE
Pulse2 pulse;
#endif

/* Functions */
// setup sensors
void sensors_init(void)
{
  Wire.begin();

#if TETHERED_MODE
  pinMode(PPD42_PIN_DET, INPUT);
  if (digitalRead(PPD42_PIN_DET)) {
    pinMode(PPD42_PIN_1_0, INPUT);
    pinMode(PPD42_PIN_2_5, INPUT);
  }
#endif
}

#if TETHERED_MODE
// read and store values from the PPD42 particle sensor
// this measurement is recommended to take 30 seconds (parameter)
// also the sensor requires a 3 minute warm-up time
// so only available in tethered mode
void read_ppd42(unsigned long sampletime_us)
{
  if (digitalRead(PPD42_PIN_DET)) {
    unsigned long starttime_us = micros();
    unsigned long lpo10 = 0;
    unsigned long lpo25 = 0;
    unsigned long total;
    pulse.register_pin(PPD42_PIN_1_0, LOW);
    pulse.register_pin(PPD42_PIN_2_5, LOW);

    while ((total = micros() - starttime_us) < sampletime_us) {
      unsigned long duration;
      uint8_t pin;
      pin = pulse.watch(&duration, (sampletime_us-total));
      if (PULSE2_NO_PIN != pin) {
        if (PPD42_PIN_1_0 == pin)
          lpo10 += duration;
        if (PPD42_PIN_2_5 == pin)
          lpo25 += duration;
#if EXTRA_DEBUG
        //Serial.printf("%lu: pulse: %lu: %s\n", millis(), duration, (pin==PPD42_PIN_1_0)?"PIN10":"PIN25");
#endif
      }
    }

    {
      float ratio10;
      float ratio25;
      float concentration10;
      float concentration25;
      int32_t count10;
      int32_t count25;

      // percentage of sampletime_us where 1.0/2.5μm pin was pulsed low
      ratio10 = lpo10*(100.0/total);
      ratio25 = lpo25*(100.0/total);
      concentration10 = 1.1*pow(ratio10,3)-3.8*pow(ratio10,2)+520*ratio10;
      concentration25 = 1.1*pow(ratio25,3)-3.8*pow(ratio25,2)+520*ratio25;
      if (concentration10 > 0)
        count10=concentration10/10; // *100 /1000
      else
        count10 = 0;
      if (concentration25 > 0)
        count25=concentration25/10; // *100 /1000
      else
        count25 = 0;
      store_reading(SENSOR_PARTICLE_1_0, count10);
      store_reading(SENSOR_PARTICLE_2_5, count25);
#if EXTRA_DEBUG
      Serial.printf("Raw Particle Count >1.0μm: %d particles/cf\n", count10*1000);
      Serial.printf("Raw Particle Count >2.5μm: %d particles/cf\n", count25*1000);
#endif
    }

    pulse.unregister_pin(PPD42_PIN_1_0);
    pulse.unregister_pin(PPD42_PIN_2_5);
  }
}
#endif

// read and store values from the SHT30 temperature and humidity sensor
bool read_sht30(bool perform_store)
{
  static float temperature=0; // running total for averaging
  static float humidity=0;    // running total for averaging
  static unsigned int num_readings=0; // count for averaging
  sht30_data_t data;
  int ret;
  ret = sht30_get(SHT30_ADDR, SHT30_RPT_HIGH, &data);
  Serial.println();
  if (ret != 0) {
    Serial.print("Error Reading SHT30 ret=");
    Serial.println(ret);
    return false;
  } else {
    temperature += sht30_parse_temp_c(data);
    humidity += sht30_parse_humidity(data);
    num_readings++;
    if (perform_store) {
      store_reading(SENSOR_TEMPERATURE, temperature/num_readings*1000.0 + 0.5);
      store_reading(SENSOR_HUMIDITY, humidity/num_readings*1000.0 + 0.5);
    }

#if (EXTRA_DEBUG != 0)
    Serial.print("Raw Temperature: ");
    Serial.print(sht30_parse_temp_c(data), 3);
    Serial.print("°C (");
    Serial.print(sht30_parse_temp_f(data), 3);
    Serial.println("°F)");
    Serial.print("Raw Humidity: ");
    Serial.println(sht30_parse_humidity(data), 3);
#endif
  }

  return true;
}

// read and store values from the HP303B barametric pressure sensor
bool read_hp303b(bool measure_temp)
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
      return false;
    } else {
      store_reading(SENSOR_TEMPERATURE, temperature*1000);
#if (EXTRA_DEBUG != 0)
      Serial.print("Raw Temperature: ");
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
    return false;
  } else {
    store_reading(SENSOR_PRESSURE, pressure);
#if (EXTRA_DEBUG != 0)
    Serial.print("Raw Pressure: ");
    Serial.print(pressure/1000.0, 3);
    Serial.print(" kPa (");
    Serial.print((float)pressure/3386.39);
    Serial.println(" in Hg)");
#endif
  }

  return true;
}

/* Set ADC mode to read VCC voltage level (battery) */
ADC_MODE(ADC_VCC)

// read and store the ESP VCC voltage level (battery)
void read_vcc(bool perform_store)
{
  static uint32_t readings=0; // running total for averaging
  static unsigned int num_readings=0; // count for averaging
  uint16_t val;
  val = ESP.getVcc();

  if (val > 37000) {
    Serial.println("Error reading VCC value");
  } else {
    readings += val;
    num_readings++;
    if (perform_store)
      store_reading(SENSOR_BATTERY_VOLTAGE, readings/num_readings);
  }

#if (EXTRA_DEBUG != 0)
  Serial.print("Battery voltage: ");
  Serial.print(val/1000.0, 3);
  Serial.println("V");
#endif
}

// store the current uptime as an offset from RTC_MEM_DATA_TIMEBASE
void store_uptime(void)
{
  uint64_t timestamp;
  uint64_t time_h;
  uint32_t time_l;

  timestamp = uptime();
  time_h = rtc_mem[RTC_MEM_DATA_TIMEBASE] << RTC_DATA_TIMEBASE_SHIFT;
  time_l = timestamp - time_h;

#if EXTRA_DEBUG
  Serial.printf("[%llu] Storing Timestamp Offset %u (@%llu)\n", timestamp, time_l, time_h);
#endif
  store_reading(SENSOR_TIMESTAMP_OFFS, (int32_t)time_l);
}
