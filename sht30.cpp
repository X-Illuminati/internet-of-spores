#include <Arduino.h>
#include <Ethernet.h>
#include <Wire.h>
#include <stdint.h>
#include "sht30.h"

#define SHT30_LOOP_DELAY (2)
int sht30_get(uint8_t addr, sht30_repeatability_t type, sht30_data_t *data_out)
{
  int retval = 0;
  int max_duration = 0;

  Wire.beginTransmission(addr);
  Wire.write(0x24);
  switch(type) {
    case SHT30_RPT_HIGH:
      Wire.write(0x16);
      max_duration=(13.5 + SHT30_LOOP_DELAY) / SHT30_LOOP_DELAY;
    break;

    case SHT30_RPT_MED:
      Wire.write(0x0B);
      max_duration=(5 + SHT30_LOOP_DELAY) / SHT30_LOOP_DELAY;
    break;

    case SHT30_RPT_LOW:
    default:
      Wire.write(0x00);
      max_duration=(3 + SHT30_LOOP_DELAY) / SHT30_LOOP_DELAY;
    break;
  }
  retval = Wire.endTransmission();

  if (0 == retval) {
    int count = 0;
    int i = 0;

    while ((count < 6) && (i<max_duration)) {
      i++;
      delay(SHT30_LOOP_DELAY);
      count += Wire.requestFrom(addr, (uint8_t)(6-count));
    }

    if (count != 6) {
      retval = -1;
    } else {
      for (i=0; i<6; i++) {
        ((char*)data_out)[i]=Wire.read();
      }
    }
  }

  return retval;
}

float sht30_parse_temp_c(sht30_data_t data)
{
  uint16_t temp = ntohs(data.temp);

  return ((temp * 175) / 65535.0) - 45;
}

float sht30_parse_temp_f(sht30_data_t data)
{
  uint16_t temp = ntohs(data.temp);

  return ((temp * 315) / 65535.0) - 49;
}

float sht30_parse_humidity(sht30_data_t data)
{
  uint16_t humidity = ntohs(data.humidity);

  return (humidity * 100) / 65535.0;
}

static uint8_t sht30_crc(uint16_t data) {
  uint8_t crc = 0xff;

  for (uint16_t i = 0x8000; i > 0; i >>= 1) {
    bool bit = crc & 0x80;
    if (data & i) {
      bit = !bit;
    }
    crc <<= 1;
    if (bit) {
      crc ^= 0x31;
    }
  }

  return crc;
}

bool sht30_check_temp(sht30_data_t data)
{
  uint16_t temp = ntohs(data.temp);
  uint8_t check = sht30_crc(temp);

  return (check == data.temp_check);
}

bool sht30_check_humidity(sht30_data_t data)
{
  uint16_t humidity = ntohs(data.humidity);
  uint8_t check = sht30_crc(humidity);

  return (check == data.humidity_check);
}
