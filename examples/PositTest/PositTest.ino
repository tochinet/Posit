/* Sketch to test posit library
 *
 * Contains several test scenarios that can easily be commented out or uncommented
 * - Creation and printing of all (256) Posit8 and/or a few hundreds Posit16 values 
 * - Creation of (poorly aligned) tables of 16 randomly chosen pairs of values from a predefined list
 *    and printing of the values, BINary content, and results of 4 basic operations
 * - Input of two posits from Serial and test of four basic operations (with overloading),
 *   sqrt, sin ,cos, next, prior
 *
 * The test scenarios are available below for both Posit8 and Posit16,2
 */

#define DEBUG // comment out to avoid debug capabilities
//#define ES8 0 // number of bits in exponent field (default is two in library)
//#define EPSILON 0.000001 // uncomment to enable rounding small values down to zero
#include "Posit.h"

double numbersList[16]= {0,NAN,1.0,-1.01,3.14159,-7.0,11.0,-15.0,50.0,-333.0,0.5,-0.09,0.05,-0.005,0.0001};
// array of floats to choose randomly from for the table test scenarios

void randomSeeder() {
  Serial.print("Input for seeding random ? ");
  while (Serial.available() == 0);
  long seed = Serial.parseFloat() + millis();
  randomSeed (seed);
  Serial.println(seed);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars */
}

void setup() {
  Serial.begin(9600);
  Serial.println("Test of Posit library\n");

  /*/Serial.println("Creation of all posit8 values from raw integer"); 
  // This first check validates the posit2float method for Posit8 
  Serial.println("         : 00xxxxxx : 01xxxxxx : 10xxxxxx : 11xxxxxx");
  Serial.println("---------+----------+----------+----------+---------");
  for (byte raw = 0; raw < 0x40 ; raw ++) {
    Serial.print("xx");
    for (int8_t power=5; power>=0; power--) {
      if (raw & (1<<power)) Serial.print('1');else Serial.print('0');
    }
    Serial.print(" :");
    for (byte lsb = 0; lsb <4; lsb++) {
      posit8_t rawPosit ((uint8_t)(raw+(lsb<<6)));
      float floatValue=posit2float(rawPosit);
      float floatCopy=abs(floatValue);
      uint8_t decimals=7;
      while (floatCopy >= 10.0) {
        decimals--;
        floatCopy/=10.0;
       }
      Serial.print(" "); Serial.print(floatValue,decimals);
    }
    Serial.println();
  } 
  Serial.println(); //*/
 
 /*/Serial.println("Creation of about 350 posit16 values from raw integer");  
  uint16_t count=0;
  for (unsigned long raw = 0; raw<65536L ; raw += (1+random(20)*random(10)*random(10))) { // between 1 and 2k, but more small numbers
    posit16_t rawPosit ((uint16_t)raw);
    Serial.print("Raw16 ("); Serial.print(count++);
    Serial.print(") "); Serial.print(rawPosit.value, BIN);
    Serial.print(" "); Serial.println(posit2float(rawPosit),12);
  } 
  Serial.println(); //*/

  /*/Serial.println("Table of 16 random Posit 8 results"); 
  randomSeeder();
  Serial.println("    A   :    B   :   abin   :   bbin   :   sum    :   sub   :   mul   :   div");
  Serial.println("--------+--------+----------+----------+----------+---------+---------+---------");
  for (int j=0; j<16; j++) {
      posit8_t firstPosit=posit8_t(numbersList[random(16)]); 
      posit8_t secondPosit=posit8_t(numbersList[random(16)]); 
      //posit8_t sum = posit8_t::posit8_add(firstPosit, secondPosit);
      //posit8_t sub = posit8_t::posit8_sub(firstPosit, secondPosit);  
      //posit8_t mul = posit8_t::posit8_mul(firstPosit, secondPosit);  
      //posit8_t div = posit8_t::posit8_div(firstPosit, secondPosit); 
      posit8_t sum = firstPosit + secondPosit;
      posit8_t sub = firstPosit - secondPosit;
      posit8_t mul = firstPosit * secondPosit;
      posit8_t div = firstPosit / secondPosit;

      Serial.print(posit2float(firstPosit),4); // No way to align numbers in table
      Serial.print(" + ");
      Serial.print(posit2float(secondPosit),4);
      Serial.print(" + ");
      Serial.print(firstPosit.value,BIN);
      Serial.print(" + ");
      Serial.print(secondPosit.value,BIN);
      Serial.print(" + ");
      Serial.print(posit2float(sum),4);
      Serial.print(" + ");
      Serial.print(posit2float(sub),4);
      Serial.print(" + ");
      Serial.print(posit2float(mul),4);
      Serial.print(" + ");
      Serial.println(posit2float(div),4);
    } //*/

  /*/Serial.println("Table of 16 random Posit16 results"); 
  randomSeeder();
  Serial.println("   A   :   B   :   abin  :   bbin  :   sum   :   sub   :   mul   :   div");
  Serial.println("-------+-------+---------+---------+---------+---------+---------+---------");
  for (int j=0; j<16; j++) {
      posit16_t firstPosit=posit16_t(numbersList[random(16)]); 
      posit16_t secondPosit=posit16_t(numbersList[random(16)]); 
      //posit16_t sum = posit16_t::posit16_add(firstPosit, secondPosit);
      //posit16_t sub = posit16_t::posit16_sub(firstPosit, secondPosit);  
      //posit16_t mul = posit16_t::posit16_mul(firstPosit, secondPosit);  
      //posit16_t div = posit16_t::posit16_div(firstPosit, secondPosit); 
      posit16_t sum = firstPosit + secondPosit;
      posit16_t sub = firstPosit - secondPosit;  
      posit16_t mul = firstPosit * secondPosit;  
      posit16_t div = firstPosit / secondPosit; 
      //posit8_t p8sub = sub; // casting to 8 bits
      //sub = p8sub; // and back to 16 bits

      Serial.print(posit2float(firstPosit),6); // No way to align numbers in table
      Serial.print(" + ");
      Serial.print(posit2float(secondPosit),6);
      Serial.print(" + ");
      Serial.print(firstPosit.value,BIN);
      Serial.print(" + ");
      Serial.print(secondPosit.value,BIN);
      Serial.print(" + ");
      Serial.print(posit2float(sum),6);
      Serial.print(" + ");
      Serial.print(posit2float(sub),6);
      Serial.print(" + ");
      Serial.print(posit2float(mul),6);
      Serial.print(" + ");
      Serial.println(posit2float(div),6);
    } //*/  
}

