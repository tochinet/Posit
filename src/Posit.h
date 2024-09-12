/*************************************************************************************

  Posit Library for Arduino

  This library provides posit8 and posit16 floating point arithmetic
  support in the Arduino environment, especially the memory-limited 8-bit AVR boards.
  Posits are a more efficient and more precise alternative to IEEE754 floats.

  Major design goals are :
  - Small size, both in program memory and RAM usage
  - Simplicity, restricting the library to 8 and 16 bits posits and +-/* operations
  - Useful, explanatory comments in the library

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

  See https://github.com/tochinet/Posit/ for more details on Posits and the library.
************************************************************************************/

#ifndef ES8
#define ES8 0 // No exponent bits in Posit8. Define as 2 in sketch for standard compliance
#endif
#define ES16 2 // Two exponent bits in Posit16, compliant to standard
#ifndef EPSILON
#define EPSILON 0.000001 // round to zero below 1ppm. Define as 0.0 for standard compliance
#endif

char s[30]; // temporary C string for Serial debug using sprintf
struct splitPosit {
  boolean * sign; // Needs one byte, but compressing inserts lot of code
  boolean * bigNum;
  int8_t * exponent;
  uint8_t * mantissa; // maybe 16 bits to share struct between 8 and 16 bits?
};

class Posit8 {
  private:
  //uint8_t value; // maybe in future

  public: uint8_t value; // first used int8_t, but issues with nar to nan conversion

  Posit8(uint8_t raw = 0) { // Construct from raw unsigned byte
    this -> value = raw;
  }
  
  #ifdef byte // Exists in Arduino, but not in all C/C++ toolchains
      Posit8(byte raw = 0) { // Construct from raw byte type (unsigned char)
        this->value = (uint8_t)raw;
      }
  #endif //*/
  
  Posit8(float v = 0) { // Construct from float32, IEEE754 format
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    }
    tempValue;

    this->value = 0;
    if (v < 0) { // negative numbers
      if (v >= -EPSILON) return; // very small neg numbers ~=0, non-standard
      this -> value += 0x80; // set sign and continue
    } else { // v>=0
      if (v <= EPSILON) return; // include EPSILON required if EPSILON were zero
    }
    if (isnan(v)) {
      this->value = 0x80; // NaR
      return;
    }
    tempValue.tempFloat = v;
    tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
    int8_t tempExponent = tempValue.tempBytes[3] - 127;
    uint8_t esBits = tempExponent & ((1 << ES8) - 1);
    tempExponent >>= ES8;

