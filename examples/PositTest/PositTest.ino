/* Sketch to test posit library
 *
 *
 */
#define ES8 2 // number of bits in exponent field (default zero)
#include "Posit.h"

char cs[80]; // C string for table
double numbersList[16]= {0,NAN,1.0,2.0,3.0,7.0,11.0,12.0,50.0,333.0,0.5,0.12,0.05,0.005,0.0001};

void setup() {
  Serial.begin(9600);
  Serial.println("Test of Posit library\n");

  /*Serial.println("Creation of all posit8 values from raw integer");  
  for (int raw = 0; raw<256 ; raw ++) {
    Posit8 rawPosit ((int8_t)raw); // signed byte since posit are signed
  Serial.print("Raw : "); Serial.print(rawPosit.value, BIN);
  Serial.print(" "); Serial.println(posit2float(rawPosit),5);
  } 
  Serial.println(); //*/

  Serial.println("Creation of two posit8 values from input strings"); 
  Serial.println("First Posit? ");
  while (Serial.available() == 0) {
   }
  Posit8 firstPosit (Serial.parseFloat());
  //delay(300);
    while (Serial.available() > 0) { // Eliminate extra chars
      Serial.read();
    }
  Serial.println("Second Posit? ");
  while (Serial.available() == 0) { 
  }
  Posit8 secondPosit (Serial.parseFloat());
  
  Serial.print("First(");
  Serial.print(firstPosit.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(firstPosit),4);
  Serial.print("Second(");
  Serial.print(secondPosit.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(secondPosit),4); //*/

  Serial.println("Test of operations");  
  Posit8 sum = Posit8::posit8_add(firstPosit, secondPosit);
  //Posit8 sum = firstPosit + secondPosit;  // doesn"t work yet
  Serial.print("Sum(");
  Serial.print(sum.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sum),4);

	Posit8 sub = Posit8::posit8_sub(firstPosit, secondPosit);  
  //Posit8 sub = firstPosit - secondPosit;  // doesn"t work yet
  Serial.print("Sub(");
  Serial.print(sub.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sub),5); //*/	
  
  Posit8 mul = posit8_mul(firstPosit, secondPosit);  
  //Posit8 mul = firstPosit * secondPosit;  // doesn"t work yet
  Serial.print("Mul(");
  Serial.print(mul.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(mul),4); //*/	
  
  Posit8 div = posit8_div(firstPosit, secondPosit);  
  //Posit8 div = firstPosit / secondPosit;  // doesn"t work yet
  Serial.print("Div(");
  Serial.print(div.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(div),5); //*/

  randomSeed(millis());
  char as[10],bs[10];
  Serial.println("Table of random results"); 
  Serial.println("   A   :   B   :   abin  :   bbin  :   sum   :   sub   :   mul   :   div");
  Serial.println("-------+-------+---------+---------+---------+---------+---------+---------");
  for (int j=0; j<16; j++) {
      firstPosit=Posit8(numbersList[random(16)]); 
      secondPosit=Posit8(numbersList[random(16)]); 
      sum = Posit8::posit8_add(firstPosit, secondPosit);
    	sub = Posit8::posit8_sub(firstPosit, secondPosit);  
      mul = posit8_mul(firstPosit, secondPosit);  
      div = posit8_div(firstPosit, secondPosit); 

      Serial.print(posit2float(firstPosit),3); // No way to align numbers in table
      Serial.print(" + ");
      Serial.print(posit2float(secondPosit),3);
      Serial.print(" + ");
      Serial.print(firstPosit.value,BIN);
      Serial.print(" + ");
      Serial.print(secondPosit.value,BIN);
      Serial.print(" + ");
      Serial.print(posit2float(sum),3);
      Serial.print(" + ");
      Serial.print(posit2float(sub),3);
      Serial.print(" + ");
      Serial.print(posit2float(mul),3);
      Serial.print(" + ");
      Serial.println(posit2float(div),3);
    } //*/
}

void loop() { // Nothing here
}
