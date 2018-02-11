#ifndef POWERSUPPLYHARDWARE_H
#define POWERSUPPLYHARDWARE_H

#ifndef ARDUINO_AVR_PRO
  #error Incorrect board type -- Required hardware: Arduino Pro Mini
#endif

#include "Arduino.h"
#include "DigitalPotentiometer.h"
#include <EEPROM.h>
#include <Encoder.h>
#include "Shift7Segment.h"

// TODO: Add system check function (or on startup)
//       - Check current sensor reading exists non-0

class PowerSupplyHardware {
public:
  /////// Knob/Digipot Conversion Constants ///////
  static const int ENC_PULSES_PER_TAP = 4;
  static const int POT_TAPS = 255;
  static const int POT_RESISTANCE = 5000;
  /////////////////////////////////////////////////

  /////////////// Pin Assignment //////////////////
  static const int VOLTAGE_MEASUREMENT_PIN = A0;
  static const int CURRENT_MEASUREMENT_PIN = A1;

  static const int GREEN_LED_PIN = A2;
  static const int RED_LED_PIN = A3;
  static const int BLUE_LED_PIN = 1;

  static const int ENCODER_A_PIN = 2;
  static const int ENCODER_B_PIN = 3;
  // Note: pins 2,3 are interrupt pins on the Arduino Pro Mini

  static const int DIGIT1_DATA_PIN = 4;
  static const int DIGIT1_LATCH_PIN = 5;
  static const int DIGIT1_CLOCK_PIN = 6;

  static const int DIGIT2_DATA_PIN = 7;
  static const int DIGIT2_LATCH_PIN = 8;
  static const int DIGIT2_CLOCK_PIN = 9;

  static const int DIGIPOT_CS_PIN = 10;
  // NOTE: Pins 11, 13 are reserved for SPI communication, used by voltage_adjuster
  static const int BUTTON_PIN = 12;
  // Note: Pin 12 must be configured as an input due to conflicts with SPI
  /////////////////////////////////////////////////

  static PowerSupplyHardware &GetInstance();

  bool button_state();

  void set_knob_color(bool r, bool g, bool b);

  long get_constrained_knob_pos();

  float get_voltage();
  void set_voltage_raw(int taps);

  float get_current();

  void display_two_digits(float number);

  enum DIGIT { DIGIT10s, DIGIT1s };
  void disp_digit(DIGIT pos, int number, bool period);

  void load_state(int eeprom_address);
  void save_state(int eeprom_address);

private:
  Encoder control_knob;
  Shift7Segment voltage_output_1s;
  Shift7Segment voltage_output_10s;
  MCP41HVX1 voltage_adjuster; // Uses pins 11 (MOSI) and 13 (CLOCK) too

  PowerSupplyHardware();

  static const int DIGIT_1_CODES[10];
  static const int DIGIT_10_CODES[10];
};
#endif
