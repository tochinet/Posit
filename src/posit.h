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
#define ES8 0  // No exponent bits in Posit8. Define as 2 in sketch for standard compliance
#endif
#define ES16 2 // Two exponent bits in Posit16, compliant to standard
#ifndef EPSILON
#define EPSILON 0.000001 // round to zero below 1ppm. Define as 0.0 for standard compliance
#endif

char s[30]; // temporary C string for Serial debug using sprintf
struct splitPosit{
  boolean *sign; // Needs one byte, but compressing inserts lot of code
  boolean *bigNum; 
  int8_t *exponent; 
  uint8_t *mantissa; // maybe 16 bits to share struct ?
};

class Posit8 {
  private:
    //uint8_t value; // maybe in future

  public:
    uint8_t value; // int8_t worked first but issues with nar to nan conversion ?

    Posit8(uint8_t raw = 0) { // Construct from raw unsigned byte
      this->value = raw;
    }
/*#ifdef byte // Exists in Arduino, but not in all C/C++ toolchains
    Posit8(byte raw = 0) { // Construct from raw byte type (unsigned char)
      this->value = (uint8_t)raw;
    }
#endif //*/
    Posit8(double v = 0.0) { // Construct from double by casting to float
      this->value = Posit8((float)v).value; 
    }

    Posit8(int v = 0) { // Construct from int by casting to float for simplicity
      this->value = Posit8((float)v).value; 
    }

    Posit8(float v = 0) { // Construct from float32, IEEE754 format
      union float_int { // for bit manipulation
        float tempFloat; // little endian in AVR8
        uint32_t tempInt; // little-endian as well
        uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
      } tempValue ;

      this->value = 0;
      if (v < 0) {      // negative numbers
        if (v >= -EPSILON) return; // very small neg numbers ~=0, non-standard
        this->value += 0x80; // set sign and continue
      } else { // v>=0
        if (v <= EPSILON) return; // include EPSILON required if EPSILON were zero
      }
      if(isnan(v)) {
        this->value = 0x80; // NaR
        return;
      }
      tempValue.tempFloat = v;
      tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
      int8_t tempExponent = tempValue.tempBytes[3] - 127;
      /*sprintf(s, "exp=%02x mant=%02x%02x%02x", tempExponent, tempValue.tempBytes[2],
        tempValue.tempBytes[1], tempValue.tempBytes[0]); Serial.println(s); //*/

      // separate ES8 lsbs in separate esBits field
      uint8_t esBits=tempExponent & ((1<<ES8)-1); 
      tempExponent>>=ES8;
      //Serial.print (" bC(");Serial.print (bitCount);Serial.print (") ");
      //Serial.print (" es(");Serial.print (esBits);Serial.print (") ");

      // Fill value with result
      int8_t bitCount = 6; // first regime bit
      if (tempExponent >= 0) { // positive exponent, regime bits are 1
        while (tempExponent-- >= 0 && bitCount >= 0) {
          this->value |= (1 << bitCount--); 
        }
        bitCount--; // skip terminating zero bit
      }
      else { // abs(v) < 1, regime bits are zero
        while (tempExponent++ < 0 && bitCount--> 0) ; // do nothing, bits are already zero
        this->value |= (1 << bitCount--); // terminating bit is 1
      }
      //Serial.println(bitCount);
      if (bitCount > 0 && ES8==2) { // still space for exp and/or mantissa, handle exp first
        if (esBits & 2) this->value |= (1 << bitCount);
        bitCount--;
      }
      if (bitCount > 0 && ES8) { // still space for exp lsb
        if (esBits & 1) this->value |= (1 << bitCount);
        bitCount--;
      }
      //Serial.println(bitCount);

      if (bitCount > 0) { // still space for mantissa
        int8_t mantissacount = 1;
        while (bitCount-- >= 0) { //Serial.print(tempValue.tempBytes[2]);
          if (tempValue.tempBytes[2] & (1 << (8 - mantissacount++))) 
            this->value |= (1 << (bitCount+1));
        }
      }
    } // End of constructors

// Helper method to split a posit into constituents
  // passing args by pointer doesn't work for exponent, gets overwritten ???
  static ppositSplit(Posit8 p, boolean *sign, boolean *bigNum, int8_t *exponent, uint8_t *mantissa) {
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
    if(ES8 && bitCount >-1) {
      if (p.value & (1 << bitCount)) exponent += ES8; // add one or two to exponent
      bitCount--;
    }
    if((ES8>1) && bitCount >-1) {
      if (p.value & (1 << bitCount)) exponent += 1; // add one if exp ==3
      bitCount--;
    }
    // then treat mantissa bits if any
    (*mantissa)=p.value << (6-bitCount); // all zeros if none exist
    (*mantissa) |=128 ; // add implied one;
    Serial.print("exponent before "); Serial.println(*exponent); // still OK here
  } //*/ end of positSplit by pointer FAILS

