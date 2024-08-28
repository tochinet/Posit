/****************************************************************************

  Posit Library for Arduino

Copyright (c) 2024 Christophe Vermeulen 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See https://github.com/tochinet/Posit/
*****************************************************************************/

#ifndef ES8
#define ES8 0  // No exponent bits in Posit8. Define as 2 for standard compliance
#endif
#define ES16 2 // Two exponent bits in Posit16, compliant to standard
#ifndef EPSILON
#define EPSILON 0.000001 // round to zero below 1ppm. Define as 0.0 for standard compliance
#endif

char s[30]; // temporary C string for Serial debug using sprintf

class Posit8 {
  private:
    //uint8_t value; // maybe in future

  public:
    uint8_t value; // should become private ?

    Posit8(int8_t raw = 0) { // Construct from raw signed byte
      this->value = raw;
    }
#ifdef byte
    Posit8(byte raw = 0) { // Construct from raw byte type (unsigned char)
      this->value = (int8_t)raw;
    }
#endif
    Posit8(double v = 0.0) { // Construct from double by casting to float
      this->value = Posit8((float)v).value; 
    }

    Posit8(int v = 0) { // Construct from int by casting to float
      this->value = Posit8((float)v).value; 
    }

    Posit8(float v = 0) { // Construct from float32
      union float_int { // for bit manipulation
        float tempFloat; // little endian in AVR8
        uint32_t tempInt; // little-endian as well
        uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
      } tempValue ;

      this->value = 0;
      if (v < 0) {      // negative numbers
        if (v >= -EPSILON) return; // very small neg numbers ~=0, non-standard
        this->value += 128; // set sign and continue
      } else { // v>=0
        if (v <= EPSILON) return; // include EPSILON required if EPSILON were zero
      }
      if(isnan(v)) {
        this->value = 0x80; // NaR
        return;
      }
      tempValue.tempFloat = v;
      tempValue.tempInt <<= 1; // eliminate sign
      int8_t exponent = tempValue.tempBytes[3] - 127;
      /*sprintf(s, "exp=%02x mant=%02x%02x%02x", exponent, tempValue.tempBytes[2],
        tempValue.tempBytes[1], tempValue.tempBytes[0]); Serial.println(s); //*/
      int8_t bitCount = 6; // first regime bit
      if (exponent >= 0) { // positive exponent, regime bits are 1
        while (exponent-- >= 0 && bitCount >= 0) {
          this->value |= (1 << bitCount--); // es=0 assumed
        }
        bitCount--; // skip terminating zero bit
      }
      else { // abs(v) < 1, regime bits are zero
        while (exponent++ < 0 && bitCount--> 0) ; // do nothing, bits are already zero
        this->value |= (1 << bitCount--); // terminating bit is 1
      }
      //Serial.println(bitCount);
        if (bitCount > 0) { // still space for exp or mantissa
          // esp bits handling will come here
          int8_t mantissacount = 1;
          while (bitCount-- >= 0) { //Serial.print(tempValue.tempBytes[2]);
            if (tempValue.tempBytes[2] & (1 << (8 - mantissacount++))) 
              this->value |= (1 << (bitCount+1));
          }
        }
    } // End of constructors

// Helper method to split a posit into constituents
  static positSplit(Posit8 p, boolean *sign, boolean *bigNum, int8_t *exponent, uint8_t *mantissa) {
    int8_t bitCount; // posit bit counter for regime, exp and mantissa
    Serial.print("Splitting ");Serial.print(p.value,BIN);Serial.println(".");
    *sign = (p.value & 128);
    *exponent =-1; // exponent means power of 2, exp means exponent bits outside regime
    if (*bigNum = (p.value & 64)) *exponent++; // boolean assignment inside test;
    bitCount=5;
    if (*bigNum) Serial.print(">1. ");else Serial.print("<1. ");
    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((*bigNum)<<bitCount--)) { // still regime
      if(*bigNum) (*exponent) +=1; else (*exponent) -=1 ; // change to 2^es if exponent bits // (1<<ES8)
      Serial.print(*exponent);Serial.print(" ");Serial.print(bitCount);Serial.print(", ");
      if (bitCount==-1) break; // all regime, no space for exponent or mantissa
    }
    // threat exp bits here if any, TODO
    // then treat mantissa bits if any
    (*mantissa)=p.value << (6-bitCount); // all zeros if none exist
    (*mantissa) |=128 ; // add implied one;
    Serial.print("exponent before "); Serial.println(*mantissa);
  }  

