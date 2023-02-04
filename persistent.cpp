#include "project_config.h"

#include <Arduino.h>
#include <FS.h>

#include "persistent.h"


/* Function Prototypes */
static bool spiffs_init(void);

/* Functions */
// initializes spiffs as a singleton and returns the result
static bool spiffs_init(void)
{
  static bool initialized = false;
  static bool success = false;

  if (!initialized) {
    success = SPIFFS.begin();
    if (!success)
      Serial.println("SPIFFS initialization failed");
#if (EXTRA_DEBUG != 0)
    else
      Serial.println("SPIFFS initialized successfully");
#endif
    initialized = true;
  }

  return success;
}

// initialize SPIFFS from setup (optional)
void persistent_init(void)
{
  spiffs_init();
}

// read the value of a persistent file or return String("")
String persistent_read(const char* filename)
{
  String retval = "";

  if (spiffs_init()) {
    String path = "/"; path += filename;
    File persfile = SPIFFS.open(path, "r");

    if (persfile) {
      retval = persfile.readString();
      persfile.close();
#if (EXTRA_DEBUG != 0)
      Serial.println(path + " contents: " + retval);
#endif
    } else {
      Serial.println("Could not open " + path);
    }
  }

  return retval;
}

// read the value of a persistent file or return default_value
// return default_value if persistent file contains "", " ", or "default"
String persistent_read(const char* filename, String default_value)
{
  String persist_value = persistent_read(filename);
  if (persist_value == "" || persist_value == " " || persist_value == "default")
    return default_value;
  else
    return persist_value;
}

// read an integer from a persistent file or return default_value
// return default_value if persistent file contains "", " ", or if there is no parseable integer
int persistent_read(const char* filename, int default_value)
{
  String persist_value = persistent_read(filename);
  const char* nptr = persist_value.c_str();
  char* endptr = (char*)nptr;
  int retval;

  if (persist_value == "" || persist_value == " ")
    return default_value;
  else
    retval = strtol(nptr, &endptr, 0);

  if ((0==retval) && (nptr==endptr))
    return default_value;
  else
    return retval;
}

// read a float from a persistent file or return default_value
// return default_value if persistent file contains "", " ", or if there is no parseable float
float persistent_read(const char* filename, float default_value)
{
  String persist_value = persistent_read(filename);
  const char* nptr = persist_value.c_str();
  char* endptr = (char*)nptr;
  float retval;

  if (persist_value == "" || persist_value == " ")
    return default_value;
  else
    retval = strtof(nptr, &endptr);

  if ((0==retval) && (nptr==endptr))
    return default_value;
  else
    return retval;
}

// write a data string to a persistent file
bool persistent_write(const char* filename, String data)
{
  bool retval = false;

  if (spiffs_init()) {
    String path = "/"; path += filename;
    File persfile = SPIFFS.open(path, "w");

    if (persfile) {
#if (EXTRA_DEBUG != 0)
      Serial.println(path + " <- " + data);
#endif
      size_t result = persfile.print(data);
      persfile.close();
      if (result != data.length())
        Serial.printf("Short write %s (%d)\n", path.c_str(), result);
      else
        retval = true;
    } else {
      Serial.print("Could not open "); Serial.println(path);
    }
  }

  return retval;
}

// write a data buffer to a persistent file
bool persistent_write(const char* filename, const uint8_t *buf, size_t size)
{
  bool retval = false;

  if (spiffs_init()) {
    String path = "/"; path += filename;
    File persfile = SPIFFS.open(path, "w");

    if (persfile) {
#if (EXTRA_DEBUG != 0)
      Serial.printf("%s <- %.*s\n", path.c_str(), size, (const char*)buf);
#endif
      size_t result = persfile.write(buf, size);
      persfile.close();
      if (result != size)
        Serial.printf("Short write %s (%d)\n", path.c_str(), result);
      else
        retval = true;
    } else {
      Serial.print("Could not open "); Serial.println(path);
    }
  }

  return retval;
}
