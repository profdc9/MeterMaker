/* HIPanel firmware */
/* by Daniel L. Marks */

/*
   Copyright (c) 2023 Daniel Marks

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#define ADC0INP A0
#define ADC1INP A1
#define ADC2INP A2

const int dacpins[8] = { 8, 9, 10, 11, 12, 13, 14, 15 };

#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define TIMER1_INTERVAL_US 1000
#include <Arduino.h>
#include <LittleFS.h>
#include "LiquidCrystalButtons.h"

#include "consoleio.h"
#include "mini-printf.h"
#include "tinycl.h"

#define ANALOG_WRITE_RANGE 16383

LittleFSConfig littleFsConfig;
LiquidCrystalButtons lcd(3, 2, 4, 5, 6, 7);

#if 0
uint8_t writeConfig(void)
{
  File f = LittleFS.open("/config.bin","w");
  if (f)
  {
    f.write((uint8_t *)&hs,sizeof(hs));
    f.close();
    return 1;
  }
  return 0;
}
#endif

void setup() {
   Serial.begin(9600);
   lcd.begin(16, 2);
   lcd.setCursor(0,0);
   lcd.print("MeterMaker!!! by");
   lcd.setCursor(0,1);
   lcd.print("Dan Marks for RLL");
   console_setMainSerial(&Serial);
   for (int i=0;i<(sizeof(dacpins)/sizeof(dacpins[0]));i++)
   {     
      pinMode(dacpins[i], OUTPUT);
      digitalWrite(dacpins[i], LOW);
   }
    
   pinMode(ADC0INP, INPUT);
   pinMode(ADC1INP, INPUT);
   pinMode(ADC2INP, INPUT);
   analogReadResolution(16);
   analogWriteFreq(10000u);
   analogWriteRange(ANALOG_WRITE_RANGE);
 
   littleFsConfig.setAutoFormat(true);
   LittleFS.setConfig(littleFsConfig);
   LittleFS.begin();
}

int set_cmd(int args, tinycl_parameter* tp, void *v)
{
  int chan = tp[0].ti.i;
  int val = tp[1].ti.i;
  if ((chan >= 0) && (chan < (sizeof(dacpins)/sizeof(dacpins[0]))) && (val >= 0) && (val <= ANALOG_WRITE_RANGE))
  {
    char s[16];
    analogWrite(dacpins[chan],val);
    console_print("Set channel  ");
    snprintf(s,sizeof(s), "S %u:%u              ",chan,val);
    console_println(s);
    lcd.setCursor(0,1);
    lcd.print(s);
  } else
  {
    console_println("Invalid values");
  } 
  return 1;
}

int write_cmd(int args, tinycl_parameter* tp, void *v)
{
#if 0
  console_println(writeConfig() ? "FLASH configuration written" : "FLASH writing error");
#endif
  return 1;
}

const tinycl_command tcmds[] =
{
  { "SET", "Set output", set_cmd, TINYCL_PARM_INT, TINYCL_PARM_INT, TINYCL_PARM_END },
  { "WRITE", "Write Config Flash", write_cmd, TINYCL_PARM_END },
  { "HELP", "Display This Help", help_cmd, {TINYCL_PARM_END } }
};


int help_cmd(int args, tinycl_parameter *tp, void *v)
{
  tinycl_print_commands(sizeof(tcmds) / sizeof(tinycl_command), tcmds);
  return 1;
}

void delay_idle(uint16_t dl)
{
  while (dl > 0)
  {
    idle_task();
    delayMicroseconds(1000);    
    --dl;
  }
}

void idle_task(void)
{
  static uint32_t last_poll = 0;  

  uint32_t current_poll = millis();
  if (current_poll != last_poll)
  {
    last_poll = current_poll;
  }
}

void loop() {
  idle_task();
  lcd.pollButtons();
#if 0
  for (uint8_t i=0;i<5;i++)
  {
     if (lcd.getButtonPressed(i))
     {
      char s[20];
      sprintf(s,"Pressed %d          ",i+1);
      lcd.setCursor(0,0);
      lcd.print(s);
      Serial.println(s);
     }
  }
#endif
#if 0
  {
    static uint16_t count = 0;
    count++;
    char s[15];
    sprintf(s,"Count %d  ",count);
    lcd.setCursor(0,1);
    lcd.print(s);
  }
#endif
  if (tinycl_task(sizeof(tcmds) / sizeof(tinycl_command), tcmds, NULL))
  {
    tinycl_do_echo = 1;
    console_print("> ");
  }
}
