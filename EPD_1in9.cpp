// This software is probably copyright Waveshare, proprietary
// No license text was included, but it is freely available from their website:
// https://www.waveshare.com/w/upload/f/f8/E-Paper-Segment-Code2.zip
#include <Wire.h>
#include <stdlib.h>
#include "EPD_1in9.h"
#include "project_config.h"

//////////////////////////////////////full screen update LUT////////////////////////////////////////////

unsigned char DSPNUM_1in9_on[15]   = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,       };  // all black
unsigned char DSPNUM_1in9_off[15]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       };  // all white

unsigned char VAR_Temperature=20; 


/******************************************************************************
function :	GPIO Init
parameter:
******************************************************************************/
void EPD_1in9_GPIOInit(void)
{
	pinMode(EPD_BUSY_PIN, INPUT);
	pinMode(EPD_RST_PIN, OUTPUT);
}


/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
void EPD_1in9_Reset(void)
{
    //digitalWrite(EPD_RST_PIN, 1);
    //delay(50);
    digitalWrite(EPD_RST_PIN, 0);
    delay(20);
    digitalWrite(EPD_RST_PIN, 1);
    delay(50);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
void EPD_1in9_SendCommand(unsigned char Reg)
{
	Wire.beginTransmission(adds_com);
	Wire.write(Reg);
	Wire.endTransmission(false);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
void EPD_1in9_SendData(unsigned char Data)
{
    Wire.beginTransmission(adds_data);
	Wire.write(Data);
	Wire.endTransmission();
}

/******************************************************************************
function :	read command
parameter:
     Reg : Command register
******************************************************************************/
unsigned char EPD_1in9_readCommand(unsigned char Reg)
{
	unsigned char a;
	Wire.beginTransmission(adds_com);
	delay(10);
	Wire.write(Reg);
	a = Wire.read();
	Wire.endTransmission();
	return a;
}

/******************************************************************************
function :	read data
parameter:
    Data : Write data
******************************************************************************/
unsigned char EPD_1in9_readData(unsigned char Data)
{
	unsigned char a;
    Wire.beginTransmission(adds_data);
	delay(10);
	Wire.write(Data);
	a = Wire.read();
	Wire.endTransmission();
	return a;
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_1in9_ReadBusy(void)
{
    //Serial.println("e-Paper busy");
	delay(10);
	while(1)
	{	 //=1 BUSY;
		if(digitalRead(EPD_BUSY_PIN)==1) 
			break;
		delay(1);
	}
	//delay(10);
    //Serial.println("e-Paper busy release");
}

/*
# DU waveform white extinction diagram + black out diagram
# Bureau of brush waveform
*/
void EPD_1in9_lut_DU_WB(void)
{
	Wire.beginTransmission(adds_com);
	Wire.write(0x82);
	Wire.write(0x80);
	Wire.write(0x00);
	Wire.write(0xC0);
	Wire.write(0x80);
	Wire.write(0x80);
	Wire.write(0x62);
	Wire.endTransmission();
}

/*   
# GC waveform
# The brush waveform
*/
void EPD_1in9_lut_GC(void)
{
	Wire.beginTransmission(adds_com);
	Wire.write(0x82);
	Wire.write(0x20);
	Wire.write(0x00);
	Wire.write(0xA0);
	Wire.write(0x80);
	Wire.write(0x40);
	Wire.write(0x63);
	Wire.endTransmission();
}

/* 
# 5 waveform  better ghosting
# Boot waveform
*/
void EPD_1in9_lut_5S(void)
{
	Wire.beginTransmission(adds_com);
	Wire.write(0x82);
	Wire.write(0x28);
	Wire.write(0x20);
	Wire.write(0xA8);
	Wire.write(0xA0);
	Wire.write(0x50);
	Wire.write(0x65);
	Wire.endTransmission();	
}

/*
# temperature measurement
# You are advised to periodically measure the temperature and modify the driver parameters
# If an external temperature sensor is available, use an external temperature sensor
*/
void EPD_1in9_Temperature(void)
{
	Wire.beginTransmission(adds_com);
	if( VAR_Temperature < 10 )
	{
		Wire.write(0x7E);
		Wire.write(0x81);
		Wire.write(0xB4);
	}
	else
	{
		Wire.write(0x7E);
		Wire.write(0x81);
		Wire.write(0xB4);
	}
	Wire.endTransmission();

    delay(10);        

	Wire.beginTransmission(adds_com);
	Wire.write(0xe7);    // Set default frame time
        
	// Set default frame time
	if(VAR_Temperature<5)
		Wire.write(0x31); // 0x31  (49+1)*20ms=1000ms
	else if(VAR_Temperature<10)
		Wire.write(0x22); // 0x22  (34+1)*20ms=700ms
	else if(VAR_Temperature<15)
		Wire.write(0x18); // 0x18  (24+1)*20ms=500ms
	else if(VAR_Temperature<20)
		Wire.write(0x13); // 0x13  (19+1)*20ms=400ms
	else
		Wire.write(0x0e); // 0x0e  (14+1)*20ms=300ms
	Wire.endTransmission();
}

/*
# Note that the size and frame rate of V0 need to be set during initialization, 
# otherwise the local brush will not be displayed
*/
uint8_t EPD_1in9_init(void)
{
	//unsigned char i = 0;
  uint8_t res;
	EPD_1in9_Reset();
	//delay(100);

	Wire.beginTransmission(adds_com);
	Wire.write(0x2B); // POWER_ON
	res = Wire.endTransmission();
  Serial.print("1in9 disp i2c status: ");
  Serial.println(res);
  if (0 != res)
    return res;

	delay(10);

	Wire.beginTransmission(adds_com);
	Wire.write(0xA7); // boost
	Wire.write(0xE0); // TSON 
	res = Wire.endTransmission();

	delay(10);

	EPD_1in9_Temperature();
  return res;
}

void EPD_1in9_Write_Screen( unsigned char *image)
{
	Wire.beginTransmission(adds_com);
	Wire.write(0xAC); // Close the sleep
	Wire.write(0x2B); // turn on the power
	Wire.write(0x40); // Write RAM address
	Wire.write(0xA9); // Turn on the first SRAM
	Wire.write(0xA8); // Shut down the first SRAM
	Wire.endTransmission();

	Wire.beginTransmission(adds_data);
	for(char j = 0 ; j<15 ; j++ )
		Wire.write(image[j]);

	Wire.write(0x00);
	Wire.endTransmission();


	Wire.beginTransmission(adds_com);
	Wire.write(0xAB); // Turn on the second SRAM
	Wire.write(0xAA); // Shut down the second SRAM
	Wire.write(0xAF); // display on
	Wire.endTransmission();

	EPD_1in9_ReadBusy();
	//delay(2000);
	
	Wire.beginTransmission(adds_com);
	Wire.write(0xAE); // display off
	Wire.write(0x28); // HV OFF
	Wire.write(0xAD); // sleep in	
	Wire.endTransmission();
}

void EPD_1in9_Write_Screen1( unsigned char *image)
{
	Wire.beginTransmission(adds_com);
	Wire.write(0xAC); // Close the sleep
	Wire.write(0x2B); // turn on the power
	Wire.write(0x40); // Write RAM address
	Wire.write(0xA9); // Turn on the first SRAM
	Wire.write(0xA8); // Shut down the first SRAM
	Wire.endTransmission();

	Wire.beginTransmission(adds_data);
	for(char j = 0 ; j<15 ; j++ )
		Wire.write(image[j]);

	Wire.write(0x03);
	Wire.endTransmission();

	Wire.beginTransmission(adds_com);
	Wire.write(0xAB); // Turn on the second SRAM
	Wire.write(0xAA); // Shut down the second SRAM
	Wire.write(0xAF); // display on
	Wire.endTransmission();

	EPD_1in9_ReadBusy();
	//delay(2000);

	Wire.beginTransmission(adds_com);
	Wire.write(0xAE); // display off
	Wire.write(0x28); // HV OFF
	Wire.write(0xAD); // sleep in	
	Wire.endTransmission();

}

void EPD_1in9_sleep(void)
{
	Wire.beginTransmission(adds_com);
	Wire.write(0x28); // POWER_OFF
	EPD_1in9_ReadBusy();
	Wire.write(0xAD); // DEEP_SLEEP
	Wire.endTransmission();

	delay(50);
	digitalWrite(EPD_RST_PIN, 0);
}

void EPD_1in9_Clear_Screen(void)
{
  EPD_1in9_lut_5S();
	EPD_1in9_Write_Screen(DSPNUM_1in9_off);
	delay(50);
	
	EPD_1in9_lut_GC();
	
	EPD_1in9_Write_Screen1(DSPNUM_1in9_on);
	delay(50);
	
	EPD_1in9_Write_Screen(DSPNUM_1in9_off);
	delay(50);
	
	EPD_1in9_lut_DU_WB();
}

void EPD_1in9_Set_Temp(unsigned char temp)
{
  VAR_Temperature = temp;
}

void EPD_1in9_Easy_Write_Full_Screen(float temp, bool fahrenheit, float humidity, bool connect, bool battery)
{
  unsigned char ram_buffer[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0x4,0};
  unsigned char i;
  const unsigned char symbols[10][2] = {
    {0xbf, 0x1f}, //0
    {0x00, 0x1f}, //1
    {0xfd, 0x17}, //2
    {0xf5, 0x1f}, //3
    {0x47, 0x1f}, //4
    {0xf7, 0x1d}, //5
    {0xff, 0x1d}, //6
    {0x21, 0x1f}, //7
    {0xff, 0x1f}, //8
    {0xf7, 0x1f}, //9
  };

  // correct bogus inputs
  if (temp >= 200.0f)
    temp = 199.9f;

  if (temp <= -100.0f)
    temp = -99.9f;

  if (humidity >= 100.0f)
    humidity = 99.9;
  
  if (humidity < 0.0f)
    humidity = 0;

  // set temperature hundreds place
  if (temp >= 100.0f)
    ram_buffer[0] = symbols[0][1];

  // or set temperature hundreds place to a single segment if below 0
  if (temp < 0.0f)
    ram_buffer[0] = 0x04;

  // set the 10s digit for temperature
  i = int(abs(temp / 10)) % 10;
  ram_buffer[1] = symbols[i][0];
  ram_buffer[2] = symbols[i][1];

  // set the 1s digit for temperature
  i = int(abs(temp)) % 10;
  ram_buffer[3] = symbols[i][0];
  ram_buffer[4] = symbols[i][1];

  // set the tenths digit for temperature
  i = int(abs(temp * 10)) % 10;
  ram_buffer[11] = symbols[i][0];
  ram_buffer[12] = symbols[i][1];

  // set the 10s digit for humidity
  i = int(abs(humidity / 10)) % 10;
  ram_buffer[5] = symbols[i][0];
  ram_buffer[6] = symbols[i][1];

  // set the 1s digit for humidity
  i = int(abs(humidity)) % 10;
  ram_buffer[7] = symbols[i][0];
  ram_buffer[8] = symbols[i][1];

  // set the tenths digit for humidity
  i = int(abs(humidity * 10)) % 10;
  ram_buffer[9] = symbols[i][0];
  ram_buffer[10] = symbols[i][1];

  // add decimal points and % symbol
  ram_buffer[4] |= 0x20;
  ram_buffer[8] |= 0x20;
  ram_buffer[10] |= 0x20;

  // add other symbols
  if (fahrenheit)
    ram_buffer[13] |= 0x02;
  else
    ram_buffer[13] |= 0x01;

  if (connect)
    ram_buffer[13] |= 0x08;
  
  if (battery)
    ram_buffer[13] |= 0x10;

  // ship it
  EPD_1in9_Write_Screen(ram_buffer);
  delay(50);
}
