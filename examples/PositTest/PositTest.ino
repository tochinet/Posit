/* Sketch to test posit library
 *
 *
 */
#define ES8 2 // number of bits in exponent field (default zero)
#include "Posit.h"

char cs[80]; // C string for table
double numbersList[16]= {0,NAN,1.0,-2.0,3.0,7.0,-11.0,12.0,50.0,-333.0,0.5,NAN,-0.05,0.005,0.01};

void setup() {
  Serial.begin(9600);
  Serial.println("Test of Posit library\n");

  /*Serial.println("Creation of all posit8 values from raw integer");  
  for (int raw = 0; raw<256 ; raw ++) {
    Posit8 rawPosit ((uint8_t)raw); // signed byte since posit are signed
    Serial.print("Raw8 : "); Serial.print(rawPosit.value, BIN);
    Serial.print(" "); Serial.println(posit2float(rawPosit),7);
  } 
  Serial.println(); //*/
 
 /*Serial.println("Creation of many posit16 values from raw integer");  
  for (long raw = 0; raw<65535 ; raw += sqrt(random(50000))) {
    Posit16 rawPosit ((uint16_t)raw); // signed byte since posit are signed
    Serial.print("Raw16 : "); Serial.print(rawPosit.value, BIN);
    Serial.print(" "); Serial.println(posit2float(rawPosit),12);
  } 
  Serial.println(); //*/

  /*Serial.println("Creation of two posit8 values from input strings"); 
  Serial.println("First Posit? ");
  while (Serial.available() == 0) {
   }
  float floatValue = Serial.parseFloat();
  Serial.println(floatValue); 
  Posit8 firstPosit (floatValue);
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.println("Second Posit? ");
  while (Serial.available() == 0) { 
  }
  Posit8 secondPosit (Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  
  Serial.print("First(");
  Serial.print(firstPosit.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(firstPosit),4);
  Serial.print("Second(");
  Serial.print(secondPosit.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(secondPosit),4); 

  Serial.println("Test of operations");  
  //Posit8 sum = Posit8::posit8_add(firstPosit, secondPosit);
  Posit8 sum = firstPosit + secondPosit;  // doesn"t work yet
  Serial.print("Sum(");
  Serial.print(sum.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sum),4);

	//Posit8 sub = Posit8::posit8_sub(firstPosit, secondPosit);  
  Posit8 sub = firstPosit - secondPosit;  // doesn"t work yet
  Serial.print("Sub(");
  Serial.print(sub.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sub),5); 	
  
  Posit8 mul = Posit8::posit8_mul(firstPosit, secondPosit);  
  //Posit8 mul = firstPosit * secondPosit;  // doesn"t work yet
  Serial.print("Mul(");
  Serial.print(mul.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(mul),4); 	
  
  //Posit8 div = Posit8::posit8_div(firstPosit, secondPosit);  
  Posit8 div = firstPosit / secondPosit;  // doesn"t work yet
  Serial.print("Div(");
  Serial.print(div.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(div),5); //*/

  /*Serial.println("Creation of two posit16 values from input strings"); 
  Serial.println("First Posit? ");
  while (Serial.available() == 0) {
   }
  Posit16 firstP16 (Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.println("Second Posit? ");
  while (Serial.available() == 0) { 
  }
  Posit16 secondP16 (Serial.parseFloat());
  
  Serial.print("First(");
  Serial.print(firstP16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(firstP16),10);
  Serial.print("Second(");
  Serial.print(secondP16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(secondP16),10); 

  Serial.println("Test of operations");  
  Posit16 sum16 = Posit16::posit16_add(firstP16, secondP16);
  //Posit16 sum = firstP16 + secondP16;  // doesn't work yet
  Serial.print("Sum(");
  Serial.print(sum16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sum16),10);

  Posit16 sub16 = Posit16::posit16_sub(firstP16, secondP16);
  //Posit16 sub16 = firstP16 - secondP16;  // doesn't work yet
  Serial.print("Sub(");
  Serial.print(sub16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sub16),10); 

  Posit16 mul16 = Posit16::posit16_mul(firstP16, secondP16);
  //Posit16 sum = firstP16 + secondP16;  // doesn't work yet
  Serial.print("Mul(");
  Serial.print(mul16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(mul16),10);

  Posit16 div16 = Posit16::posit16_div(firstP16, secondP16);
  //Posit16 sub16 = firstP16 - secondP16;  // doesn't work yet
  Serial.print("Div(");
  Serial.print(div16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(div16),10); //*/
  

  Serial.println("Table of 16 random Posit 8 results"); 
  randomSeed(millis());
  char as[10],bs[10];
  Serial.println("   A   :   B   :   abin  :   bbin  :   sum   :   sub   :   mul   :   div");
  Serial.println("-------+-------+---------+---------+---------+---------+---------+---------");
  for (int j=0; j<16; j++) {
      Posit8 firstPosit=Posit8(numbersList[random(16)]); 
      Posit8 secondPosit=Posit8(numbersList[random(16)]); 
      Posit8 sum = Posit8::posit8_add(firstPosit, secondPosit);
    	Posit8 sub = Posit8::posit8_sub(firstPosit, secondPosit);  
      Posit8 mul = Posit8::posit8_mul(firstPosit, secondPosit);  
      Posit8 div = Posit8::posit8_div(firstPosit, secondPosit); 

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

  Serial.println("Table of 16 random Posit16 results"); 
  randomSeed(millis());
  char as[10],bs[10];
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

void loop() { // Nothing here if not uncommented
  //Serial.println("Creation of two posit16 values from input strings"); 
  /*Serial.println("First Posit? ");
  while (Serial.available() == 0) {
   }
  Posit16 firstP16 (Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars
  Serial.println("Second Posit? ");
  while (Serial.available() == 0) { 
  }
  Posit16 secondP16 (Serial.parseFloat());
  while (Serial.available() > 0) Serial.read(); // Eliminate extra chars

  Serial.print("First(");
  Serial.print(firstP16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(firstP16),15);
  Serial.print("Second(");
  Serial.print(secondP16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(secondP16),15); 

  Serial.println("Test of operations");  
  Posit16 sum16 = Posit16::posit16_add(firstP16, secondP16);
  //Posit16 sum = firstP16 + secondP16;
  Serial.print("Sum(");
  Serial.print(sum16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sum16),15);

  Posit16 sub16 = Posit16::posit16_sub(firstP16, secondP16);
  //Posit16 sub16 = firstP16 - secondP16;
  Serial.print("Sub(");
  Serial.print(sub16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(sub16),15);

  Posit16 mul16 = Posit16::posit16_mul(firstP16, secondP16);
  //Posit16 sum = firstP16 + secondP16;
  Serial.print("Mul(");
  Serial.print(mul16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(mul16),15);

  Posit16 div16 = Posit16::posit16_div(firstP16, secondP16);
  //Posit16 sub16 = firstP16 - secondP16;
  Serial.print("Div(");
  Serial.print(div16.value, BIN);
  Serial.print(") ");
  Serial.println(posit2float(div16),15); //*/
}