void loop() { 
  /**/Serial.println("Creation of two posit8 values from input strings"); 
  Serial.print("First Posit8 ? ");
  while (Serial.available() == 0) ;
  float floatValue = Serial.parseFloat();
  Serial.println(floatValue,4); 
  posit8_t firstPosit (floatValue);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.print("Second Posit8 ? ");
  while (Serial.available() == 0) ;
  floatValue = Serial.parseFloat();
  Serial.println(floatValue,4);
  posit8_t secondPosit (floatValue);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  
  posit8_t op8 = posit8_t::posit8_add(firstPosit, secondPosit);
  //posit8_t op8 = firstPosit + secondPosit;
  Serial.print("Sum (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  //op8 = posit8_t::posit8_sub(firstPosit, secondPosit);  
  op8 = firstPosit - secondPosit;
  Serial.print("Sub (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),5); 	
  
  //op8 = posit8_t::posit8_mul(firstPosit, secondPosit);  
  op8 = firstPosit * secondPosit;
  Serial.print("Mul (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4); 	
  
  //op8 = posit8_t::posit8_div(firstPosit, secondPosit);  
  op8 = firstPosit / secondPosit;
  Serial.print("Div (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),5);

  op8 = posit8_t::posit8_sqrt(firstPosit);
  Serial.print("Sqrt1 (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  op8 = posit8_t::posit8_sqrt(secondPosit);
  Serial.print("Sqrt2 (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  op8 = posit8_t::posit8_sin(firstPosit);
  Serial.print("Sin1 (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  op8 = posit8_t::posit8_sin(secondPosit);
  Serial.print("Sin2 (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  op8 = posit8_t::posit8_cos(firstPosit);
  Serial.print("Cos1 (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  op8 = posit8_t::posit8_cos(secondPosit);
  Serial.print("Cos2 (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4);

  op8 = posit8_t::posit8_next(firstPosit);
  Serial.print("Next (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4); 

  op8 = posit8_t::posit8_prior(firstPosit);
  Serial.print("Prior (");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4); //*/

  /*/Serial.println("Creation of two posit16 values from input strings"); 
  Serial.print("First Posit16 ? ");
  while (Serial.available() == 0) ;
  float floatValue=Serial.parseFloat();
  posit16_t firstP16 (floatValue);
  Serial.print(floatValue); Serial.print("(");
  Serial.print(firstP16.value, BIN); Serial.print(") ");
  Serial.println(posit2float(firstP16),10);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.print("Second Posit16 ? ");
  while (Serial.available() == 0) ;
  posit16_t secondP16 (floatValue = Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.print(floatValue);Serial.print("(");
  Serial.print(secondP16.value, BIN); Serial.print(") ");
  Serial.println(posit2float(secondP16),15); 
  
  Serial.println("Test of operations");  
  posit16_t op16 = firstP16 + secondP16;
  Serial.print("Sum(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);

  op16 = firstP16 - secondP16;
  Serial.print("Sub(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);

  op16 = firstP16 * secondP16;
  Serial.print("Mul(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);

  op16 = firstP16 / secondP16;
  Serial.print("Div(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);
  
  op16 = posit16_sqrt(firstP16);
  Serial.print("Sqrt1(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);

  op16 = posit16_sqrt(secondP16);
  Serial.print("Sqrt2(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);
  
  op16 = posit16_sin(firstP16);
  Serial.print("Sin1(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);

  op16 = posit16_sin(secondP16);
  Serial.print("Sin2(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);
  
  op16 = posit16_cos(firstP16);
  Serial.print("Cos1(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);

  op16 = posit16_cos(secondP16);
  Serial.print("Cos2(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15);
  
  op16 = posit16_next(firstP16);
  Serial.print("Next(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15); 
  
  op16 = posit16_prior(firstP16);
  Serial.print("Prior(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),15); //*/
}
