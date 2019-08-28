#ifndef _SHT30_H_
#define _SHT30_H_
#include <stdint.h>

typedef enum sht30_repeatability_e {
  SHT30_RPT_LOW,
  SHT30_RPT_MED,
  SHT30_RPT_HIGH
} sht30_repeatability_t;

typedef struct __packed sht30_data_s {
	uint16_t temp;
	uint8_t  temp_check;
	uint16_t humidity;
	uint8_t  humidity_check;
} sht30_data_t;

int sht30_get(uint8_t addr, sht30_repeatability_t type, sht30_data_t *data_out);

float sht30_parse_temp_c(sht30_data_t data);
float sht30_parse_temp_f(sht30_data_t data);
float sht30_parse_humidity(sht30_data_t data);

bool sht30_check_temp(sht30_data_t data);
bool sht30_check_humidity(sht30_data_t data);

#endif /* _SHT30_H_ */