// Methods for posit8 arithmetic
  static Posit8 posit_add(Posit8 a, Posit8 b) { // better as externalized function ?
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent,bexponent; 
    uint8_t amantissa,bmantissa; 
    int8_t bitCount; // posit bit counter
    int8_t tempResult=0;

    if (a.value == 0x80 || b.value == 0x80) return Posit8((int8_t)0x80);
    if (a.value == 0) return b;
    if (b.value == 0) return a;

// Move to separate (inline?) function ? Could be useful for all operations
    positSplit(a, &aSign, &abigNum, &aexponent, &amantissa);
    Serial.print("exponent after "); Serial.println( aexponent);
    sprintf(s, "a=%02X x 2\^%02X ", amantissa, aexponent); Serial.println(s); //*/
    positSplit(b, &bSign, &bbigNum, &bexponent, &bmantissa);
    /*aSign= (a.value & 128);
    aexponent =-1;
    if (abigNum = (a.value & 64)) aexponent++; // boolean assignment inside test;
    bitCount=5;
    //first extract exponent from regime bits
    while ((a.value & (1 << bitCount)) == (abigNum<<bitCount--)) { // still regime
      abigNum ? aexponent ++ : aexponent -- ; // change to 2^es if exponent bits // 1<<(1+ES8)
      if (bitCount==-1) break; // no exponent or mantissa
    }
    // threat exponent bits here if any TODO
    // then treat mantissa bits if any
    amantissa=a.value << (6-bitCount); // all zeros if none exist
    amantissa |=128 ; // add implied one; //*/
    sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.println(s); //*/
    
    bSign= (b.value & 128);
    bexponent=-1;if (bbigNum = (b.value & 64)) bexponent++;
    bitCount=5; // reinitialize counter
    //first extract exponent from regime bits
    while ((b.value & (1 << bitCount)) == (bbigNum<<bitCount--)) { // still regime
      bbigNum ? bexponent ++ : bexponent -- ; // change to 2^es if exponent bits // 1<<(1+ES8)
      if (bitCount==-1) break; // no exponent or mantissa
    }
    // threat exponent bits here if any TODO
    // then treat mantissa bits if any
    bmantissa=b.value << (6-bitCount);
    bmantissa |=128 ; // add implied one; */
    
    // align smaller number
    if (aexponent>bexponent) bmantissa >>= (aexponent-bexponent);
    if (aexponent<bexponent) amantissa >>= (bexponent-aexponent);
    int tempExponent=max(aexponent, bexponent);
    /* Check scaled mantissa * /
    sprintf(s, "aexp=%02x 1mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x 1mant=%02x ", bexponent, bmantissa); Serial.println(s); //*/
    int tempMantissa = amantissa+bmantissa; // Sign untreated here
    if (aSign) tempMantissa -= 2* amantissa;
    if (bSign) tempMantissa -= 2* bmantissa;
    //sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.print(s);

  // assign sign of result
    if (tempMantissa<0) {
      tempResult |=128;
      tempMantissa = - tempMantissa;
      //sprintf(s, "-sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.println(s);
    }
    if (tempMantissa ==0) return Posit8((int8_t)0);

    if(tempMantissa>255) tempExponent++; // one more power of two if sum carries to bit8
                else tempMantissa<<=1; // eliminate uncoded msb otherwise (same exponent or less)
    while(tempMantissa<256) { // if msb not reached
      tempExponent--; // divide by two
      tempMantissa<<=1; // eliminate msb uncoded
    }
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s); //*/

    bitCount=6; // start of regime
    // Write regimebits
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-- >= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--); 
      }
      bitCount--; // terminating zero
      //sprintf(s, "1reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount-- > 0) ; // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      //sprintf(s, "0reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    
    if (bitCount > 0) { // still space for exp or mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++))) 
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit8(tempResult);
  }

  static Posit8 posit_sub(Posit8 a, Posit8 b) {
    if (a.value == 0x80 || b.value == 0x80) return Posit8((int8_t)0x80);
    b.value=b.value ^ 128; // change sign
    return posit_add(a,b); // temp solution to compile
  }

    /*/ Operator overloading for Posit8
        Posit8 operator+(const Posit8& other) const;
        Posit8 operator-(const Posit8& other) const;
        Posit8 operator*(const Posit8& other) const;
        Posit8 operator/(const Posit8& other) const;

      Posit8 Posit8::operator+(const Posit8& other) const {
        return Posit8(posit8_add(this->value, other.value));
      }

      Posit8 Posit8::operator-(const Posit8& other) const {
        return Posit8(posit8_sub(this->value,  other.value);
      }

      Posit8 Posit8::operator*(const Posit8& other) const {
        return Posit8(posit8_mul(this->value,  other.value);
      }

      Posit8 Posit8::operator/(const Posit8& other) const {
        return Posit8(posit8_div(this->value,  other.value);
      } //*/

}; //*/ end of Class definition

