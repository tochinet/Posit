/***********************
 * Arduino sketch to evaluate the memory used by each routine or operation on various types.
 * Obviously, the target is Posit8 and Posit16, but with comparison with byte, int et float 
 */

#include "Posit.h"

#define TYPE_T posit8_t
//#define TYPE_T posit16_t
//#define TYPE_T byte // == uint8_t
//#define TYPE_T int // = int16_t
//#define TYPE_T float // = IEEE754
//#define TYPE_T double // = float32 for AVR

void setup() {
  Serial.begin(9600);
  Serial.println("Test of Posit library\n");
}

void loop() {
  // Create two values, add them, repeat with time measurement
  TYPE_T res=0; arg1=1.0, arg2=0.1;
  for (byte i=0; i<1000; i++) {
    res = arg1+arg2;
  }
  Serial.print("Calculated 100x in "); millis()-
}
