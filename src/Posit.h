/*************************************************************************************

  Posit Library for Arduino
  
  This library provides posit8 and posit16 floating point arithmetic support
  for the Arduino environment, especially the memory-limited 8-bit AVR boards.
  Posits are a more efficient and more precise alternative to IEEE754 floats.

  Major design goals are :
  - Small size, both in code/program memory and RAM usage
  - Simplicity, restricting the library to 8 and 16 bits posits and most common operations
  - Useful, explanatory comments in the library

  As corollary, major non-goals are :
  - Support for 32bits posits (not enough added value compared with usual floats)
  - Full compliance with the Posit standard (2022)
  - Complex rounding algorithms (rounding to nearest even requires handling G,R and S bits)
  - Support of quire (very long accumulator)

  CURRENT STATUS: Restarted from release 0.1.0 to 
  1°) handle 2's complement globally
  2°) unify the code into the 16-bit version
      This requires equality of ES8 and ES16
  3°) include some rounding in the Posit8 class
  At the moment, 2's complement is active in Posit16 and 4 ops work again
  Code for sqrt, next and prior added back and tested.
  Posit8 2's complement still WIP. constructors work, most operations fail

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

#ifndef EPSILON          // Define EPSILON as 0.0 in sketch for standard compliance
#define EPSILON 0.000001 // results smaller than EPSILON are rounded to zero. 
#endif
#ifndef ES8   // Define ES8 in sketch for other values
#define ES8 2 // Two exponent bits in Posit8 by default (standard).
#endif
#define ES16 2 // Always two exponent bits in Posit16

#if ES8 != ES16
#define NOCAST // different exponent formats needs separate routines
#endif

#ifndef NODEBUG
char s[30]; // temporary C string for Serial debug using sprintf
#endif

struct splitPosit { // Not in use (yet)
  boolean sign;
  int8_t exponent; // 2's power, both from/to regime and exp fields
  uint16_t mantissa; // worth using 16 bits to share struct between 8 and 16 bits?
}; 

class Posit8; // Forward-declared for casting from Posit8 to Posit16

class Posit16 {
  private:
  //uint16_t value; // moved to public since used by posit2float

  public: 
  uint16_t value;

  Posit16(uint16_t v = 0): value(v) {} // default constructor, raw from unsigned

  // construc from parts (sign exponent and mantissa without leading 1)
  Posit16(bool& sign, int8_t tempExponent, uint16_t& tempMantissa) {
    // tempExponent by value, will be modified
    // sign and mantissa by reference, won't be modified.
    //Serial.print(sign?"-1.":"+1.");Serial.print(tempMantissa,BIN); // bug, missing leading zeros
    //Serial.print("x2^");Serial.println(tempExponent);

    int8_t bitCount = 14; // start of regime
    uint16_t tempResult = 0; // better than this->value ?

    // Extract exponent lsb(s) for exponent field
    uint8_t esBits = tempExponent & ((1 << ES16) - 1);
    tempExponent >>= ES16;
    if (tempExponent >= 0) { // positive exponent, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
    } else { // abs(v) < 1
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing
      tempResult |= (1 << bitCount--); // terminating 1
    }
    if (bitCount >= 0 && ES16 == 2) { // still space for exp field
      if (esBits & 2) tempResult |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES16) { // still space for exp lsb
      if (esBits & 1) tempResult |= (1 << bitCount);
      bitCount--;
    }
    //if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if (tempMantissa & (1 << (16 - mantissacount++)))
          tempResult |= (1 << (bitCount + 1));
      //}
    }
    this->value=sign?~tempResult+1:tempResult;
  } // end of posit16 constructor from parts

  Posit16(float v = 0) { // Construct from float32, IEEE754 format
    union float_int { // for bit manipulation
      float tempFloat;
      uint32_t tempInt; // little-endian in AVR
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;
    bool sign = 0;

    this->value = 0;
    if (v < 0) { // negative numbers
      if (v >= -EPSILON * EPSILON) return; // lower underflow limit for Posit16
      sign = 1; // set sign and continue
    } else { // v>=0
      if (v <= EPSILON * EPSILON) return;
    }
    if (isnan(v)) {
      this->value = 0x8000; // NaR
      return;
    }

    tempValue.tempFloat = v;
    tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
    int8_t exponent = tempValue.tempBytes[3] - 127;
    uint16_t mantissa = (tempValue.tempBytes[2] << 8) + tempValue.tempBytes[1];

    this->value = Posit16(sign, exponent, mantissa).value;
  }

  Posit16(double v = 0) { // Construct from double by casting to float32
    this -> value = Posit16((float)v).value;
  }

  Posit16(int v = 0) { // Construct from int by casting to float for simplicity
    this -> value = Posit16((float)v).value;
  }
#ifndef NOCAST 
  Posit16(Posit8) ; // forward declaration
#endif 
  // End of constructors

  static void positSplit(Posit16 p, bool& sign, int8_t& exponent, uint16_t& mantissa) {
    // Reads Posit16 by value, writes sign, exponent and mantissa (without leading 1)
    bool bigNum; // true for abs(p) >= 1
    int8_t bitCount= 5+8; // posit bit counter for regime, exp and mantissa

    sign = (p.value & 0x8000);
    if (sign) p.value = -p.value;
    exponent = -(1 << ES16); // negative exponents start at -1/-2/-4;
    // Note: don't confuse "exponent" meaning power of 2 with the exp field (exponent lsbs outside regime)
    bigNum = (p.value >= 0x4000 && p.value < 0xC000);
    if (bigNum) exponent = 0; // positive exponents start at 0

    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((bigNum) << bitCount--)) { // still regime
      if (bigNum) exponent += 1 << ES16;
      else exponent -= 1 << ES16; // add/sub 2^es for each bit in regime
      if (bitCount == -1) break; // all regime, no space for exponent or mantissa
    }
    // threat exp bits if any
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
    mantissa = p.value << (14 - bitCount); // all zeros automatically if none exist
    mantissa |= 0x8000; // add implied one;
  } // end of positSplit for Posit16

  // Add methods for posit16 arithmetic
  static Posit16 posit16_add(Posit16 a, Posit16 b) {
    bool aSign, bSign, sign=0;
    int8_t aExponent, bExponent;
    uint16_t aMantissa, bMantissa; // with leading one
    uint8_t esBits;
    int8_t bitCount = 14; // posit bit counter, start of regime
    uint16_t tempResult = 0;

    if (a.value == 0x8000 || b.value == 0x8000) return Posit16((uint16_t) 0x8000);
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, aExponent, aMantissa); // mantissas with leading 1
    positSplit(b, bSign, bExponent, bMantissa);

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
    if (tempMantissa == 0) return Posit16((uint16_t) 0);

    // treat sign of result
    if (tempMantissa < 0) {
      sign=1;
      tempMantissa = -tempMantissa; // back to positive
    }

    if (tempMantissa > 0xFFFFL) tempExponent++; // one more power of two if sum carries to bit8
    else tempMantissa <<= 1; // eliminate uncoded msb otherwise (same exponent or less)
    while (tempMantissa < (long) 65536) { // if msb not reached, subtractions
      //Serial.print("SmallMant ");
      tempExponent--; // divide by two
      tempMantissa <<= 1;
    }
    uint16_t mantissa16 = tempMantissa; // cast to 16bits
    //sprintf(s, "sexp=%02x mant=%05x ", tempExponent, tempMantissa); Serial.print(s);

    return Posit16(sign, tempExponent, mantissa16);
  } // end of posit16_add function definition

  static Posit16 posit16_sub(Posit16 a, Posit16 b) {
    //if (a.value == 0x8000 || b.value == 0x8000) return Posit16((uint16_t) 0x8000);
    /*if (b.value != 0)*/ b.value = -b.value; // change sign, except for zero (NaR)
    return posit16_add(a, b);
  }

  static Posit16 posit16_mul(Posit16 a, Posit16 b) {
    bool aSign, bSign, sign=0;
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
    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa);

    // xor signs, add exponents, multiply mantissas
    sign = (aSign ^ bSign); //tempResult = 0x8000;
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
    while (tempMantissa < 0x10000L) { // if msb not reached, impossible?
      tempExponent--; // divide by two
      tempMantissa <<= 1; // eliminate msb uncoded
      Serial.println("ERROR? : MSB is zero");
    }
    uint16_t mantissa16 = tempMantissa ; 
    return Posit16(sign, tempExponent, mantissa16);
  } // end of posit16_mul function definition

  static Posit16 posit16_div(Posit16 a, Posit16 b) {
    boolean aSign, bSign, sign;
    int8_t aExponent, bExponent;
    uint16_t aMantissa, bMantissa;
    uint8_t esBits;
    uint16_t tempResult = 0;

    if (b.value == 0x8000 || b.value == 0) return Posit16((uint16_t) 0x8000); // NaR if /0 or /NaR
    if (a.value == 0 || a.value == 0x8000 || b.value == 0x4000) return a; // a==0 or NaR, b==1.0

    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa); // with leading one

    // xor signs, sub exponents, div mantissas
    sign = (aSign ^ bSign) ;//tempResult = 0x8000;
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
    return Posit16(sign, tempExponent, tempMantissa);
  } // end of posit8_div function definition

  // Operator overloading for Posit16
  Posit16 operator+ (const Posit16 & other) const {
    return posit16_add(*this, other);
  }
  Posit16 operator- (const Posit16 & other) const {
    return posit16_sub(*this, other);
  }
  Posit16 operator* (const Posit16 & other) const {
    return posit16_mul(*this, other);
  }
  Posit16 operator/ (const Posit16 & other) const {
    return posit16_div(*this, other);
  }
  Posit16& operator+= (const Posit16& other) {
  *this= posit16_add(*this, other);
  return *this;
  }
  Posit16& operator-= (const Posit16& other) {
    *this= posit16_sub(*this, other);
    return *this;
  }
  Posit16& operator*= (const Posit16& other) {
    *this= posit16_mul(*this, other);
    return *this;
  }
  Posit16& operator/= (const Posit16& other) {
    *this= posit16_div(*this, other);
    return *this;
  }
}; // end of Posit16 class definition