  // arguments by reference 
  static positSplit(Posit8& p, boolean& sign, boolean& bigNum, int8_t& exponent, uint8_t& mantissa) {
    int8_t bitCount; // posit bit counter for regime, exp and mantissa
    //Serial.print("Splitting ");Serial.print(p.value,BIN);Serial.print(" : ");
    sign = (p.value & 128);
    exponent =-(1<<ES8) ; // negative exponents start at -1/-2/-4;
      // Note: "exponent" means power of 2, "exp" means exponent bits outside regime
    if (bigNum = (p.value & 64)) exponent=0; // boolean assignment inside test;
    bitCount=5;
    //if (bigNum) Serial.print(">1. ");else Serial.print("<1. ");

    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((bigNum)<<bitCount--)) { // still regime
      if(bigNum) exponent +=1<<ES8; else exponent -=1<<ES8 ; // add/sub 2^es for each bit in regime
      //Serial.print(exponent);Serial.print(" ");Serial.print(bitCount);Serial.print(", ");
      if (bitCount==-1) break; // all regime, no space for exponent or mantissa
    }
   if(ES8 && (bitCount >-1)) {
      if (p.value & (1 << bitCount)) {
        exponent += ES8; // add one or two to exponent
        //Serial.print ("+ES8 ");
      } 
      bitCount--;
    }
    if((ES8>1) && (bitCount >-1)) {
      if (p.value & (1 << bitCount)) {
        exponent += 1; // add one if exp ==0Bx1
        //Serial.print ("+1 ");
      }
      bitCount--;
    }
    // then treat mantissa bits if any
    mantissa = p.value << (6-bitCount); // all zeros automatically if none exist
    mantissa |= 0x80 ; // add implied one;
    //sprintf(s, "sexp=%02x mant=%02x ", exponent, mantissa); Serial.println(s); //*/
  } // end of positSplit by reference

