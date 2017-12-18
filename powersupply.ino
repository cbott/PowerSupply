#include "DigitalPotentiometer.h"
#include <EEPROM.h>
#include <Encoder.h>
#include "Shift7Segment.h"

//#define DEBUG

// Inspiration: http://www.instructables.com/id/Easy-variable-voltage-power-supply/

// Parameters for saving state when shut down ///
const int EEPROM_SAVE_ADDRESS = 2;
const unsigned long save_frequency = 10000; // milliseconds
unsigned long last_save_time;
/////////////////////////////////////////////////

/////// Knob/Digipot Conversion Constants ///////
const int ENC_PULSES_PER_TAP = 4;
const int POT_TAPS = 255;
const int POT_RESISTANCE = 5000;
/////////////////////////////////////////////////

/////////////// Pin Assignment //////////////////
const int VOLTAGE_MEASUREMENT_PIN = A0;
const int CURRENT_MEASUREMENT_PIN = A1;

const int RED_LED_PIN = A3;
const int GREEN_LED_PIN = A2;
const int BLUE_LED_PIN = 1;

Encoder control_knob(2, 3); // Interrupt pins on the pro mini
Shift7Segment voltage_output_1s(4, 6, 5);
Shift7Segment voltage_output_10s(7, 9, 8);
MCP41HVX1 voltage_adjuster(10, POT_RESISTANCE, POT_TAPS); // Uses pins 11 (MOSI) and 13 (CLOCK) too

const int BUTTON_PIN = 12; // Pin 12 must be configured as an input due to conflicts with SPI
/////////////////////////////////////////////////

///////////// Power Supply Modes ////////////////
enum class Mode { VOLTAGE, CURRENT };
Mode current_mode = Mode::VOLTAGE;
/////////////////////////////////////////////////


void setup(){
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("System Init");
#endif
  // recall last saved control value
  long saved_knob_pos;
  EEPROM.get(EEPROM_SAVE_ADDRESS, saved_knob_pos);
  control_knob.write(saved_knob_pos);
  last_save_time = millis();

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT);

}


bool prev_button_state = false;
void loop(){
  bool current_button_state = digitalRead(BUTTON_PIN);
  if(current_button_state && !prev_button_state){
    // Cycle through modes
    current_mode = (current_mode == Mode::VOLTAGE) ? Mode::CURRENT : Mode::VOLTAGE;
  }
  prev_button_state = current_button_state;

  switch(current_mode){
    case Mode::VOLTAGE:
      {
      // Read and display voltage
      float actual_voltage = get_voltage();
      display_two_digits(actual_voltage);
      set_knob_color(0,1,0); // Green
      }
      break;

    case Mode::CURRENT:
      {
      // Read and display current
      float actual_current = get_current();
      display_two_digits(actual_current);
      set_knob_color(0,0,1); // Blue
      }
      break;
  }

  // Read control knob
  long knob_pos = get_constrained_knob_pos();

  // Set voltage
  int target_taps = knob_pos / ENC_PULSES_PER_TAP;
  voltage_adjuster.set_raw(target_taps);
  
#ifdef DEBUG
  Serial.println(target_taps);
#endif

  // Save knob position to EEPROM
  if(millis() - last_save_time > save_frequency){
    EEPROM.put(EEPROM_SAVE_ADDRESS, knob_pos);
    last_save_time = millis();
  }

#ifdef DEBUG
  delay(50);
#else
  delay(10);
#endif
}


/*****************************************************************************/

void set_knob_color(bool r, bool g, bool b){
  digitalWrite(RED_LED_PIN, !r);
  digitalWrite(GREEN_LED_PIN, !g);
  digitalWrite(BLUE_LED_PIN, !b);
}

long get_constrained_knob_pos(){
  long knob_pos = control_knob.read();
  long adjusted_knob_pos = constrain(knob_pos, 0, POT_TAPS * ENC_PULSES_PER_TAP);
  if(knob_pos != adjusted_knob_pos){
    control_knob.write(adjusted_knob_pos);
  }
  return adjusted_knob_pos;
}

float get_voltage(){
  // Return the current voltage of the power supply output
  // Measured from the voltage divider
  /*
    VCC---/\/\/\/---.---/\/\/\/---GND
            R1      |     R2
                Analog In
  */
  long R1 = 65450;
  long R2 = 21800;
  int reading = analogRead(VOLTAGE_MEASUREMENT_PIN);
  float measured_voltage = reading * 4.968 / 1023; // On a 5v scale
  float actual_voltage = measured_voltage * (R1 + R2) / R2; // Convert to 20v scale
  return actual_voltage;
}

float get_current(){
  int reading = analogRead(CURRENT_MEASUREMENT_PIN);
  // Sensor spec: 0.185v/A --> 5.4 A/v
  // 5.4 A/v * 5v / 1023 analog = 0.0264 A / analog
  // 0v --> -13.51 A
  return 0.0264194 * reading - 13.5135;
}

void display_two_digits(float number){
  number = round(abs(number)*10) / 10.0;
  if(number < 10){
    disp_10s((int)number, true);
    disp_1s((int)(number * 10) % 10, false);
  } else {
    number = round(number);
    disp_10s(((int)number / 10) % 10, false);
    disp_1s(((int)number) % 10, false);
  }
}

void disp_1s(int number, bool period){
  byte code;
  switch(number){
    case 0:
      code = 0b11011110;
      break;
    case 1:
      code = 0b00011000;
      break;
    case 2:
      code = 0b11001101;
      break;
    case 3:
      code = 0b10011101;
      break;
    case 4:
      code = 0b00011011;
      break;
    case 5:
      code = 0b10010111;
      break;
    case 6:
      code = 0b11010111;
      break;
    case 7:
      code = 0b00011100;
      break;
    case 8:
      code = 0b11011111;
      break;
    case 9:
      code = 0b10011111;
      break;
    default:
      code = 0b00000000;
  }
  if(period){
    bitWrite(code, 5, 1); // set bit 5
  }
  voltage_output_1s.disp_byte(code);
}

void disp_10s(int number, bool period){
  byte code;
  switch(number){
    case 0:
      code = 0b11011110;
      break;
    case 1:
      code = 0b01000010;
      break;
    case 2:
      code = 0b11101100;
      break;
    case 3:
      code = 0b11100110;
      break;
    case 4:
      code = 0b01110010;
      break;
    case 5:
      code = 0b10110110;
      break;
    case 6:
      code = 0b10111110;
      break;
    case 7:
      code = 0b11000010;
      break;
    case 8:
      code = 0b11111110;
      break;
    case 9:
      code = 0b11110110;
      break;
    default:
      code = 0b00000000;
  }
  if(period){
    bitWrite(code, 0, 1); // set bit 5
  }
  voltage_output_10s.disp_byte(code);
}