    // Fill value with result
    int8_t bitCount = 6; // first regime bit
    if (tempExponent >= 0) { // abs(v) >= 1, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        this->value |= (1 << bitCount--);
      }
      bitCount--; // skip terminating (zero) bit
    } else { // abs(v) < 1, regime bits are zero
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing, bits are already zero
      this->value |= (1 << bitCount--); // set terminating bit as 1
    }

    if (bitCount >= 0 && ES8 == 2) { // still space for exp and/or mantissa, handle exp first
      if (esBits & 2) this->value |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES8) { // still space for exp lsb
      if (esBits & 1) this->value |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount>= 0) {
        if (tempValue.tempBytes[2] & (1 << (8 - mantissacount++)))
          this->value |= (1 << bitCount);
        bitCount--;
      }
    }
  } 

  Posit8(double v = 0.0) { // Construct from double by casting to float
    this -> value = Posit8((float) v).value;
  }

  Posit8(int v = 0) { // Construct from int by casting to float for simplicity
    this->value = Posit8((float) v).value;
  }
  // End of constructors

  // Helper method to split a posit into constituents
  // arguments by reference to avoid copy
  static positSplit(Posit8 & p, boolean & sign, boolean & bigNum, int8_t & exponent, uint8_t & mantissa) {
    int8_t bitCount; // posit bit counter for regime, exp and mantissa
 
    sign = (p.value & 128);
    exponent = -(1 << ES8); // negative exponents start at -1/-2/-4;
    // Note: "exponent" means power of 2, "exp" means exponent bits outside regime
    if (bigNum = (p.value & 64)) exponent = 0; // boolean assignment inside test;
    bitCount = 5; // starts at second regime bit

    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((bigNum) << bitCount--)) { // still regime
      if (bigNum) exponent += 1 << ES8;
      else exponent -= 1 << ES8; // add/sub 2^es for each bit in regime
      if (bitCount == -1) break; // regime fills all bits, no space for exponent or mantissa
    }
    // then extract exponent lsb(s) from esBits
    if (ES8 && (bitCount > -1)) {
      if (p.value & (1 << bitCount)) {
        exponent += ES8; // add one or two to exponent
      }
      bitCount--;
    }
    if ((ES8 > 1) && (bitCount > -1)) {
      if (p.value & (1 << bitCount)) {
        exponent += 1; // add one if exp ==0Bx1
      }
      bitCount--;
    }
    // then treat mantissa bits if any
    mantissa = p.value << (6 - bitCount); // all zeros automatically if none exist
    mantissa |= 0x80; // add implied one;
  } // end of positSplit

  // Methods for posit8 arithmetic
  static Posit8 posit8_add(Posit8 a, Posit8 b) { // better as externalized function ?
    boolean aSign, bSign;
    boolean aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent, bExponent;
    uint8_t aMantissa, bMantissa, esBits;
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if (a.value == 0x80 || b.value == 0x80) return Posit8((uint8_t) 0x80); // NaR
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, aBigNum, aExponent, aMantissa);
    positSplit(b, bSign, bBigNum, bExponent, bMantissa);

    // align smaller number
    if (aExponent > bExponent) bMantissa >>= (aExponent - bExponent);
    if (aExponent < bExponent) aMantissa >>= (bExponent - aExponent);
    int tempExponent = max(aExponent, bExponent);
    int tempMantissa = aMantissa + bMantissa; // Sign not treated here
    if (aSign) tempMantissa -= 2 * aMantissa;
    if (bSign) tempMantissa -= 2 * bMantissa;

    // handle sign of result of mantissa addition
    if (tempMantissa < 0) {
      tempResult |= 0x80;
      tempMantissa = -tempMantissa; // No 2's complement in Posits
    }
    if (tempMantissa == 0) return Posit8(0);

    if (tempMantissa > 255) tempExponent++; // add one power of two if sum carries to bit8
    else tempMantissa <<= 1; // eliminate uncoded msb otherwise (same exponent or less)
    while (tempMantissa < 256) { // if msb not reached (substration)
      //Serial.print("SmallMant ");
      tempExponent--; // divide by two
      tempMantissa <<= 1; // eliminate msb uncoded
    }

    // TODO move to separate positFill (sign, exponent, mantissa) routine
    // Extract exponent lsb(s) for exponent field
    esBits = tempExponent & ((1 << ES8) - 1);
    tempExponent >>= ES8;
    // Write regimebits
    bitCount = 6; // start of regime
    if (tempExponent >= 0) { // abs(v)>=1, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // skip terminating zero
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing, regime bits are zero
      tempResult |= (1 << bitCount--); // set terminating 1
    }

    if (bitCount >= 0 && ES8 == 2) { // still space for esBits
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES8) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++)))
          tempResult |= (1 << (bitCount + 1));
      }
    }
    return Posit8(tempResult);
  } // end of posit8_add function definition

  static Posit8 posit8_sub(Posit8 a, Posit8 b) {
    if (a.value == 0x80 || b.value == 0x80) return Posit8((uint8_t) 0x80);
    if (b.value != 0) b.value = b.value ^ 128; // change sign, except for zero (NaR)
    return posit8_add(a, b); // temp solution to compile
  }

  static Posit8 posit8_mul(Posit8 a, Posit8 b) {
    boolean aSign, bSign;
    boolean aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent = -1, bExponent = -1;
    uint8_t aMantissa = 0, bMantissa = 0, esBits;
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if ((a.value == 0 && b.value != 0x80) || a.value == 0x80) return a; // 0, /0, NaR
    if (b.value == 0 || b.value == 0x80) return b; // 0, NaR
    if (a.value == 0x40) return b; // 1*b
    if (b.value == 0x40) return a; // a*1

    // Split posit into constituents
    positSplit(a, aSign, aBigNum, aExponent, aMantissa);
    positSplit(b, bSign, bBigNum, bExponent, bMantissa);

    // xor signs, add exponents, multiply mantissas
    if (aSign ^ bSign) tempResult = 0x80;
    int tempExponent = (aExponent + bExponent);
    uint16_t tempMantissa = ((uint16_t) aMantissa * bMantissa) >> 6; // puts result in LSB and eliminates msb

    // normalise mantissa
    while (tempMantissa > 511) { 
      tempExponent++; // add power of two if product carried to MSB
      tempMantissa >>= 1; // move towards bit8-alignment
      //Serial.println("OK? : MSB is 2 or more"); //*/
    }
    while (tempMantissa < 256) { // if msb not reached, possible?
      tempExponent--; // divide by two
      tempMantissa <<= 1; // eliminate msb uncoded
      //Serial.println("ERROR? : MSB is zero"); //*/
    }
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);

    esBits = tempExponent & ((1 << ES8) - 1);
    tempExponent >>= ES8;

    // Write regimebits
    bitCount = 6; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
      //sprintf(s, "1reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      //sprintf(s, "0reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }

    if (bitCount >= 0 && ES8 == 2) { // still space for exp and/or mantissa, handle exp first
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES8) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++))) // from bit7 till end of space
          tempResult |= (1 << (bitCount + 1));
      }
    }
    return Posit8(tempResult);
  } // end of posit8_mul function definition

  static Posit8 posit8_div(Posit8 a, Posit8 b) {
    boolean aSign, bSign;
    boolean aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent, bExponent;
    uint8_t aMantissa, bMantissa, esBits;
    int8_t bitCount = 5; // posit bit counter
    uint8_t tempResult = 0;

    if (b.value == 0x80 || b.value == 0) return Posit8((uint8_t) 0x80); // NaR if /0 or /NaR
    if (a.value == 0 || a.value == 0x80 || b.value == 0x40) return a; // a==0 or NaR, b==1.0

    Posit8::positSplit(a, aSign, aBigNum, aExponent, aMantissa);
    Posit8::positSplit(b, bSign, bBigNum, bExponent, bMantissa);

    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) tempResult = 0x80;
    int tempExponent = (aExponent - bExponent);
    // first solution to divide mantissas : use float (not efficient?)
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    }
    tempValue;
    tempValue.tempFloat = ((float) aMantissa / (float) bMantissa); // result should be between 0.5 and 2
    //Serial.print (tempValue.tempFloat);

    tempValue.tempInt <<= 1; // eliminate sign, align exponent and mantissa
    uint8_t tempMantissa = tempValue.tempBytes[2]; // eliminate msb
    tempExponent += tempValue.tempBytes[3] - 127;
    //sprintf(s, " mant=%02x ", tempMantissa); Serial.print(s); //*/
    /*sprintf(s, "aexp=%02x mant=%02x ", aExponent, aMantissa); Serial.print(s);
      sprintf(s, "bexp=%02x mant=%02x ", bExponent, bMantissa); Serial.println(s);
      sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/

    esBits = tempExponent & ((1 << ES8) - 1);
    tempExponent >>= ES8;

    // Write regimebits
    bitCount = 6; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
      //sprintf(s, "1reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      //sprintf(s, "0reg %02x, bitCount %02 ", tempResult, bitCount); Serial.print(s); //*/
    }
    // Write exp bits
    if (bitCount >= 0 && ES8 == 2) { // still space for exp and/or mantissa, handle exp first
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES8) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++)))
          tempResult |= (1 << (bitCount + 1));
      }
    }
    return Posit8(tempResult);
  }

  // Operator overloading for Posit8
  Posit8 Posit8::operator + (const Posit8 & other) const {
    return posit8_add( * this, other);
  }
  Posit8 Posit8::operator - (const Posit8 & other) const {
    return posit8_sub( * this, other);
  }
  Posit8 Posit8::operator * (const Posit8 & other) const {
    return posit8_mul( * this, other);
  }
  Posit8 Posit8::operator / (const Posit8 & other) const {
    return posit8_div( * this, other);
  } //*/
}; // end of Posit8 Class definition

