#include "PowerSupplyHardware.h"

//#define DEBUG

// Inspiration: http://www.instructables.com/id/Easy-variable-voltage-power-supply/

///////////// Power Supply Modes ////////////////
enum class Mode { VOLTAGE, CURRENT };
Mode current_mode = Mode::VOLTAGE;
/////////////////////////////////////////////////

// Parameters for saving state when shut down ///
const int EEPROM_SAVE_ADDRESS = 2;
const unsigned long SAVE_FREQUENCY = 10000; // milliseconds
unsigned long last_save_time;

PowerSupplyHardware &powersupply = PowerSupplyHardware::GetInstance();

const int CURRENT_BUFFER_LEN = 10;
float current_buffer[CURRENT_BUFFER_LEN];
int buffer_index;

void setup(){
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("System Init");
#endif
  last_save_time = millis();
  powersupply.load_state(EEPROM_SAVE_ADDRESS);

  for(int i=0; i<CURRENT_BUFFER_LEN; ++i){
    current_buffer[i] = 0;
  }
  buffer_index = 0;
}


bool prev_button_state = false;
void loop(){
  bool current_button_state = powersupply.button_state();
  if(current_button_state && !prev_button_state){
    // Cycle through modes
    current_mode = (current_mode == Mode::VOLTAGE) ? Mode::CURRENT : Mode::VOLTAGE;
  }
  prev_button_state = current_button_state;

  current_buffer[buffer_index] = powersupply.get_current();
  buffer_index = (buffer_index + 1) % CURRENT_BUFFER_LEN;

  switch(current_mode){
    case Mode::VOLTAGE:
      {
      // Read and display voltage
      float actual_voltage = powersupply.get_voltage();
      powersupply.display_two_digits(actual_voltage);
      powersupply.set_knob_color(0,1,0); // Green
      }
      break;

    case Mode::CURRENT:
      {
      // Read and display current
      float actual_current = average(current_buffer, CURRENT_BUFFER_LEN);
      powersupply.display_two_digits(actual_current);
      powersupply.set_knob_color(0,0,1); // Blue
      }
      break;
  }

  // Read control knob
  long knob_pos = powersupply.get_constrained_knob_pos();

  // Set voltage
  int target_taps = knob_pos / PowerSupplyHardware::ENC_PULSES_PER_TAP;
  powersupply.set_voltage_raw(target_taps);

#ifdef DEBUG
  Serial.print("Potentiometer Taps: ");
  Serial.print(target_taps);
  Serial.print("\t Current: ");
  Serial.print(powersupply.get_current());
  Serial.print("\t Avg Current: ");
  Serial.println(average(current_buffer, CURRENT_BUFFER_LEN));
#endif

  // Save knob position to EEPROM
  if(millis() - last_save_time > SAVE_FREQUENCY){
    powersupply.save_state(EEPROM_SAVE_ADDRESS);
    last_save_time = millis();
  }

#ifdef DEBUG
  delay(50);
#else
  delay(10);
#endif
}

float average(float *arr, int arr_len){
  float sum = 0;
  for(int i=0; i<arr_len; ++i){
    sum += arr[i];
  }
  return sum / arr_len;
}
