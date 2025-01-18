#include "LiquidCrystalButtons.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1 
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that it's in that state when a sketch starts (and the
// LiquidCrystalButtons constructor is called).

#define LC_OUTPUT_INVERTED

LiquidCrystalButtons::LiquidCrystalButtons(uint8_t rs,  uint8_t enable,
			     uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
  _rs_pin = rs;
  _enable_pin = enable;
  
  _data_pins[0] = d4;
  _data_pins[1] = d5;
  _data_pins[2] = d6;
  _data_pins[3] = d7; 
  _data_pins[4] = rs;

  _displayfunction = LCDB_1LINE;
}

#define SETDATALINEOUTPUTS() do { setDataLineOutput(OUTPUT); delayMicroseconds(10); } while (0)
#define SETDATALINEINPUTS() do { setDataLineOutput(INPUT_PULLUP); delayMicroseconds(10); } while (0)

void LiquidCrystalButtons::clearButtons()
{
  for (uint8_t i=0; i<5; i++)
    _button_pressed[i] = false;
}

uint8_t repeat_states_init_ctr = 100;
uint8_t repeat_states_next_ctr = 30;

void LiquidCrystalButtons::pollButtons()
{
  //SETDATALINEINPUTS();
  uint16_t m = millis();
  if (((uint16_t)(m -_last_millis)) < 5) return;
  _last_millis = m; 
  
  for (uint8_t i=0; i<5; i++)
  {
     uint8_t state = digitalRead(_data_pins[i]);
     if (state == _last_state[i])
     {
        _state_count[i] = 0;
        if (!state)
        {
          if ((--_repeated_state[i]) == 0)
          {
            _repeated_state[i] = repeat_states_next_ctr;
            _button_pressed[i] = true;
          }
        }
     } else 
     {  if (_state_count[i] > 10)
        {
           _state_count[i] = 0;
           _repeated_state[i] = repeat_states_init_ctr;
           _last_state[i] = state;
           if ((!state) && (!_button_pressed[i]))
              _button_pressed[i] = true;
        } else _state_count[i]++;
     }
  }
}

bool LiquidCrystalButtons::getButtonPressed(uint8_t b)
{
  bool state = _button_pressed[b];
  if (state) _button_pressed[b] = false;
  return state;
}

bool LiquidCrystalButtons::waitButtonPressed(uint8_t b)
{
  bool state = getButtonPressed(b);
  if (!state)
  {
    while (!(_last_state[b]))
      pollButtons();
  }
  return state;
}

uint8_t LiquidCrystalButtons::readUnBounced(uint8_t b)
{
  return (!digitalRead(_data_pins[b]));
}

uint8_t LiquidCrystalButtons::readButton(uint8_t b)
{
  return !(_last_state[b]);
}

void LiquidCrystalButtons::setDataLineOutput(uint8_t val)
{
  for (uint8_t i=0; i<5; i++)
    pinMode(_data_pins[i], val);
}

const uint8_t defaultCharTable[] PROGMEM = 
{
  0b00010000,  // character 0
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,

  0b00011000,  // character 1
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,
  0b00011000,

  0b00011100,  // character 2
  0b00011100,
  0b00011100,
  0b00011100,
  0b00011100,
  0b00011100,
  0b00011100,
  0b00011100,

  0b00011110,  // character 3
  0b00011110,
  0b00011110,
  0b00011110,
  0b00011110,
  0b00011110,
  0b00011110,
  0b00011110,

  0b00000000,  // character 4
  0b00000000,
  0b00000000,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,

  0b00000000,  // character 5
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,

  0b00000000,  // character 6
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00011111,

  0b00000000,  // character 7
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000
};

void LiquidCrystalButtons::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCDB_2LINE;
  }
  _numlines = lines;

  setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);  

  pinMode(_rs_pin, OUTPUT);
  pinMode(_enable_pin, OUTPUT);
  
  SETDATALINEOUTPUTS();

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40 ms after power rises above 2.7 V
  // before sending commands. Arduino can turn on way before 4.5 V so we'll wait 50
  delayMicroseconds(50000); 
  // Now we pull both RS and R/W low to begin commands
  #ifdef LC_OUTPUT_INVERTED
  digitalWrite(_rs_pin, HIGH);
  digitalWrite(_enable_pin, HIGH);
  #else
  digitalWrite(_rs_pin, LOW);
  digitalWrite(_enable_pin, LOW);
  #endif
    // this is according to the Hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms

    // second try
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms
    
    // third go!
    write4bits(0x03); 
    delayMicroseconds(150);

    // finally, set to 4-bit interface
    write4bits(0x02); 

  // finally, set # lines, font size, etc.
  command(LCDB_FUNCTIONSET | _displayfunction);  

  for (uint8_t i=0;i<8;i++)
    createChar(i, (uint8_t *)&defaultCharTable[i << 3]); 

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCDB_DISPLAYON | LCDB_CURSOROFF | LCDB_BLINKOFF;  
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCDB_ENTRYLEFT | LCDB_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCDB_ENTRYMODESET | _displaymode);

}

