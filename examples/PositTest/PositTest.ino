// examples/PositTest/PositTest.ino
#include <Posit.h>

void setup() {
    Serial.begin(9600);
    Serial.println("Test of Posit library : creation and sum");
    Serial.print("Sum: ");
    
    Posit8 positZero(0); 
    Posit8 positOne(0b0 10 00 000); // Regime 10 means power zero 2^0

    Posit8 sum = posit8_add(zero, one);
	Posit8 mul = posit8_add(zero, one);
    Serial.print("Sum : ");
    Serial.println(sum.value, HEX);
    Serial.print("Mult: ");
    Serial.println(mul.value, HEX);
}

void loop() {
    // nothing here
}
