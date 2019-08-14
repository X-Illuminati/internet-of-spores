#include <ESP8266WiFi.h>
#include <LOLIN_HP303B.h>
#include <IOTAppStory.h>                // IotAppStory.com library


// HP303B Opject
LOLIN_HP303B HP303BPressureSensor;

#define COMPDATE __DATE__ __TIME__
#define MODEBUTTON 0                    // Button pin on the esp for selecting modes. D3 for the Wemos!
IOTAppStory IAS(COMPDATE, MODEBUTTON);  // Initialize IotAppStory

//#define INCLUDE_TEMP

void preinit()
{
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}

void setup()
{
  IAS.begin('P');

  //Call begin to initialize HP303BPressureSensor
  //The parameter 0x76 is the bus address. The default address is 0x77 and does not need to be given.
  //HP303BPressureSensor.begin(Wire, 0x76);
  //Use the commented line below instead of the one above to use the default I2C address.
  //if you are using the Pressure 3 click Board, you need 0x76
  HP303BPressureSensor.begin();

  Serial.println(millis());
}



void loop()
{
  int32_t pressure;
  int16_t oversampling = 4;
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

  int16_t ret;

#ifdef INCLUDE_TEMP
  int32_t temperature;

  //lets the HP303B perform a Single temperature measurement with the last (or standard) configuration
  //The result will be written to the paramerter temperature
  //ret = HP303BPressureSensor.measureTempOnce(temperature);
  //the commented line below does exactly the same as the one above, but you can also config the precision
  //oversampling can be a value from 0 to 7
  //the HP303B will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
  //measurements with higher precision take more time, consult datasheet for more information
  ret = HP303BPressureSensor.measureTempOnce(temperature, oversampling);

  if (ret != 0)
  {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C (");
    Serial.print(((float)temperature * 9/5)+32.0);
    Serial.println(" °F)");
  }
#endif

  //Pressure measurement behaves like temperature measurement
  //ret = HP303BPressureSensor.measurePressureOnce(pressure);
  ret = HP303BPressureSensor.measurePressureOnce(pressure, oversampling);
  if (ret != 0)
  {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.print(" Pa (");
    Serial.print((float)pressure/3386.39);
    Serial.println(" in Hg)");
  }

  //Wait some time
  ESP.deepSleep(15000000, RF_DISABLED);
}