// Methods for posit8 arithmetic
  static Posit8 posit8_add(Posit8 a, Posit8 b) { // better as externalized function ?
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent,bexponent; 
    uint8_t amantissa,bmantissa,esfield; 
    int8_t bitCount; // posit bit counter
    uint8_t tempResult=0;

    if (a.value == 0x80 || b.value == 0x80) return Posit8((uint8_t)0x80);
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    // Passing arguments by pointer fails for one variable ???
    //ppositSplit(a, &aSign, &abigNum, &aexponent, &amantissa);
    //Serial.print("exponent after "); Serial.println(aexponent); // NOK here, always -1
    positSplit(a, aSign, abigNum, aexponent, amantissa);
    positSplit(b, bSign, bbigNum, bexponent, bmantissa);
    
    // align smaller number
    if (aexponent>bexponent) bmantissa >>= (aexponent-bexponent);
    if (aexponent<bexponent) amantissa >>= (bexponent-aexponent);
    int tempExponent=max(aexponent, bexponent);
    int tempMantissa = amantissa+bmantissa; // Sign untreated here
    if (aSign) tempMantissa -= 2* amantissa;
    if (bSign) tempMantissa -= 2* bmantissa;
    /* Check results */
    /*sprintf(s, "aexp=%02x 1mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x 1mant=%02x ", bexponent, bmantissa); Serial.println(s);
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.print(s); //*/

  // handle sign of result
    if (tempMantissa<0) {
      tempResult |= 0x80;
      tempMantissa = - tempMantissa;
      //sprintf(s, "-"); Serial.print(s);
    }
    if (tempMantissa ==0) return Posit8(0);

    if(tempMantissa>255) tempExponent++; // one more power of two if sum carries to bit8
                else tempMantissa<<=1; // eliminate uncoded msb otherwise (same exponent or less)
    while(tempMantissa<256) { // if msb not reached (substration)
      Serial.print("SmallMant ");
      tempExponent--; // divide by two
      tempMantissa<<=1; // eliminate msb uncoded
    }
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s); //*/

    // TODO move to separate positFill (exponent, mantissa) routine
    // Handle exponent fields
    esfield=tempExponent & ((1<<ES8)-1); 
    tempExponent>>=ES8;
    // Write regimebits
    bitCount=6; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-- >= 0 && bitCount >= 0) { 
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
    
    if (bitCount > 0 && ES8==2) { // still space for exp and/or mantissa, handle exp first
      if (esfield & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0 && ES8) { // still space for exp lsb
      if (esfield & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    
    if (bitCount > 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++))) 
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit8(tempResult);
  }

  static Posit8 posit8_sub(Posit8 a, Posit8 b) {
    if (a.value == 0x80 || b.value == 0x80) return Posit8((uint8_t)0x80);
    if (b.value !=0) b.value=b.value ^ 128; // change sign, except for zero (NaR)
    return posit8_add(a,b); // temp solution to compile
  }

    // Operator overloading for Posit8
        Posit8 operator+(const Posit8& other) const;
        Posit8 operator-(const Posit8& other) const;
        Posit8 operator*(const Posit8& other) ;
        Posit8 operator/(const Posit8& other) ;

      /*Posit8 Posit8::operator+(const Posit8& other) const {
        return posit8_add(this->value, other.value); // doesn't compile
      }

      /*Posit8 Posit8::operator-(const Posit8& other) const {
        return Posit8(posit8_sub(this->value,  other.value);
      } //*/

      Posit8 Posit8::operator*( const Posit8& other) const {
        return posit8_sub(this->value,  other.value); // Compiles but doesn't work
      }

      /*Posit8 Posit8::operator/(const Posit8& other) const {
        return Posit8(posit8_div(this->value,  other.value));
      } //*/

}; //*/ end of Posit8 Class definition

static Posit8 posit8_mul(Posit8 a, Posit8 b) { // external function, needed for operator overloading ?
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent=-1,bexponent = -1; 
    uint8_t amantissa=0,bmantissa = 0, esfield; 
    int8_t bitCount; // posit bit counter
    uint8_t tempResult=0;

    if ((a.value == 0 && b.value != 0x80) || a.value == 0x80) return a; // 0, NaR
    if (b.value == 0 || b.value == 0x80) return b;
    if (a.value == 0x40) return b;
    if (b.value == 0x40) return a;

    // Split posit into constituents
    Posit8::positSplit(a, aSign, abigNum, aexponent, amantissa);
    Posit8::positSplit(b, bSign, bbigNum, bexponent, bmantissa);
       
    // xor signs, add exponents, multiply mantissas
    if (aSign ^ bSign) tempResult = 0x80;
    int tempExponent=(aexponent+bexponent);
    uint16_t tempMantissa = ((uint16_t)amantissa*bmantissa)>>6; // puts result in LSB and eliminates msb
    /*sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%02x ", bexponent, bmantissa); Serial.println(s); 
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

  // normalise mantissa
    while(tempMantissa>511) { // need to test <256=error ?
      tempExponent++; // add power of two if product carried to MSB
      tempMantissa>>=1; // eliminate uncoded msb otherwise (same exponent or less)
      Serial.println("ERROR? : MSB is 2 or more"); //*/
    }
    while(tempMantissa<256) { // if msb not reached, possible?
      tempExponent--; // divide by two
      tempMantissa<<=1; // eliminate msb uncoded
      Serial.println("ERROR : MSB is zero"); //*/
    }
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);

    esfield=tempExponent & ((1<<ES8)-1); 
    tempExponent>>=ES8;

    // Write regimebits
    bitCount=6; // start of regime
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
    
    if (bitCount > 0 && ES8==2) { // still space for exp and/or mantissa, handle exp first
      if (esfield & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0 && ES8) { // still space for exp lsb
      if (esfield & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++))) // from bit7 till end of space
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit8(tempResult);
  } // end of posit8_mul function definition

