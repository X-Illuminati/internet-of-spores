#ifndef _SENSORS_H_
#define _SENSORS_H_

/* Types and Enums */
// Labels for the different types of sensor readings
typedef enum sensor_type_e {
  SENSOR_UNKNOWN,
  SENSOR_TEMPERATURE,
  SENSOR_HUMIDITY,
  SENSOR_PRESSURE,
  SENSOR_PARTICLE,
  SENSOR_BATTERY_VOLTAGE,
} sensor_type_t;


/* Function Prototypes */
bool read_sht30(void);
void read_hp303b(bool measure_temp);
void read_vcc(void);

#endif /* _SENSORS_H_ */
