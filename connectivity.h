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

#endif /* _CONNECTIVITY_H_ */
