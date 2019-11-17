#ifndef _CONNECTIVITY_H_
#define _CONNECTIVITY_H_

#include "project_config.h"


/* Function Prototypes */
void connectivity_preinit(void);
void connectivity_init(void);
void connectivity_disable(void);

bool connect_wifi(void);
void enter_config_mode(void);

void upload_readings(void);

float get_wifi_power(uint8_t power_setting);

#endif /* _CONNECTIVITY_H_ */