class Posit16 {
  private:
    //uint16_t value; // moved to public since used by posit2float

  public: Posit16(uint16_t v = 0): value(v) {} // default constructor, raw from unsigned
    uint16_t value;

  Posit16(float v = 0) { // Construct from float32, IEEE754 format
    union float_int { // for bit manipulation
      float tempFloat;
      uint32_t tempInt; // little-endian in AVR
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;

    this->value = 0;
    if (v < 0) { // negative numbers
      if (v >= -EPSILON * EPSILON) return; // lower underflow limit for Posit16
      this->value += 0x8000; // set sign and continue
    } else { // v>=0
      if (v <= EPSILON * EPSILON) return;
    }
    if (isnan(v)) {
      this->value = 0x8000; // NaR
      return;
    }

    tempValue.tempFloat = v;
    tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
    int8_t tempExponent = tempValue.tempBytes[3] - 127;

    // separate ES8 lsbs in separate es field
    uint8_t esBits = tempExponent & ((1 << ES16) - 1);
    tempExponent >>= ES16;

    int8_t bitCount = 14; // first regime bit
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        this->value |= (1 << bitCount--);
      }
      bitCount--; // skip terminating zero bit
    } else { // abs(v) < 1, regime bits are zero
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing, bits are already zero
      this->value |= (1 << bitCount--); // set terminating bit as 1
    }