// Fractional division routine for sqrt calculation
uint16_t fracDiv (uint16_t above, uint16_t below) {
  uint16_t result=0;
  
  if (above >= below) Serial.println("ERROR fracDiv : result >=1");
  below>>=1; // lsb divider is lost
  for(int8_t bitCount = 14; bitCount>=0; bitCount--) {
    if (above >= below) {
      above -= below;
      result |= (1<<bitCount);
      //Serial.print('1');
    } //else Serial.print('0');

    above <<= 1;
    if (above == 0) break;
  }
  return result;
}

// external functions to provide global scope
static Posit16 posit16_sqrt(Posit16& a) {
  bool aSign;
  int8_t aExponent, bExponent;
  uint16_t aMantissa, bMantissa;

  if (a.value > 0x7FFF) return Posit16(0x8000); // NaR for negative and NaR
  if (a.value == 0) return Posit16(0); // Newton-Raphson would /0
    
  Posit16::positSplit(a, aSign, aExponent, aMantissa);
  bExponent = aExponent >>1; // exponent sqrt = exponent square/2
  aMantissa &=0x7FFF; // exclude leading 1
  //aMantissa >>=1; // move fixed point one bit right to avoid overflows
  bMantissa = (aMantissa>>1); // first approx, ok for even exponents  
  //Serial.print("a no1(");Serial.print(aMantissa,HEX);Serial.print(") "); //*/
  //Serial.print("b org(");Serial.print(bMantissa,HEX);Serial.print(") "); //*/

  if (aExponent &1) {
    bMantissa += 0x4000; // uneven powers of 2 need carried down exponent lsb
    aMantissa += 0x4000; // both at the same bit place !
    aMantissa <<=1; // this can cause overflow, but unsigned substraction corrects it
  }   
  //Serial.print("aMant(");Serial.print(aMantissa,HEX);Serial.print(") "); //*/
  //Serial.print("guess(");Serial.print(bMantissa,HEX);Serial.print(") "); //*/
   
  // Max 5 Newton-raphson iterations for 16 bits, less if no change
  uint8_t iter=0;
  while (iter++<5) {
    uint16_t oldApprox=bMantissa;
    //approx = (approx + a / approx) /2; // Newton-Raphson equation
    bMantissa += fracDiv(aMantissa-bMantissa,0x8000+bMantissa);
    bMantissa >>=1;
    //Serial.print("(");Serial.print(bMantissa,HEX);
    //Serial.print(") "); //*/
    if (bMantissa == oldApprox) break;
  }
  bMantissa <<= 1 ; // correct for fixed point
  return Posit16(aSign,bExponent,bMantissa);
}

