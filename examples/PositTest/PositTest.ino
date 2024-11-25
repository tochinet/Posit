/* Sketch to test posit library
 *
 * Contains several test scenarios that can easily be commented out or uncommented
 * - Creation and printing of all (256) Posit8 and/or a few hundreds Posit16 values 
 * - Input of two posits from Serial and test of four basic operations (with overloading)
 * - Creation of (poorly aligned) tables of 16 randomly chosen pairs of values from a predefined list
 *    and printing of the values, BINary content, and results of 4 basic operations
 *
 * The test scenarios are available below for both Posit8 and Posit16,2
 */

#define ES8 0 // number of bits in exponent field (default is two in library)
//#define EPSILON 0.0 // uncomment to disable rounding small values down to zero
#include "Posit.h"

double numbersList[16]= {0,NAN,1.0,-1.01,3.14159,-7.0,11.0,-15.0,50.0,-333.0,0.5,-0.09,0.05,-0.005,0.0001};
// array of floats to choose randomly from for the table test scenarios

void setup() {
  Serial.begin(9600);
  Serial.println("Test of Posit library\n");

  Serial.print("Input for seeding random ? ");
  while (Serial.available() == 0);
  long seed = Serial.parseFloat() + millis();
  randomSeed (seed);
  Serial.println(seed);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars */

  /*/Serial.println("Creation of all posit8 values from raw integer");  
  for (byte raw = 0; raw < 0x40 ; raw ++) {
    Serial.print("xx");
    for (int8_t power=5; power>=0; power--) {
      if (raw & (1<<power)) Serial.print('1');else Serial.print('0');
    }
    Serial.print(" : ");
    for (byte lsb = 0; lsb <4; lsb++) {
      Posit8 rawPosit ((uint8_t)(raw+(lsb<<6)));
      Serial.print(" "); Serial.print(posit2float(rawPosit),7);
    }
    Serial.println();
  } 
  Serial.println(); //*/
 
 /*/Serial.println("Creation of about 350 posit16 values from raw integer");  
  uint16_t count=0;
  for (unsigned long raw = 0; raw<65536L ; raw += (1+random(20)*random(10)*random(10))) { // between 1 and 2k, but more small numbers
    Posit16 rawPosit ((uint16_t)raw);
    Serial.print("Raw16 ("); Serial.print(count++);
    Serial.print(") "); Serial.print(rawPosit.value, BIN);
    Serial.print(" "); Serial.println(posit2float(rawPosit),12);
  } 
  Serial.println(); //*/

  /*/Serial.println("Creation of two posit8 values from input strings"); 
  Serial.print("First Posit8 ? ");
  while (Serial.available() == 0) ;
  float floatValue = Serial.parseFloat();
  Serial.println(floatValue); 
  Posit8 firstPosit (floatValue);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.print("Second Posit8 ? ");
  while (Serial.available() == 0) ;
  floatValue = Serial.parseFloat();
  Serial.println(floatValue);
  Posit8 secondPosit (floatValue);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.println(posit2float(secondPosit)); 
  
  Serial.print("First(");
  Serial.print(firstPosit.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(firstPosit),4);
  Serial.print("Second(");
  Serial.print(secondPosit.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(secondPosit),4); //*/

  Serial.println("Test of operations");  
  /*/Posit8 sum = Posit8::posit8_add(firstPosit, secondPosit);
  Posit8 sum = firstPosit + secondPosit;
  Serial.print("Sum(");
  Serial.print(sum.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sum),4);

  //Posit8 sub = Posit8::posit8_sub(firstPosit, secondPosit);  
  Posit8 sub = firstPosit - secondPosit;
  Serial.print("Sub(");
  Serial.print(sub.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sub),5); 	
  
  //Posit8 mul = Posit8::posit8_mul(firstPosit, secondPosit);  
  Posit8 mul = firstPosit * secondPosit;
  Serial.print("Mul(");
  Serial.print(mul.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(mul),4); 	
  
  //Posit8 div = Posit8::posit8_div(firstPosit, secondPosit);  
  Posit8 div = firstPosit / secondPosit;
  Serial.print("Div(");
  Serial.print(div.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(div),5); //*/

  /*/Posit8 op8 = posit8_sqrt(firstPosit);
  Serial.print("Sqrt(");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4); //*/

  /*/Serial.println("Creation of two posit16 values from input strings"); 
  Serial.println("First Posit16 ? ");
  while (Serial.available() == 0) ;
  Posit16 firstP16 (Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.println("Second Posit16 ? ");
  while (Serial.available() == 0) ;
  Posit16 secondP16 (Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  
  Serial.print("First(");
  Serial.print(firstP16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(firstP16),10);
  Serial.print("Second(");
  Serial.print(secondP16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(secondP16),10); 

  Serial.println("Test of operations");  
  //Posit16 sum16 = Posit16::posit16_add(firstP16, secondP16);
  Posit16 sum16 = firstP16 + secondP16;
  Serial.print("Sum(");
  Serial.print(sum16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sum16),10);

  //Posit16 sub16 = Posit16::posit16_sub(firstP16, secondP16);
  Posit16 sub16 = firstP16 - secondP16;
  Serial.print("Sub(");
  Serial.print(sub16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sub16),10); 

  //Posit16 mul16 = Posit16::posit16_mul(firstP16, secondP16);
  Posit16 mul16 = firstP16 * secondP16;
  Serial.print("Mul(");
  Serial.print(mul16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(mul16),10);

  //Posit16 div16 = Posit16::posit16_div(firstP16, secondP16);
  Posit16 div16 = firstP16 / secondP16;
  Serial.print("Div(");
  Serial.print(div16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(div16),10); //*/
  

  /*/Serial.println("Table of 16 random Posit 8 results"); 
  randomSeed(millis());
  Serial.println("    A   :    B   :   abin   :   bbin   :   sum    :   sub   :   mul   :   div");
  Serial.println("--------+--------+----------+----------+----------+---------+---------+---------");
  for (int j=0; j<16; j++) {
      Posit8 firstPosit=Posit8(numbersList[random(16)]); 
      Posit8 secondPosit=Posit8(numbersList[random(16)]); 
      //Posit8 sum = Posit8::posit8_add(firstPosit, secondPosit);
      //Posit8 sub = Posit8::posit8_sub(firstPosit, secondPosit);  
      //Posit8 mul = Posit8::posit8_mul(firstPosit, secondPosit);  
      //Posit8 div = Posit8::posit8_div(firstPosit, secondPosit); 
      Posit8 sum = firstPosit + secondPosit;
      Posit8 sub = firstPosit - secondPosit;
      Posit8 mul = firstPosit * secondPosit;
      Posit8 div = firstPosit / secondPosit;

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
  randomSeed(millis());
  Serial.println("   A   :   B   :   abin  :   bbin  :   sum   :   sub   :   mul   :   div");
  Serial.println("-------+-------+---------+---------+---------+---------+---------+---------");
  for (int j=0; j<16; j++) {
      Posit16 firstPosit=Posit16(numbersList[random(16)]); 
      Posit16 secondPosit=Posit16(numbersList[random(16)]); 
      //Posit16 sum = Posit16::posit16_add(firstPosit, secondPosit);
      //Posit16 sub = Posit16::posit16_sub(firstPosit, secondPosit);  
      //Posit16 mul = Posit16::posit16_mul(firstPosit, secondPosit);  
      //Posit16 div = Posit16::posit16_div(firstPosit, secondPosit); 
      Posit16 sum = firstPosit + secondPosit;
      Posit16 sub = firstPosit - secondPosit;  
      Posit16 mul = firstPosit * secondPosit;  
      Posit16 div = firstPosit / secondPosit; 

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

void loop() { // Nothing here if not uncommented
  /*/Serial.print("Posit(float) ? ");
  while (Serial.available() == 0) ;
  float floatValue = Serial.parseFloat();
  Serial.println(floatValue); 
  Posit8 firstPosit (floatValue);
  Posit16 StPosit(floatValue);
  while (Serial.available() > 0) Serial.read();

  Posit8 op8 = posit8_sqrt(firstPosit);
  Posit16 op16 = posit16_sqrt(StPosit);
  
  Serial.print("Sqrt(");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.print(posit2float(op8),4); 
  Serial.print("(");
  Serial.print(op16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op16),6); 
  
  /*op8 = posit8_next(firstPosit);
  Serial.print("Next(");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4); 

  op8 = posit8_prior(firstPosit);
  Serial.print("Prior(");
  Serial.print(op8.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(op8),4); //*/

  /**/Serial.print("First Posit16 ? ");
  while (Serial.available() == 0) ;
  float floatValue=Serial.parseFloat();
  Posit16 firstP16 (floatValue);
  Serial.print(floatValue); Serial.print("(");
  Serial.print(firstP16.value, BIN); Serial.print(") ");
  Serial.println(posit2float(firstP16),10);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.print("Second Posit16 ? ");
  while (Serial.available() == 0) ;
  Posit16 secondP16 (floatValue = Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.print(floatValue);Serial.print("(");
  Serial.print(secondP16.value, BIN); Serial.print(") ");
  Serial.println(posit2float(secondP16),15); 
  
  Serial.println("Test of operations");  
  Posit16 op16 = firstP16 + secondP16;
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
  Serial.println(posit2float(op16),15); //*/
}