Posit8 posit8_div(Posit8 a, Posit8 b) {
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent,bexponent; 
    uint8_t amantissa,bmantissa,esfield; 
    int8_t bitCount = 5; // posit bit counter
    uint8_t tempResult=0;

  if (b.value == 0x80 || b.value == 0) return Posit8((uint8_t)0x80); // NaR if /0 or /NaR
  if (a.value == 0 || a.value == 0x80 || b.value == 0x40) return a; // a==0 or NaR, b==1.0
  
    Posit8::positSplit(a, aSign, abigNum, aexponent, amantissa);
    Posit8::positSplit(b, bSign, bbigNum, bexponent, bmantissa);
    
    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) tempResult = 0x80;
    int tempExponent=(aexponent-bexponent);
    // first solution to divide mantissas : use float (not efficient?)
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue ;
    tempValue.tempFloat = ((float)amantissa / (float)bmantissa); // result should be between 0.5 and 2
    //Serial.print (tempValue.tempFloat);

    tempValue.tempInt <<= 1; // eliminate sign, align exponent and mantissa
    uint8_t tempMantissa = tempValue.tempBytes[2]; // eliminate msb
    tempExponent += tempValue.tempBytes[3] - 127;
    //sprintf(s, " mant=%02x ", tempMantissa); Serial.print(s); //*/
    /*sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%02x ", bexponent, bmantissa); Serial.println(s); 
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

    esfield=tempExponent & ((1<<ES8)-1); 
    tempExponent>>=ES8;

    // Write regimebits
    bitCount=6; // start of regime
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
    // Write exp bits
    if (bitCount > 0 && ES8==2) { // still space for exp and/or mantissa, handle exp first
      if (esfield & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0 && ES8) { // still space for exp lsb
      if (esfield & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
     if (bitCount > 0) { // still space for mantissa
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
    Posit16(uint16_t v = 0) : value(v) {} // default constructor, raw from unsigned
    uint16_t value; // unsigned for Posit16, better ?

    Posit16(float v = 0) { // Construct from float32, IEEE754 format
      union float_int { // for bit manipulation
        float tempFloat; // little endian in AVR8
        uint32_t tempInt; // little-endian as well
        uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
      } tempValue ;

      this->value = 0;
      if (v < 0) {      // negative numbers
        //if (v >= -EPSILON) return; // no undeflow for Posit16
        this->value += 128*256; // set sign and continue
      } else { // v>=0
        //if (v <= EPSILON) return; 
      }
      if(isnan(v)) {
        this->value = 0x8000; // NaR
        return;
      }
      tempValue.tempFloat = v;
      tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
      int8_t tempExponent = tempValue.tempBytes[3] - 127;
      sprintf(s, "exp=%02x mant=%02x%02x%02x", tempExponent, tempValue.tempBytes[2],
        tempValue.tempBytes[1], tempValue.tempBytes[0]); Serial.println(s); //*/

      // separate ES8 lsbs in separate es field
      uint8_t esBits=tempExponent & ((1<<ES16)-1); 
      tempExponent>>=ES16;

      int8_t bitCount = 6+8; // first regime bit
      if (tempExponent >= 0) { // positive exponent, regime bits are 1
        while (tempExponent-- >= 0 && bitCount >= 0) {
          this->value |= (1 << bitCount--); 
        }
        bitCount--; // skip terminating zero bit
      }
      else { // abs(v) < 1, regime bits are zero
        while (tempExponent++ < 0 && bitCount--> 0) ; // do nothing, bits are already zero
        this->value |= (1 << bitCount--); // terminating bit is 1
      }
      Serial.print (" bC(");Serial.print (bitCount);Serial.print (") ");
      Serial.print (" es(");Serial.print (esBits);Serial.print (") ");

      //Serial.println(bitCount);
      if (bitCount > 0 && ES16 == 2) { // still space for exp msb
        if (esBits & 2) this->value |= (1 << bitCount);
        bitCount--;
      }
      if (bitCount > 0 && ES16) { // still space for exp lsb
        if (esBits & 1) this->value |= (1 << bitCount);
        bitCount--;
      }
      Serial.println(bitCount);

      if (bitCount > 0) { // still space for mantissa
        int8_t mantissacount = 1;
        int16_t mantissa = (tempValue.tempBytes[2] <<8) + tempValue.tempBytes[1];
        while (bitCount-- >= 0) { 
          if (mantissa & (1 << (16 - mantissacount++))) 
            this->value |= (1 << (bitCount+1));
        }
      }
    } // End of constructors

  static positSplit(Posit16& p, boolean& sign, boolean& bigNum, int8_t& exponent, uint16_t& mantissa) {
    int8_t bitCount; // posit bit counter for regime, exp and mantissa
    Serial.print("Splitting ");Serial.print(p.value,BIN);Serial.print(" : ");
    sign = (p.value & 128*256);
    exponent =-(1<<ES16) ; // negative exponents start at -1/-2/-4;
      // Note: "exponent" means power of 2, "exp" means exponent bits outside regime
    if (bigNum = (p.value & 64*256)) exponent=0; // boolean assignment inside test;
    bitCount=5+8;
    //if (bigNum) Serial.print(">1. ");else Serial.print("<1. ");

    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((bigNum)<<bitCount--)) { // still regime
      if(bigNum) exponent +=1<<ES16; else exponent -=1<<ES16 ; // add/sub 2^es for each bit in regime
      Serial.print(exponent);Serial.print(" ");Serial.print(bitCount);Serial.print(", ");
      if (bitCount==-1) break; // all regime, no space for exponent or mantissa
    }
    // threat exp bits if any, untested
    //Serial.print (" bC(");Serial.print (bitCount);Serial.print (") ");
    //Serial.print (" es(");Serial.print (esBits);Serial.print (") ");
    if(ES16 && (bitCount >-1)) {
      if (p.value & (1 << bitCount)) {
        exponent += ES16; // add one or two to exponent
        Serial.print ("+ES16 ");
      } 
      bitCount--;
    }
    if((ES16>1) && (bitCount >-1)) {
      if (p.value & (1 << bitCount)) {
        exponent += 1; // add one if exp ==0Bx1
        Serial.print ("+1 ");
      }
      bitCount--;
    }
    // then treat mantissa bits if any
    (mantissa)=p.value << (6+8-bitCount); // all zeros automatically if none exist
    (mantissa) |=256*128 ; // add implied one;
    sprintf(s, "sexp=%02x mant=%02x ", exponent, mantissa); Serial.println(s); //*/
  } // end of positSplit for Posit16

    // Add methods for posit16 arithmetic
  static Posit16 posit16_add(Posit16 a, Posit16 b) {
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent,bexponent; 
    uint16_t amantissa,bmantissa;
    uint8_t esfield; 
    int8_t bitCount; // posit bit counter
    uint16_t tempResult=0;

    if (a.value == 0x8000 || b.value == 0x8000) return Posit16((uint16_t)0x8000);
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, abigNum, aexponent, amantissa);
    positSplit(b, bSign, bbigNum, bexponent, bmantissa);
    
    // align smaller number
    if (aexponent>bexponent) bmantissa >>= (aexponent-bexponent);
    if (aexponent<bexponent) amantissa >>= (bexponent-aexponent);
    int tempExponent=max(aexponent, bexponent);
    long tempMantissa = (long)amantissa+bmantissa; // Sign untreated here
    if (aSign) { tempMantissa -= amantissa; tempMantissa -= amantissa; }
    if (bSign) { tempMantissa -= bmantissa; tempMantissa -= bmantissa; }
    /* Check results */
    
    sprintf(s, "aexp=%02x 1mant=%04x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x 1mant=%04x ", bexponent, bmantissa); Serial.println(s);
    sprintf(s, "sexp=%02x 1mant=%06x ", tempExponent, tempMantissa); Serial.print(s); //*/
    Serial.println(tempMantissa); // Still OK here

  // handle sign of result
    if (tempMantissa<0L) {
      tempResult |=32768;
      tempMantissa = - tempMantissa; // 2's complement
      sprintf(s, "-"); Serial.print(s);
    }
    if (tempMantissa ==0) return Posit16((uint16_t)0);

    if(tempMantissa>65535) tempExponent++; // one more power of two if sum carries to bit8
                else tempMantissa<<=1; // eliminate uncoded msb otherwise (same exponent or less)
    while(tempMantissa<(long)65536) { // if msb not reached, ever the case ?
      Serial.print("SmallMant ");
      tempExponent--; // divide by two
      tempMantissa<<=1; 
    }
    sprintf(s, "sexp=%02x mant=%05x ", tempExponent, tempMantissa); Serial.print(s); //*/

    // TODO move to separate positFill (exponent, mantissa) routine
    // Handle exponent fields
    esfield=tempExponent & ((1<<ES16)-1); 
    tempExponent>>=ES16;
    // Write regimebits
    bitCount=14; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-- >= 0 && bitCount >= 0) { 
        tempResult |= (1 << bitCount--); 
      }
      bitCount--; // terminating zero
      sprintf(s, "1reg %04x, bitCount %02x ", tempResult, bitCount); Serial.print(s); //*/
    }
    else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount-- > 0) ; // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      sprintf(s, "0reg %04x, bitCount %02x ", tempResult, bitCount); Serial.print(s); //*/
    }
    
    if (bitCount > 0 && ES16==2) { // still space for exp and/or mantissa, handle exp first
      if (esfield & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0 && ES16) { // still space for exp lsb
      if (esfield & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
      sprintf(s, "es %04x, bitCount %02x ", tempResult, bitCount); Serial.println(s); //*/
    
    if (bitCount > 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if ((unsigned int)tempMantissa & (1 << (16 - mantissacount++))) 
          tempResult |= (1 << (bitCount+1));
      sprintf(s, "ma %04x, bitCount %02x ", tempResult, bitCount); Serial.print(s); //*/
       }
    }
    return Posit16(tempResult);
  }

  static Posit16 posit16_sub(Posit16 a, Posit16 b) {
    if (a.value == 0x8000 || b.value == 0x8000) return Posit16((uint16_t)0x8000);
    if (b.value !=0) b.value=b.value ^ 32768; // change sign, except for zero (NaR)
    return posit16_add(a,b); // temp solution to compile
  }

  Posit16 posit16_mul(Posit16 a, Posit16 b) {
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent,bexponent; 
    uint16_t amantissa,bmantissa;
    uint8_t esfield; 
    int8_t bitCount; // posit bit counter
    uint16_t tempResult;

    if ((a.value == 0 && b.value != 0x8000) || a.value == 0x80) return a; // 0, NaR
    if (b.value == 0 || b.value == 0x8000) return b;
    if (a.value == 0x4000) return b;
    if (b.value == 0x4000) return a;

    // Split posit into constituents
    positSplit(a, aSign, abigNum, aexponent, amantissa);
    positSplit(b, bSign, bbigNum, bexponent, bmantissa);

   // xor signs, add exponents, multiply mantissas
    if (aSign ^ bSign) tempResult = 0x8000;
    int tempExponent=(aexponent+bexponent);
    uint16_t tempMantissa = ((uint16_t)amantissa*bmantissa)>>6; // puts result in LSB and eliminates msb
    /*sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%02x ", bexponent, bmantissa); Serial.println(s); 
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

  // normalise mantissa
    while(tempMantissa>0xFFFFL) { 
      tempExponent++; // add power of two if product carried to MSB
      tempMantissa>>=1; // eliminate uncoded msb otherwise (same exponent or less)
    }
    while(tempMantissa < 0x10000L) { // if msb not reached, possible?
      tempExponent--; // divide by two
      tempMantissa<<=1; // eliminate msb uncoded
      Serial.println("ERROR : MSB is zero"); //*/
    }
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);

    esfield=tempExponent & ((1<<ES16)-1); 
    tempExponent>>=ES16;

    // Write regimebits
    bitCount=14; // start of regime
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
    
    if (bitCount > 0 && ES16==2) { // still space for exp and/or mantissa, handle exp first
      if (esfield & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0 && ES16) { // still space for exp lsb
      if (esfield & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (16 - mantissacount++))) // from bit15 till end of space
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit16(tempResult);
  } // end of posit8_mul function definition