    if (bitCount >= 0 && ES16 == 2) { // still space for exp msb
      if (esBits & 2) this -> value |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES16) { // still space for exp lsb
      if (esBits & 1) this -> value |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      int16_t mantissa = (tempValue.tempBytes[2] << 8) + tempValue.tempBytes[1];
      while (bitCount-->= 0) {
        if (mantissa & (1 << (16 - mantissacount++)))
          this -> value |= (1 << (bitCount + 1));
      }
    }
  }

  Posit16(double v = 0) { // Construct from double by casting to float32
    this -> value = Posit16((float)v).value;
  }

  Posit16(int v = 0) { // Construct from int by casting to float for simplicity
    this -> value = Posit16((float)v).value;
  }
  // End of constructors

  static positSplit(Posit16 & p, boolean & sign, boolean & bigNum, int8_t & exponent, uint16_t & mantissa) {
    int8_t bitCount; // posit bit counter for regime, exp and mantissa
    sign = (p.value & 0x8000);
    exponent = -(1 << ES16); // negative exponents start at -1/-2/-4;
    // Note: "exponent" means power of 2, "exp" means exponent bits outside regime
    if (bigNum = (p.value & 64 * 256)) exponent = 0; // boolean assignment inside test;
    bitCount = 5 + 8;

    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((bigNum) << bitCount--)) { // still regime
      if (bigNum) exponent += 1 << ES16;
      else exponent -= 1 << ES16; // add/sub 2^es for each bit in regime
      if (bitCount == -1) break; // all regime, no space for exponent or mantissa
    }
    // threat exp bits if any, untested
    if (ES16 && (bitCount > -1)) {
      if (p.value & (1 << bitCount)) {
        exponent += ES16; // add one or two to exponent
      }
      bitCount--;
    }
    if ((ES16 > 1) && (bitCount > -1)) {
      if (p.value & (1 << bitCount)) {
        exponent += 1; // add one if exp ==0Bx1
      }
      bitCount--;
    }
    // then treat mantissa bits if any
    (mantissa) = p.value << (14 - bitCount); // all zeros automatically if none exist
    (mantissa) |= 0x8000; // add implied one;
 } // end of positSplit for Posit16

  // Add methods for posit16 arithmetic
  static Posit16 posit16_add(Posit16 a, Posit16 b) {
    boolean aSign, bSign;
    boolean aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent, bExponent;
    uint16_t aMantissa, bMantissa;
    uint8_t esBits;
    int8_t bitCount; // posit bit counter
    uint16_t tempResult = 0;

    if (a.value == 0x8000 || b.value == 0x8000) return Posit16((uint16_t) 0x8000);
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, aBigNum, aExponent, aMantissa);
    positSplit(b, bSign, bBigNum, bExponent, bMantissa);

    // align smaller number
    if (aExponent > bExponent) bMantissa >>= (aExponent - bExponent);
    if (aExponent < bExponent) aMantissa >>= (bExponent - aExponent);
    int tempExponent = max(aExponent, bExponent);
    long tempMantissa = (long) aMantissa + bMantissa; // Sign untreated here
    if (aSign) {
      tempMantissa -= aMantissa;
      tempMantissa -= aMantissa;
    }
    if (bSign) {
      tempMantissa -= bMantissa;
      tempMantissa -= bMantissa;
    }

    // treat sign of result
    if (tempMantissa < 0) {
      tempResult |= 0x8000;
      tempMantissa = -tempMantissa; // no 2's complement in Posit
    }
    if (tempMantissa == 0) return Posit16((uint16_t) 0);

    if (tempMantissa > 0xFFFFL) tempExponent++; // one more power of two if sum carries to bit8
    else tempMantissa <<= 1; // eliminate uncoded msb otherwise (same exponent or less)
    while (tempMantissa < (long) 65536) { // if msb not reached, ever the case ?
      //Serial.print("SmallMant ");
      tempExponent--; // divide by two
      tempMantissa <<= 1;
    }
    //sprintf(s, "sexp=%02x mant=%05x ", tempExponent, tempMantissa); Serial.print(s); //*/

    // TODO move to separate positFill (exponent, mantissa) routine
    // Handle exponent fields
    esBits = tempExponent & ((1 << ES16) - 1);
    tempExponent >>= ES16;
    // Write regimebits
    bitCount = 14; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
      //sprintf(s, "1reg %04x, bitCount %02x ", tempResult, bitCount); Serial.print(s); //*/
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
      //sprintf(s, "0reg %04x, bitCount %02x ", tempResult, bitCount); Serial.print(s); //*/
    }

    if (bitCount >= 0 && ES16 == 2) { // still space for exp and/or mantissa, handle exp first
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES16) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if ((unsigned int) tempMantissa & (1 << (16 - mantissacount++)))
          tempResult |= (1 << (bitCount + 1));
      }
    }
    return Posit16(tempResult);
  }

  static Posit16 posit16_sub(Posit16 a, Posit16 b) {
    if (a.value == 0x8000 || b.value == 0x8000) return Posit16((uint16_t) 0x8000);
    if (b.value != 0) b.value = b.value ^ 0x8000; // change sign, except for zero (NaR)
    return posit16_add(a, b);
  }

  static Posit16 posit16_mul(Posit16 a, Posit16 b) {
    boolean aSign, bSign;
    boolean aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent, bExponent;
    uint16_t aMantissa, bMantissa;
    uint8_t esBits;
    int8_t bitCount; // posit bit counter
    uint16_t tempResult = 0;

    if ((a.value == 0 && b.value != 0x8000) || a.value == 0x8000) return a; // 0, NaR
    if (b.value == 0 || b.value == 0x8000) return b;
    if (a.value == 0x4000) return b;
    if (b.value == 0x4000) return a;

    // Split posit into constituents
    positSplit(a, aSign, aBigNum, aExponent, aMantissa);
    positSplit(b, bSign, bBigNum, bExponent, bMantissa);

    // xor signs, add exponents, multiply mantissas
    if (aSign ^ bSign) tempResult = 0x8000;
    int tempExponent = (aExponent + bExponent);
    uint32_t tempMantissa = ((uint32_t) aMantissa * bMantissa) >> 14; // puts result in lower 16 bits and eliminates msb
    /*sprintf(s, "aexp=%02x mant=%04x ", aExponent, aMantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%04x ", bExponent, bMantissa); Serial.println(s);
    sprintf(s, "mexp=%02x mant=%06x ", tempExponent, (uint32_t)tempMantissa); Serial.println(s); //*/

    // normalise mantissa
    if (tempMantissa > 0x1FFFFL) {
      tempExponent++; // add power of two if product carried to MSB
      tempMantissa >>= 1; // eliminate uncoded msb otherwise (same exponent or less)
    }
    while (tempMantissa < 0x10000L) { // if msb not reached, possible?
      tempExponent--; // divide by two
      tempMantissa <<= 1; // eliminate msb uncoded
      Serial.println("ERROR? : MSB is zero");
    } //*/

    esBits = tempExponent & ((1 << ES16) - 1);
    tempExponent >>= ES16;

    // Write regime field
    bitCount = 14; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
    }
    // Write exponent field
    if (bitCount >= 0 && ES16 == 2) { // still space for exp and/or mantissa, handle exp first
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES16) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    // Write mantissa field
    if (bitCount >= 0) { // still space for mantissa
      uint8_t mantissaCount = 1;
      while (bitCount >= 0) {
        if (tempMantissa & (uint16_t)(1 << (16 - mantissaCount++))) { // from bit15 till end of space
          tempResult |= (1 << bitCount);
        }
        bitCount--;
      }
    }
    return Posit16(tempResult);
  } // end of posit8_mul function definition

  static Posit16 posit16_div(Posit16 a, Posit16 b) {
    boolean aSign, bSign;
    boolean aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent, bExponent;
    uint16_t aMantissa, bMantissa;
    uint8_t esBits;
    int8_t bitCount = 5; // posit bit counter
    uint16_t tempResult = 0;

    if (b.value == 0x8000 || b.value == 0) return Posit16((uint16_t) 0x8000); // NaR if /0 or /NaR
    if (a.value == 0 || a.value == 0x8000 || b.value == 0x4000) return a; // a==0 or NaR, b==1.0

    positSplit(a, aSign, aBigNum, aExponent, aMantissa);
    positSplit(b, bSign, bBigNum, bExponent, bMantissa);

    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) tempResult = 0x8000;
    int tempExponent = (aExponent - bExponent);
    // first solution to divide mantissas : use float (not efficient)
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;

    tempValue.tempFloat = ((float) aMantissa / (float) bMantissa); // result should be between 0.5 and 2

    tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
    uint16_t tempMantissa = tempValue.tempBytes[2] * 256 + tempValue.tempBytes[1]; // eliminate msb
    tempExponent += tempValue.tempBytes[3] - 127;

    esBits = tempExponent & ((1 << ES16) - 1);
    tempExponent >>= ES16;

    // Write regimebits
    bitCount = 14; // start of regime
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) { // es=0 assumed
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
    }

    if (bitCount >= 0 && ES16 == 2) { // still space for exp and/or mantissa, handle exp first
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES16) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if (tempMantissa & (1 << (16 - mantissacount++)))
          tempResult |= (1 << (bitCount + 1));
      }
    }
    return Posit16(tempResult);
  }

  // Operator overloading for Posit16
  Posit16 operator + (const Posit16 & other) const {
    return posit16_add( * this, other);
  }
  Posit16 Posit16::operator - (const Posit16 & other) const {
    return posit16_sub( * this, other);
  }
  Posit16 Posit16::operator * (const Posit16 & other) const {
    return posit16_mul( * this, other);
  }
  Posit16 Posit16::operator / (const Posit16 & other) const {
    return posit16_div( * this, other);
  } //*/
};

