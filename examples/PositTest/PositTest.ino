#include "Posit.h"

char cs[80]; // C string for table
double numbersList[16]= {0,NAN,1.0,2.0,3.0,4.0,7.0,10.0,15.0,25.0,30.0,0.5,0.1,0.02,0.005,0.0001};

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

  Serial.println("Creation of two posit8 values from double"); 
  Posit8 firstPosit (-0.19);
  Posit8 secondPosit (0.65);
  Serial.print("First: ");
  Serial.print(firstPosit.value, BIN);
  Serial.print(" ");
  Serial.println(posit2float(firstPosit));
  Serial.print("Second: ");
  Serial.print(secondPosit.value, BIN);
  Serial.print(" ");
  Serial.println(posit2float(secondPosit)); //*/

  Serial.println("Test of operations");  
  Posit8 sum = Posit8::posit8_add(firstPosit, secondPosit);
  //Posit8 sum = firstPosit + secondPosit;  // doesn"t work yet
  Serial.print("Sum : ");
  Serial.print(sum.value, BIN);
  Serial.print(" ");
  Serial.println(posit2float(sum));

	Posit8 sub = Posit8::posit8_sub(firstPosit, secondPosit);  
  //Posit8 sub = firstPosit - secondPosit;  // doesn"t work yet
  Serial.print("Sub : ");
  Serial.print(sub.value, BIN);
  Serial.print(" ");
  Serial.println(posit2float(sub)); //*/	
  
  Posit8 mul = posit8_mul(firstPosit, secondPosit);  
  //Posit8 mul = firstPosit * secondPosit;  // doesn"t work yet
  Serial.print("Mul : ");
  Serial.print(mul.value, BIN);
  Serial.print(" ");
  Serial.println(posit2float(mul)); //*/	
  
  Posit8 div = posit8_div(firstPosit, secondPosit);  
  //Posit8 div = firstPosit / secondPosit;  // doesn"t work yet
  Serial.print("Div : ");
  Serial.print(div.value, BIN);
  Serial.print(" ");
  Serial.println(posit2float(div)); //*/

  char as[10],bs[10];
  Serial.println("Posit Creation from double"); 
  Serial.println("   A   :   B   :   abin  :   bbin  :   sum   :   sub   :   mul   :   div");
  Serial.println("-------+-------+---------+---------+---------+---------+---------+---------");
  /*/for (int i=0; i<16; i++)
    for (int j=0; j<16; j++) {
      firstPosit=Posit8(numbersList[random(16)]); 
      secondPosit=Posit8(numbersList[random(16)]); 
      itoa(firstPosit.value,as,2);
      itoa(secondPosit.value,bs,2);
      sum = Posit8::posit8_add(firstPosit, secondPosit);
    	sub = Posit8::posit8_sub(firstPosit, secondPosit);  
      mul = posit8_mul(firstPosit, secondPosit);  
      div = posit8_div(firstPosit, secondPosit); 

      sprintf(cs, "%07.3f+%07.3f+%8s+%8s+%07.f+%07.f+%07.f+%07.f", posit2float(firstPosit), 
        posit2float(secondPosit),*as,*bs,posit2float(sum),posit2float(sub),posit2float(mul),posit2float(div));
      Serial.println(cs);
    } //*/
}

void loop() { // Nothing here
}