Posit16 posit16_div(Posit16 a, Posit16 b) {
    boolean aSign,bSign;
    boolean abigNum,bbigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aexponent,bexponent; 
    uint16_t amantissa,bmantissa; 
    uint8_t esfield;
    int8_t bitCount = 5; // posit bit counter
    uint16_t tempResult=0;

  if (b.value == 0x8000 || b.value == 0) return Posit16((uint16_t)0x8000); // NaR if /0 or /NaR
  if (a.value == 0 || a.value == 0x8000 || b.value == 0x4000) return a; // a==0 or NaR, b==1.0
  
    positSplit(a, aSign, abigNum, aexponent, amantissa);
    positSplit(b, bSign, bbigNum, bexponent, bmantissa);
    
    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) tempResult = 0x8000;
    int tempExponent=(aexponent-bexponent);
    // first solution to divide mantissas : use float (not efficient)
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue ;
    tempValue.tempFloat = ((float)amantissa / (float)bmantissa); // result should be between 0.5 and 2
    //Serial.print (tempValue.tempFloat);

    tempValue.tempInt <<= 1; // eliminate sign, align exponent and mantissa
    uint8_t tempMantissa = tempValue.tempBytes[2]; // eliminate msb
    tempExponent += tempValue.tempBytes[3] - 127;
    //sprintf(s, " mant=%02x ", tempMantissa); Serial.print(s); //*/
    /*sprintf(s, "aexp=%02x mant=%02x ", aexponent, amantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%02x ", bexponent, bmantissa); Serial.println(s); 
    sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

    esfield=tempExponent & ((1<<ES16)-1); 
    tempExponent>>=ES16;

    // Write regimebits
    bitCount=614; // start of regime
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
    // Write exp bits
    if (bitCount > 0 && ES16==2) { // still space for exp and/or mantissa, handle exp first
      if (esfield & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount > 0 && ES16) { // still space for exp lsb
      if (esfield & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
     if (bitCount > 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-- >= 0) {
        if (tempMantissa & (1 << (16 - mantissacount++))) 
          tempResult |= (1 << (bitCount+1));
      }
    }
    return Posit16(tempResult);
  }

   //### Posit16 posit16_div(Posit16 a, Posit16 b);

  private:
    //uint16_t value; // moved to public since used by posit2float

    // Overload operators
    Posit16 operator+(const Posit16& other) const;
    Posit16 operator-(const Posit16& other) const;
    Posit16 operator*(const Posit16& other) const;
    Posit16 operator/(const Posit16& other) const;
};

/*Posit16 posit16_add(Posit16 a, Posit16 b);
Posit16 posit16_sub(Posit16 a, Posit16 b);
Posit16 posit16_mul(Posit16 a, Posit16 b);
Posit16 posit16_div(Posit16 a, Posit16 b);*/