float posit2float(Posit8 p) { // Best by value of reference ?
  boolean sign = false;
  boolean bigNum = false; // 0 between 0 and 1, 1 otherwise
  int8_t bitCount = 5; // posit bit counter
  int8_t exponent = -1; // correct if regime bits are 0. but zero if thet are 1
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  } tempValue ;
  if (p.value ==0) return 0.0f;
  if (p.value ==128) return NAN;
  if (p.value & 128) sign = true; // negative
  if (p.value & 64) {
    bigNum = true; // bit6 set : abs value one or bigger
    exponent++ ;
  }
  while ((p.value & (1 << bitCount)) == (bigNum<<bitCount--)) { // still regime
    bigNum ? exponent ++ : exponent -- ;
    if (bitCount==-1) break; // no exponent or mantissa
  }
  tempValue.tempBytes[3] = exponent + 127;
  //p.value & ((1<<(bitCount+1))-1)
  tempValue.tempBytes[2] = (p.value & ((1<<(bitCount+1))-1))<<(7-bitCount);
  tempValue.tempBytes[1] = 0;
  tempValue.tempBytes[0] = 0;
  tempValue.tempInt >>= 1;
  //while (bitCount-- >= 0)
  if (sign) return -tempValue.tempFloat;
  return tempValue.tempFloat;
}  // end of posit2float 8-bit*/

