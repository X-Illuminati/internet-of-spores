// This software is probably copyright Waveshare, proprietary
// No license text was included, but it is freely available from their website:
// https://www.waveshare.com/w/upload/f/f8/E-Paper-Segment-Code2.zip
#ifndef _EPD_1in9_H_
#define _EPD_1in9_H_

#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>

// address
#define adds_com  	0x3C
#define adds_data	0x3D

extern unsigned char DSPNUM_1in9_on[];
extern unsigned char DSPNUM_1in9_off[];

void EPD_1in9_GPIOInit(void);
void EPD_1in9_Reset(void);
void EPD_1in9_SendCommand(unsigned char Reg);
void EPD_1in9_SendData(unsigned char Data);
unsigned char EPD_1in9_readCommand(unsigned char Reg);
unsigned char EPD_1in9_readData(unsigned char Data);
void EPD_1in9_ReadBusy(void);
void EPD_1in9_lut_DU_WB(void);
void EPD_1in9_lut_GC(void);
void EPD_1in9_lut_5S(void);
void EPD_1in9_Temperature(void);
uint8_t EPD_1in9_init(void);
void EPD_1in9_Write_Screen(unsigned char *image);
void EPD_1in9_Write_Screen1(unsigned char *image);
void EPD_1in9_sleep(void);
void EPD_1in9_Clear_Screen(void);
void EPD_1in9_Set_Temp(unsigned char temp);
void EPD_1in9_Easy_Write_Full_Screen(float temp, bool fahrenheit, float humidity, bool connect, bool battery, bool connection_error);

#endif