Posit16 posit16_next(Posit16& a) {
  uint16_t nextValue = a.value+1;
  return Posit16(nextValue);
}

Posit16 posit16_prior(Posit16& a) {
  uint16_t priorValue = a.value-1;
  return Posit16(priorValue);
}

class Posit8 { // Class definition
  private:
  //uint8_t value; // maybe in future

  public: 
  uint8_t value; // tried int8_t as sign is msb, but nar to nan conversion bugged
                 // Still with 2's complement, int8_t should be possible/better

  Posit8(uint8_t raw = 0): value(raw) {} // Construct from raw unsigned byte
 
  #ifdef byte // Exists in Arduino, but not in all C/C++ toolchains
      Posit8(byte raw = 0): value(raw) {}  { // Construct from raw byte type (unsigned char)
  #endif //*/
  
  Posit8(Posit16 v=0) { // casting from Posit16 to Posit8
    this->value = v.value >> 8;
  }

  Posit8(bool& sign, int8_t tempExponent, uint8_t& tempMantissa) {
    int8_t bitCount = 6; // first regime bit for Posit8

    uint8_t esBits = tempExponent & ((1 << ES8) - 1);
    tempExponent >>= ES8;

    if (tempExponent >= 0) { // abs(v) >= 1, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        this->value |= (1 << bitCount--); // mark one regime bit
      }
      bitCount--; // skip terminating (zero) bit
    } else { // abs(v) < 1, regime bits are zero
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing, bits are already zero
      this->value |= (1 << bitCount--); // mark terminating bit as 1
    }

    if (bitCount >= 0 && ES8 == 2) { // still space for exp msb (2^2=4)
      if (esBits & 2) this->value |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES8) { // still space for exp lsb (2^1)
      if (esBits & 1) this->value |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // is this line redundant ?
      int8_t mantissacount = 1;
      while (bitCount>= 0) { // still space for mantissa
        if (tempMantissa & (1 << (8 - mantissacount++)))
          this->value |= (1 << bitCount);
        bitCount--;
      }
    }
    if (sign) this->value = - (int8_t)this->value;
  }

