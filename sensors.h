#ifndef _SENSORS_H_
#define _SENSORS_H_

/* Global Configurations */
#define SENSOR_TIMESTAMP_SHIFT (20)
#define SENSOR_TIMESTAMP_MASK  ((1<<SENSOR_TIMESTAMP_SHIFT)-1)


/* Types and Enums */
// Labels for the different types of sensor readings
typedef enum sensor_type_e {
  SENSOR_UNKNOWN         = 0,
  SENSOR_TEMPERATURE     = 1,
  SENSOR_HUMIDITY        = 2,
  SENSOR_PRESSURE        = 3,
  SENSOR_PARTICLE        = 4,
  SENSOR_BATTERY_VOLTAGE = 5,
  SENSOR_TIMESTAMP_L     = 6,
  SENSOR_TIMESTAMP_H     = 7,
} sensor_type_t;


/* Function Prototypes */
void sensors_init(void);

bool read_sht30(bool perform_store);
bool read_hp303b(bool measure_temp);
void read_vcc(bool perform_store);
void store_uptime(void);

#endif /* _SENSORS_H_ */
