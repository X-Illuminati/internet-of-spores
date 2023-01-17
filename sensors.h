#ifndef _SENSORS_H_
#define _SENSORS_H_

#include "project_config.h"


/* Types and Enums */
// Labels for the different types of sensor readings
typedef enum sensor_type_e {
  SENSOR_UNKNOWN,
  SENSOR_TEMPERATURE,
  SENSOR_HUMIDITY,
  SENSOR_PRESSURE,
  SENSOR_PARTICLE_1_0,
  SENSOR_PARTICLE_2_5,
  SENSOR_BATTERY_VOLTAGE,
  SENSOR_TIMESTAMP_OFFS,
} sensor_type_t;


/* Function Prototypes */
void sensors_init(void);

#if TETHERED_MODE
void read_ppd42(unsigned long sampletime_us=30000000);
#endif
bool read_sht30(bool perform_store);
bool read_hp303b(bool measure_temp);
void read_vcc(bool perform_store);
void store_uptime(void);
float get_temp(void);
float get_humidity(void);

#endif /* _SENSORS_H_ */