  Posit8(float v = 0) { // Construct from float32, IEEE754 format
    union float_int { // for bit manipulation
      float tempFloat; // little-endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;
    bool sign=0;

    this->value = 0;
    if (v < 0) { // negative numbers
      if (v >= -EPSILON) return; // very small neg numbers ~=0, non-standard
      sign=true;
      this->value += 0x80; // set sign and continue
    } else { // v>=0
      if (v <= EPSILON) return; // including EPSILON is required as EPSILON can be zero
    }
    if (isnan(v)) {
      this->value = 0x80; // NaR
      return;
    }

    tempValue.tempFloat = v;
    tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
    int8_t tempExponent = tempValue.tempBytes[3] - 127; // remove IEEE754 bias
    uint8_t tempMantissa = tempValue.tempBytes[2];

    // use previous constructor 
    this->value = Posit8(sign, tempExponent, tempMantissa).value;
    /*/ Fill value with result, start with regime
    int8_t bitCount = 6; // first regime bit for Posit8
    if (tempExponent >= 0) { // abs(v) >= 1, regime bits are 1
      while (tempExponent-->= 0 && bitCount >= 0) {
        this->value |= (1 << bitCount--); // mark one regime bit
      }
      bitCount--; // skip terminating (zero) bit
    } else { // abs(v) < 1, regime bits are zero
      while (tempExponent++ < 0 && bitCount--> 0); // do nothing, bits are already zero
      this->value |= (1 << bitCount--); // mark terminating bit as 1
    }

    if (bitCount >= 0 && ES8 == 2) { // still space for exp msb (2^2=4)
      if (esBits & 2) this->value |= (1 << bitCount);
      bitCount--;
    }
    if (bitCount >= 0 && ES8) { // still space for exp lsb (2^1)
      if (esBits & 1) this->value |= (1 << bitCount);
      bitCount--;
    }

    if (bitCount >= 0) { // is this line redundant ?
      int8_t mantissacount = 1;
      while (bitCount>= 0) { // still space for mantissa
        if (tempMantissa & (1 << (8 - mantissacount++)))
          this->value |= (1 << bitCount);
        bitCount--;
      }
    } //*/
  }

  Posit8(double v = 0.0) { // Construct from double by casting to float
    this -> value = Posit8((float) v).value;
  }

  Posit8(int v = 0) { // Construct from int by casting to float for simplicity
    this->value = Posit8((float) v).value; // Now casting to float for simplicity
    /* TODO : construct from int algorithm : first get log2(N), then extract mantissa
    // algorithm copied from https://graphics.stanford.edu/~seander/bithacks.html
    uint32_t v;         // 32-bit value to find the log2 of 
    uint8_t exponent=0; // result of log2(v) will go here
    uint8_t shift; // each bit of the log2
    uint16_t mantissa=v; // Copy of the argument needed to fill mantissa!
    // exponent = (v > 0xFFFF) << 4; v >>= exponent; // only for long values
    shift = (v > 0xFF  ) << 3; v >>= shift; exponent |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; exponent |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; exponent |= shift;
    exponent |= (v >> 1);
    // then extract mantissa (all other bits shifted right). 
    mantissa <<= 16-exponent;
    // then fill Posit8(sign, exponent, mantissa);
    */
  }
  // End of constructors

  // Helper method to split a posit into constituents
  // arguments by reference to avoid copy of Posit and to write to as result
  static void positSplit(Posit8 p, bool& sign, int8_t& exponent, uint8_t& mantissa) {
    bool bigNum; // true for abs(p)>=1
    int8_t bitCount; // posit bit counter for regime, exp and mantissa
 
    sign = (p.value & 128);
    if(sign) p.value=-p.value;
    exponent = -(1 << ES8); // negative exponents start at -1/-2/-4;
    // Note: "exponent" means power of 2, "exp" means exponent bits outside regime
    if ((bigNum = (p.value & 64)!=0)) exponent = 0; // boolean assignment inside test;
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
    bool aSign, bSign, tempSign = false;
    int8_t aExponent, bExponent;
    uint8_t aMantissa, bMantissa, esBits;
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if (a.value == 0x80 || b.value == 0x80) return Posit8((uint8_t) 0x80); // NaR
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa);

    // align smaller number
    if (aExponent > bExponent) bMantissa >>= (aExponent - bExponent);
    if (aExponent < bExponent) aMantissa >>= (bExponent - aExponent);
    int8_t tempExponent = max(aExponent, bExponent);
    int tempMantissa = aMantissa + bMantissa; // Sign not treated here
    if (aSign) tempMantissa -= 2 * aMantissa;
    if (bSign) tempMantissa -= 2 * bMantissa;

    // handle sign of result of mantissa addition
    if (tempMantissa < 0) {
      tempSign = true;
      tempMantissa = -tempMantissa; // as in posit16
    }
    if (tempMantissa == 0) return Posit8(0);

    if (tempMantissa > 255) tempExponent++; // add one power of two if sum carries to bit8
    else tempMantissa <<= 1; // eliminate uncoded msb otherwise (same exponent or less)
    while (tempMantissa < 256) { // if msb not reached (substration)
      //Serial.print("SmallMant ");
      tempExponent--; // divide by two
      tempMantissa <<= 1; // eliminate msb uncoded
    }
    uint8_t temp8M = tempMantissa;
    return Posit8(tempSign,tempExponent,temp8M);
    // TODO move to positFill (sign, exponent, mantissa) or constructor
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
    return posit8_add(a, b); 
  }