float posit2float(Posit8 & p) { // Now by reference to save memory
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 5; // posit bit counter
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  } tempValue ;
  // Handle special cases first 
  if (p.value == 0) return 0.0f;
  if (p.value == 0x80) return NAN;
  if (p.value & 0x80) sign = true; // negative
  // handle regime bits
  if (p.value & 0x40) { //bit6 set : abs value one or bigger
    bigNum = true; 
  } else { 
    exponent=-(1<<ES8) ; // negative exponents start at -1/-2/-4
  }
  while ((p.value & (1 << bitCount)) == (bigNum<<bitCount--)) { // regime bits
    bigNum ? exponent +=(1<<ES8) : exponent -=(1<<ES8) ; // add 1, 2 or 4 for each regime bit
    if (bitCount==-1) break; // bit0 is regime, no marker, exponent or mantissa at all in posit
  }
  //handle exponent bits if any
  uint8_t esBits = ((p.value & ((1<<(bitCount+1))-1)) >> (bitCount+1-ES8)) & ((1<<ES8)-1);
  bitCount -= ES8;
  tempValue.tempBytes[3] = exponent + esBits + 127;
  // create 8 bits of mantissa
  tempValue.tempBytes[2] = (p.value & ((1<<(bitCount+1))-1))<<(7-bitCount);
  tempValue.tempBytes[1] = 0;
  tempValue.tempBytes[0] = 0;
  tempValue.tempInt >>= 1; // unsigned shift left for IEEE format
  if (sign) return -tempValue.tempFloat;
  return tempValue.tempFloat;
}  // end of posit2float 8-bit*/