Posit8 posit_mul(Posit8 a, Posit8 b) { // external function, needed for operator overloading ?
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent=-1,bexponent = -1; 
    uint8_t amantissa=0,bmantissa = 0; 
    int8_t bitCount; // posit bit counter
    int8_t tempResult=0;

    if ((a.value == 0 && b.value != 0x80) || a.value == 0x80) return a; // 0, NaR
    if (b.value == 0 || b.value == 0x80) return b;
    if (a.value == 0x40) return b;
    if (b.value == 0x40) return a;

// Split posit into constituents
    aSign= (a.value & 128);
    if (abigNum = (a.value & 64)) aexponent++; // boolean assignment inside test;
    bitCount = 5;
    //first extract exponent from regime bits
    while ((a.value & (1 << bitCount)) == (abigNum<<bitCount--)) { // still regime
      abigNum ? aexponent ++ : aexponent -- ; 
      if (bitCount==-1) break; // no exponent or mantissa
    }
    // threat exponent bits here if any TODO
    // then treat mantissa bits if any
    amantissa=a.value << (6-bitCount); // all zeros if none exist
    amantissa |=128 ; // add implied one;
    
    bSign= (b.value & 128);
    if (bbigNum = (b.value & 64)) bexponent++;
    bitCount=5; // reinitialize counter
    //first extract exponent from regime bits
    while ((b.value & (1 << bitCount)) == (bbigNum<<bitCount--)) { // still regime
      bbigNum ? bexponent ++ : bexponent -- ; 
      if (bitCount==-1) break; // no exponent or mantissa
    }
    // threat exponent bits here if any TODO
    // then treat mantissa bits if any
    bmantissa=b.value << (6-bitCount);
    bmantissa |=128 ; // add implied one;
    
    // xor signs, add exponents, multiply mantissas
    if (aSign ^ bSign) tempResult = 128;
    int tempExponent=(aexponent+bexponent);
    uint16_t tempMantissa = ((uint16_t)amantissa*bmantissa)>>6; // puts result in LSB and eliminates msb
    sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%02x ", bexponent, bmantissa); Serial.println(s); //*/
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

  // assign sign of result
    while(tempMantissa>511) { // need to test <256=error ?
      tempExponent++; // one more power of two if product carries to bit8+
      tempMantissa>>=1; // eliminate uncoded msb otherwise (same exponent or less)
    }
    while(tempMantissa<256) { // if msb not reached
      tempExponent--; // divide by two
      tempMantissa<<=1; // eliminate msb uncoded
      Serial.println("ERROR : MSB is zero"); //*/
    }
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s); //*/

    bitCount=6; // start of regime
    // Write regimebits
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-- >= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--); 
      }
      bitCount--; // terminating zero
      sprintf(s, "1reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount-- > 0) ; // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      sprintf(s, "0reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    
    if (bitCount > 0) { // still space for exp or mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++))) // from bit7 till end of space
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit8(tempResult);
  }

Posit8 posit_div(Posit8 a, Posit8 b) {
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent=-1,bexponent = -1; 
    uint8_t amantissa=0,bmantissa = 0; 
    int8_t bitCount = 5; // posit bit counter
    int8_t tempResult=0;

  if (b.value == 0x80 || b.value == 0) return Posit8((int8_t)0x80); // NaR
  if (a.value == 0 || a.value == 0x80 || b.value == 0x40) return a;
  
// Move to separate (inline?) function ? Could be useful for all operations
    aSign= (a.value & 128);
    if (abigNum = (a.value & 64)) aexponent++; // boolean assignment inside test;
    //first extract exponent from regime bits
    while ((a.value & (1 << bitCount)) == (abigNum<<bitCount--)) { // still regime
      abigNum ? aexponent ++ : aexponent -- ; // change to 2^es if exponent bits // 1<<(1+ES8)
      if (bitCount==-1) break; // no exponent or mantissa
    }
    // threat exponent bits here if any TODO
    // then treat mantissa bits if any
    amantissa=a.value << (6-bitCount); // all zeros if none exist
    amantissa |=128 ; // add implied one;
    
    bSign= (b.value & 128);
    if (bbigNum = (b.value & 64)) bexponent++;
    bitCount=5; // reinitialize counter
    //first extract exponent from regime bits
    while ((b.value & (1 << bitCount)) == (bbigNum<<bitCount--)) { // still regime
      bbigNum ? bexponent ++ : bexponent -- ; // change to 2^es if exponent bits // 1<<(1+ES8)
      if (bitCount==-1) break; // no exponent or mantissa
    }
    // threat exponent bits here if any TODO
    // then treat mantissa bits if any
    bmantissa=b.value << (6-bitCount);
    bmantissa |=128 ; // add implied one;
    
    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) tempResult = 128;
    int tempExponent=(aexponent-bexponent);
    // first solution to divide mantissas : use float (not efficient)
    union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue ;
    tempValue.tempFloat = ((float)amantissa / (float)bmantissa); // result should be between 0.5 and 2
    Serial.println (tempValue.tempFloat);
    sprintf(s, " fdiv=%08f ", tempValue.tempFloat); Serial.print(s); //*/
    uint8_t tempMantissa = tempValue.tempBytes[2] | 128;
    sprintf(s, "1mant=%02x ", tempMantissa); Serial.print(s); //*/
    // puts result in LSB and eliminate msb
    sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%02x ", bexponent, bmantissa); Serial.println(s); 
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

  // assign sign of result
    while(tempMantissa>511) { 
      Serial.print("MSB is ");Serial.println(tempMantissa >>8);
      tempExponent++; // one more power of two if product carries to bit8+
      tempMantissa>>=1; // eliminate uncoded msb otherwise (same exponent or less)
    }
    while(tempMantissa<256) { 
      tempExponent--; // divide by two
      tempMantissa<<=1; // eliminate msb uncoded
      Serial.println("MSB is zero"); break; //*/
    }
    sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s); //*/

    bitCount=6; // start of regime
    // Write regimebits
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-- >= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--); 
      }
      bitCount--; // terminating zero
      sprintf(s, "1reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount-- > 0) ; // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      sprintf(s, "0reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    Serial.println(bitCount);
    if (bitCount > 0) { // still space for exp or mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++))) 
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit8(tempResult);
  }

class Posit16 {
  public:
    Posit16(uint16_t v = 0) : value(v) {}

    // Add methods for posit16 arithmetic
    Posit16 posit16_add(Posit16 a, Posit16 b);
    Posit16 posit16_sub(Posit16 a, Posit16 b);
    Posit16 posit16_mul(Posit16 a, Posit16 b);
    Posit16 posit16_div(Posit16 a, Posit16 b);
  private:
    uint16_t value;

    // Overload operators
    Posit16 operator+(const Posit16& other) const;
    Posit16 operator-(const Posit16& other) const;
    Posit16 operator*(const Posit16& other) const;
    Posit16 operator/(const Posit16& other) const;
};


Posit16 posit16_add(Posit16 a, Posit16 b);
Posit16 posit16_sub(Posit16 a, Posit16 b);
Posit16 posit16_mul(Posit16 a, Posit16 b);
Posit16 posit16_div(Posit16 a, Posit16 b);


