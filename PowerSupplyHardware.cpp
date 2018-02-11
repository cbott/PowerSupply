#include "PowerSupplyHardware.h"

PowerSupplyHardware::PowerSupplyHardware() :
  control_knob(ENCODER_A_PIN, ENCODER_B_PIN),
  voltage_output_1s(DIGIT1_DATA_PIN, DIGIT1_CLOCK_PIN, DIGIT1_LATCH_PIN),
  voltage_output_10s(DIGIT2_DATA_PIN, DIGIT2_CLOCK_PIN, DIGIT2_LATCH_PIN),
  voltage_adjuster(DIGIPOT_CS_PIN, POT_RESISTANCE, POT_TAPS)
{

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT);

}

static PowerSupplyHardware &PowerSupplyHardware::GetInstance(){
  static PowerSupplyHardware instance;
  return instance;
}

bool PowerSupplyHardware::button_state(){
  return digitalRead(BUTTON_PIN);
}

void PowerSupplyHardware::set_knob_color(bool r, bool g, bool b){
  digitalWrite(RED_LED_PIN, !r);
  digitalWrite(GREEN_LED_PIN, !g);
  digitalWrite(BLUE_LED_PIN, !b);
}

long PowerSupplyHardware::get_constrained_knob_pos(){
  // Prevent knob from setting to value that cannot be set on the digipot
  // TODO: possibly remove
  long knob_pos = control_knob.read();
  long adjusted_knob_pos = constrain(knob_pos, 0, POT_TAPS * ENC_PULSES_PER_TAP);
  if(knob_pos != adjusted_knob_pos){
    control_knob.write(adjusted_knob_pos);
  }
  return adjusted_knob_pos;
}

float PowerSupplyHardware::get_voltage(){
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
void PowerSupplyHardware::set_voltage_raw(int taps){
  // TODO: replace with actual set voltage
  voltage_adjuster.set_raw(taps);
}

float PowerSupplyHardware::get_current(){
  int reading = analogRead(CURRENT_MEASUREMENT_PIN);
  // Sensor spec: 0.185v/A --> 5.4 A/v
  // 5.4 A/v * 5v / 1023 analog = 0.0264 A / analog
  // 0v --> -13.51 A
  const float SLOPE = 5.4;//A/v
  const float SYS_VOLTAGE = 5;//v
  const float A_TO_D_RES = 1023;
  const float CURRENT_SENSE_ZERO = 2.482;//v
  float measured_voltage = SYS_VOLTAGE / A_TO_D_RES * reading;
  return SLOPE * (measured_voltage - CURRENT_SENSE_ZERO);
}

void PowerSupplyHardware::display_two_digits(float number){
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

void PowerSupplyHardware::disp_1s(int number, bool period){
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

void PowerSupplyHardware::disp_10s(int number, bool period){
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

void PowerSupplyHardware::load_state(int eeprom_address){
  // recall last saved control value
  long saved_knob_pos;
  EEPROM.get(eeprom_address, saved_knob_pos);
  control_knob.write(saved_knob_pos);
}

void PowerSupplyHardware::save_state(int eeprom_address){
  EEPROM.put(eeprom_address, get_constrained_knob_pos());
}