float posit2float(Posit8 & p) {
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 5; // posit bit counter
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  }
  tempValue;
  // Handle special cases first
  if (p.value == 0) return 0.0f;
  if (p.value == 0x80) return NAN;
  if (p.value & 0x80) sign = true; // negative
  // handle regime bits
  if (p.value & 0x40) { //bit6 set : abs value one or bigger
    bigNum = true;
  } else {
    exponent = -(1 << ES8); // negative exponents start at -1/-2/-4
  }
  while ((p.value & (1 << bitCount)) == (bigNum << bitCount--)) { // regime bits
    bigNum ? exponent += (1 << ES8) : exponent -= (1 << ES8); // add 1, 2 or 4 for each regime bit
    if (bitCount == -1) break; // bit0 is regime, no marker, exponent or mantissa at all in posit
  }
  //handle exponent bits if any
  uint8_t esBits = ((p.value & ((1 << (bitCount + 1)) - 1)) >> (bitCount + 1 - ES8)) & ((1 << ES8) - 1);
  bitCount -= ES8;

  tempValue.tempBytes[3] = exponent + esBits + 127;
  // create 8 bits of mantissa
  tempValue.tempBytes[2] = (p.value & ((1 << (bitCount + 1)) - 1)) << (7 - bitCount);
  tempValue.tempBytes[1] = 0;
  tempValue.tempBytes[0] = 0;
  tempValue.tempInt >>= 1; // unsigned shift left for IEEE format
  if (sign) return -tempValue.tempFloat;
  return tempValue.tempFloat;
} // end of posit2float 8-bit*/