  static Posit8 posit8_mul(Posit8 a, Posit8 b) {
    bool aSign, bSign;
    int8_t aExponent = -1, bExponent = -1;
    uint8_t aMantissa = 0, bMantissa = 0, esBits;
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if ((a.value == 0 && b.value != 0x80) || a.value == 0x80) return a; // 0, /0, NaR
    if (b.value == 0 || b.value == 0x80) return b; // 0, NaR
    if (a.value == 0x40) return b; // 1*b
    if (b.value == 0x40) return a; // a*1

    // Split posit into constituents
    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa);

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

    
    // TODO move to positFill(sign, exponent, mantissa) or constructor
    // Extract exponent lsb(s) for exponent fieldesBits = tempExponent & ((1 << ES8) - 1);
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
    bool aSign, bSign;
    //bool aBigNum, bBigNum; // 0 between 0 and 1, 1 otherwise
    int8_t aExponent, bExponent;
    uint8_t aMantissa, bMantissa, esBits;
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if (b.value == 0x80 || b.value == 0) return Posit8((uint8_t) 0x80); // NaR if /0 or /NaR
    if (a.value == 0 || a.value == 0x80 || b.value == 0x40) return a; // a==0 or NaR, b==1.0

    Posit8::positSplit(a, aSign, aExponent, aMantissa);
    Posit8::positSplit(b, bSign, bExponent, bMantissa);

    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) tempResult = 0x80;
    int tempExponent = (aExponent - bExponent);
    // first solution to divide mantissas : use float division (not efficient?)
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;

    tempValue.tempFloat = ((float) aMantissa / (float) bMantissa); // result should be between 0.5 and 2

    tempValue.tempInt <<= 1; // eliminate sign, align exponent and mantissa
    uint8_t tempMantissa = tempValue.tempBytes[2]; // eliminate msb
    tempExponent += tempValue.tempBytes[3] - 127;
    //sprintf(s, " mant=%02x ", tempMantissa); Serial.print(s);
    /*sprintf(s, "aexp=%02x mant=%02x ", aExponent, aMantissa); Serial.print(s);
      sprintf(s, "bexp=%02x mant=%02x ", bExponent, bMantissa); Serial.println(s);
      sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/
    
    // TODO move to positFill(aSign ^ bSign, exponent, mantissa) or constructor
    // Extract exponent lsb(s) for exponent field
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
    // Write mantissa bits
    if (bitCount >= 0) { // still space for mantissa
      int8_t mantissacount = 1;
      while (bitCount-->= 0) {
        if (tempMantissa & (1 << (8 - mantissacount++)))
          tempResult |= (1 << (bitCount + 1));
      }
    }
    return Posit8(tempResult);
  } // end of posit8_div function definition


static Posit8 posit8_sqrt(Posit8& a) {
  Serial.println(a.value,HEX);
  if (a.value > 0x7F) return Posit8((uint8_t)0x80); // NaR for negative and NaR
  if (a.value == 0) return Posit8(0); // Newton-Raphson would /0

  Posit8 approx = a; // Initial approximation, OK for small regimes
  if ((a.value > 0x60) || (a.value < 0x1F)) { // Regime >= 2 bits
    approx.value = (a.value<<1) & 0x7F; // Better initial approximation
    //Serial.print("Approx (");Serial.print(approx.value,BIN);
    //Serial.print("): ");Serial.println(posit2float(approx)); //*/
  }
  if ((approx.value > 0x70) || (approx.value < 0x0F)) { // Regime >= 4 bits
    approx.value = (approx.value<<1) & 0x7F; // Better initial approximation
    //Serial.print("Approx (");Serial.print(approx.value,BIN);
    //Serial.print("): ");Serial.println(posit2float(approx)); //*/
  }
  Posit8 half = Posit8(0.5); // using float constructor, varies with ES8
  
  // Newton-raphson iterations (not converging for 100 !)
  for (int8_t iter=0; iter<9; iter++) {
    Posit8 oldApprox=approx;
    approx = (approx + a / approx) * half; // needs better rounding!
    Serial.print("(");Serial.print(approx.value,BIN);
    Serial.print(") "); //*/
    if (approx.value == oldApprox.value) break; 
  }
  Serial.println();
  return approx;
}