void LiquidCrystalButtons::setRowOffsets(int row0, int row1, int row2, int row3)
{
  _row_offsets[0] = row0;
  _row_offsets[1] = row1;
  _row_offsets[2] = row2;
  _row_offsets[3] = row3;
}

/********** high level commands, for the user! */
void LiquidCrystalButtons::clear()
{
  command(LCDB_CLEARDISPLAY);  // clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LiquidCrystalButtons::home()
{
  command(LCDB_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LiquidCrystalButtons::setCursor(uint8_t col, uint8_t row)
{
  const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
  if ( row >= max_lines ) {
    row = max_lines - 1;    // we count rows starting w/ 0
  }
  if ( row >= _numlines ) {
    row = _numlines - 1;    // we count rows starting w/ 0
  }
  
  command(LCDB_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystalButtons::noDisplay() {
  _displaycontrol &= ~LCDB_DISPLAYON;
  command(LCDB_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalButtons::display() {
  _displaycontrol |= LCDB_DISPLAYON;
  command(LCDB_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystalButtons::noCursor() {
  _displaycontrol &= ~LCDB_CURSORON;
  command(LCDB_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalButtons::cursor() {
  _displaycontrol |= LCDB_CURSORON;
  command(LCDB_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystalButtons::noBlink() {
  _displaycontrol &= ~LCDB_BLINKON;
  command(LCDB_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalButtons::blink() {
  _displaycontrol |= LCDB_BLINKON;
  command(LCDB_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystalButtons::scrollDisplayLeft(void) {
  command(LCDB_CURSORSHIFT | LCDB_DISPLAYMOVE | LCDB_MOVELEFT);
}
void LiquidCrystalButtons::scrollDisplayRight(void) {
  command(LCDB_CURSORSHIFT | LCDB_DISPLAYMOVE | LCDB_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystalButtons::leftToRight(void) {
  _displaymode |= LCDB_ENTRYLEFT;
  command(LCDB_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystalButtons::rightToLeft(void) {
  _displaymode &= ~LCDB_ENTRYLEFT;
  command(LCDB_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystalButtons::autoscroll(void) {
  _displaymode |= LCDB_ENTRYSHIFTINCREMENT;
  command(LCDB_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystalButtons::noAutoscroll(void) {
  _displaymode &= ~LCDB_ENTRYSHIFTINCREMENT;
  command(LCDB_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystalButtons::createChar(uint8_t location, const uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCDB_SETCGRAMADDR | (location << 3));
  for (uint8_t i=0; i<8; i++) {
     write(pgm_read_byte_near(&charmap[i]));
  }
}

/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystalButtons::command(uint8_t value) {
  SETDATALINEOUTPUTS();
  send(value, LOW);
  SETDATALINEINPUTS();
}

inline size_t LiquidCrystalButtons::write(uint8_t value) {
  SETDATALINEOUTPUTS();
  send(value, HIGH);
  SETDATALINEINPUTS();
  return 1; // assume success
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LiquidCrystalButtons::send(uint8_t value, uint8_t mode) {
#ifdef LC_OUTPUT_INVERTED
  digitalWrite(_rs_pin, mode == 0);
#else
  digitalWrite(_rs_pin, mode);
#endif

  write4bits(value>>4);
  write4bits(value);
}

void LiquidCrystalButtons::pulseEnable(void) {
#ifdef LC_OUTPUT_INVERTED
  digitalWrite(_enable_pin, HIGH);
  delayMicroseconds(1);    
  digitalWrite(_enable_pin, LOW);
  delayMicroseconds(1);    // enable pulse must be >450 ns
  digitalWrite(_enable_pin, HIGH);
  delayMicroseconds(100);   // commands need >37 us to settle
#else
  digitalWrite(_enable_pin, LOW);
  delayMicroseconds(1);    
  digitalWrite(_enable_pin, HIGH);
  delayMicroseconds(1);    // enable pulse must be >450 ns
  digitalWrite(_enable_pin, LOW);
  delayMicroseconds(100);   // commands need >37 us to settle
#endif
}

void LiquidCrystalButtons::write4bits(uint8_t value) {
#ifdef LC_OUTPUT_INVERTED
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(_data_pins[i], ((value >> i) & 0x01) == 0);
  }
#else
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(_data_pins[i], (value >> i) & 0x01);
  }
#endif

  pulseEnable();
/*
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(_data_pins[i], 0);
  } */
}