float posit2float(Posit16 p) {
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 5 + 8; // posit bit counter
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  }
  tempValue;
  // Handle special cases first
  if (p.value == 0) return 0.0f;
  if (p.value == 0x8000) return NAN;
  if (p.value & 0x8000) sign = true; // negative
  if (p.value & 0x4000) {
    bigNum = true; // bit14 set : abs value one or bigger
  } else {
    exponent = -(1 << ES16); // negative exponents start at -1/-2/-4
  }
  while ((p.value & (1 << bitCount)) == (bigNum << bitCount--)) { // still regime
    bigNum ? exponent += (1 << ES16) : exponent -= (1 << ES16);
    if (bitCount == -1) break; // no exponent or mantissa at all in posit
  }
  //handle exponent bits if any
  uint8_t esBits = ((p.value & ((1 << (bitCount + 1)) - 1)) >> (bitCount + 1 - ES16)) & ((1 << ES16) - 1);
  bitCount -= ES16;

  tempValue.tempBytes[3] = exponent + esBits + 127;
  uint16_t mantissa = (p.value & ((1 << (bitCount + 1)) - 1)) << (15 - bitCount);
  tempValue.tempBytes[2] = mantissa >> 8;
  tempValue.tempBytes[1] = mantissa; // implied & 0xFF, LSB if any
  tempValue.tempBytes[0] = 0;
  tempValue.tempInt >>= 1; // unsigned shift left for IEEE format
  if (sign) return -tempValue.tempFloat;
  return tempValue.tempFloat;
} // end of posit2float 16-bit*/
