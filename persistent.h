#ifndef _PERSISTENT_H_
#define _PERSISTENT_H_

#include "project_config.h"


/* Function Prototypes */
void persistent_init(void); //optional - initialize early

String persistent_read(const char* filename);
String persistent_read(const char* filename, String default_value);
int persistent_read(const char* filename, int default_value);
float persistent_read(const char* filename, float default_value);

bool persistent_write(const char* filename, String data);
bool persistent_write(const char* filename, const uint8_t *buf, size_t size);

#endif /* _PERSISTENT_H_ */