static Posit8 posit8_next(Posit8 a) {
  uint8_t nextValue = a.value+1;
  return Posit8(nextValue);
}

static Posit8 posit8_prior(Posit8 a) {
  uint8_t priorValue = a.value-1;
  return Posit8(priorValue);
}

  // Operator overloading for Posit8
  Posit8 operator + (const Posit8 & other) const {
    return posit8_add( * this, other);
  }
  Posit8 operator - (const Posit8 & other) const {
    return posit8_sub( * this, other);
  }
  Posit8 operator * (const Posit8 & other) const {
    return posit8_mul( * this, other);
  }
  Posit8 operator / (const Posit8 & other) const {
    return posit8_div( * this, other);
  } //*/
}; // end of Posit8 Class definition

/**/
Posit16::Posit16(Posit8 a) { // Definition of Posit8 casting to Posit16
  this->value = (uint16_t)(a.value<<8);
} //*/

float posit2float(Posit16 p) {
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 13; // posit bit counter 
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  }
  tempValue;
  // Handle special cases first
  if (p.value == 0) return 0.0f;
  if (p.value == 0x8000) return NAN;
  if (p.value & 0x8000) {
    p.value=-p.value;
    sign = true; // negative
  }
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
} // end of posit2float 16-bit

float posit2float(Posit8 p) { // Can't be by ref since value is modified
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 5; // posit bit counter
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  } tempValue;
  // Handle special cases first
  if (p.value == 0) return 0.0f;
  if (p.value == 0x80) return NAN;
  if (p.value & 0x80){
    p.value=-p.value;
    sign = true; // negative
  } 
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
} // end of posit2float 8-bit