float posit2float(Posit16 p) { // temporary, non-functional yet but compiles
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 5+8; // posit bit counter
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  } tempValue ;
  // Handle special cases first 
  if (p.value == 0) return 0.0f;
  if (p.value == 0x8000) return NAN;
  if (p.value & 0x8000) sign = true; // negative
  if (p.value & 0x4000) {
    bigNum = true; // bit14 set : abs value one or bigger
   } else { 
    exponent=-(1<<ES16) ; // negative exponents start at -1/-2/-4
  }
  while ((p.value & (1 << bitCount)) == (bigNum<<bitCount--)) { // still regime
    bigNum ? exponent +=(1<<ES16) : exponent -=(1<<ES16) ;
    if (bitCount==-1) break; // no exponent or mantissa at all in posit
  }
  //handle exponent bits if any
  uint8_t esBits = ((p.value & ((1<<(bitCount+1))-1)) >> (bitCount+1-ES16)) & ((1<<ES16)-1);
  bitCount -= ES16;
  /*Serial.print ("bC(");Serial.print (bitCount);Serial.print (") ");
  Serial.print ("es(");Serial.print (esBits);Serial.print (") ");
  Serial.print (((1<<(bitCount+1))+(long)(p.value & ((1<<(bitCount+1))-1))<<(15-bitCount))/65536.0,8);
  Serial.print ("x2^");Serial.print (exponent+esBits);Serial.print (" "); //*/
  
  tempValue.tempBytes[3] = exponent + esBits + 127;
  uint16_t mantissa= (p.value & ((1<<(bitCount+1))-1))<<(15-bitCount);
  tempValue.tempBytes[2] = mantissa >> 8 ;
  tempValue.tempBytes[1] = mantissa ;
  tempValue.tempBytes[0] = 0;
  tempValue.tempInt >>= 1; // unsigned shift left for IEEE format
  if (sign) return -tempValue.tempFloat;
  return tempValue.tempFloat;
}  // end of posit2float 16-bit*/

