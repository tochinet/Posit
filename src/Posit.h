/*************************************************************************************

  Posit Library for Arduino v0.1.3 (not yet released)

    Copyright (c) 2024-2025 Christophe Vermeulen

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

  This library provides posit8 and posit16 floating point arithmetic support
  for the Arduino environment, especially the memory-limited 8-bit AVR boards.
  Posits are a more efficient and more precise alternative to IEEE754 floats.

  Major design goals are :
  - Small size, both in code/program memory and RAM usage
  - Simplicity, restricting the library to 8 and 16 bits posits and most common operations
  - Useful, explanatory comments in the library
 
  As corollary, major non-goals are :
  - Support for 32bits posits (not enough added value compared with existing floats)
  - Full compliance with the Posit-2022 standard (too many functions)
  - Complex rounding algorithms (rounding to nearest even requires handling G, R and S bits)
  - Support of quire (very long accumulator)
  - Use of templates, boilerplate, etc. bloating code memory

  CURRENT STATUS
  Provides + - * / sqrt next prior sign abs neg
  Provides trigonometric routines sin cos tan atan with conditional compilation
  Changed the class names to posit8_t and posit16_t for compatibility with other implementations
  WIP : reduce library size if ES8 == ES16 by using 16-bit code for both (done for posit2float)
  TODO : add sin/cos/tan PI*x using Taylor or Chebyshev
  TODO : improve precision of trigonometric routines (now only precise around zero)
  DROPPPED : add p10_t class for byte storage of 10bit [0..1[ numbers (probability)
     - this is nonstandard and ... maybe not very useful since posits with ES=0 is linear between -1 and 1
  

  See https://github.com/tochinet/Posit/ for more details on Posits and the library.
************************************************************************************/

#ifndef EPSILON     // Define EPSILON in sketch for rounding down ...
#define EPSILON 0.0 // ... results smaller than EPSILON to zero in Posit8 ... 
#endif              // ... and EPSILON^2 in Posit16 arithmetic
// This may allow to speed up RL algorithms and reduce memory usage of sketch, even if 
// explicitly not recommended according to https://posithub.org/docs/Posits4.pdf p.7.

#ifndef ES8   // ES8 can be set in sketch to 0 or 1 ...
#define ES8 2 // or defaults to two exponent bits (standard)
#endif
#define ES16 2 // Posit 16 always have two-bits exponent field
//#define ES32 2 // No support envisioned for Posit32.
//#define NOTRIG // uncomment/put in sketch to exclude trig routines

#ifdef DEBUG
char s[30]; // temporary C string for Serial debug using sprintf
#endif

struct splitPosit { // struct not used yet. TODO check if it reduces or increases memory usage
  boolean sign;
  int8_t powerof2; // 2's power, combining regime and exponent fields
  uint16_t mantissa; // worth using 16 bits to share struct between 8 and 16 bits
}; 

class posit8_t; // Forward-declared for casting from posit8_t to posit16_t

class posit16_t {
  private:
  //uint16_t value; // moved to public since used by posit2float

  public: 
  uint16_t value;

  posit16_t(uint16_t v = 0): value(v) {} // default constructor, raw from unsigned 16-bit value

  // construct from parts (sign, 2's power and mantissa without leading 1)
  posit16_t(bool& sign, int8_t powerof2, uint16_t& tempMantissa) {
    // powerof2 passed by value, as it will be modified
    // sign and mantissa passed by reference to avoid copy, they won't be modified.
    // REJECTED using mantissa with leading one. But mantissa might be moved down someday

#ifdef DEBUG
    //Serial.print(sign?" -1.":" +1.");
    //Serial.print(tempMantissa,HEX); // bug : missing leading zeros
    //Serial.print("x2^");Serial.println(powerof2); Serial.print(' ');
#endif

    int8_t bitCount = 14; // first bit of regime
    uint16_t tempResult = 0; // better than using this->value ? TODO evaluate !

    // Extract exponent lsb(s) for exponent field
    uint8_t esBits = powerof2 & ((1 << ES16) - 1);
    powerof2 >>= ES16;
    if (powerof2 >= 0) { // positive exponent, regime bits are 1
      while (powerof2-->= 0 && bitCount >= 0) {
        tempResult |= (1 << bitCount--);
      }
      bitCount--; // terminating zero
    } else { // abs(v) < 1
      while (powerof2++ < 0 && bitCount--> 0); // do nothing
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
      if (tempMantissa & (1 << (16 - mantissacount++))) // 15 if leading one
        tempResult |= (1 << (bitCount + 1));
      //}
    }
    this->value=sign?~tempResult+1:tempResult;
  } // end of posit16 constructor from parts

  posit16_t(float v = 0) { // Construct from float32, IEEE754 format
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
    // 7 and >>1 if leading one, but no need to "recreate" leading one

    this->value = posit16_t(sign, exponent, mantissa).value;
  }

  posit16_t(double v = 0) { // Construct from double by casting to float32
    this->value = posit16_t((float)v).value;
  }

  posit16_t(int v = 0) { // Construct from int by casting to float to KISShort
    this->value = posit16_t((float)v).value;
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
    // then fill posit(sign, exponent, mantissa);
    */  }

  posit16_t(posit8_t) ; // forward declaration
  // End of constructors

  static void positSplit(posit16_t p, bool& sign, int8_t& exponent, uint16_t& mantissa) {
    // TODO use powerof2 and maybe splitPosit struct
    // Reads posit16_t by value, writes sign, exponent and mantissa (with leading 1)
    bool bigNum; // true for abs(p) >= 1
    int8_t bitCount= 5+8; // posit bit counter for regime, exp and mantissa

    sign = (p.value & 0x8000);
    if (sign) p.value = -p.value; // intrinsic cast to signed and back
    exponent = -(1 << ES16); // negative exponents start at -1/-2/-4;
    // Note: don't confuse this "exponent" (power of 2) with the exponent field (2 lsbs outside regime)
    bigNum = (p.value >= 0x4000 && p.value < 0xC000);
    if (bigNum) exponent = 0; // positive exponents start at 0

    //first extract exponent from regime bits
    while ((p.value & (1 << bitCount)) == ((bigNum) << bitCount--)) { // still regime
      if (bigNum) exponent += 1 << ES16;
      else exponent -= 1 << ES16; // add/sub 2^es for each bit in regime
      if (bitCount == -1) break; // all regime, no space for exponent or mantissa
    }
    // add exponent field bits if any
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
  } // end of positSplit for posit16_t

  // Posit16 methods for posit16 4 operations (+ - * /)
  static posit16_t posit16_add(posit16_t a, posit16_t b) {
    bool aSign, bSign, sign=0;
    int8_t aExponent, bExponent, tempExponent;
    uint16_t aMantissa, bMantissa, tempMantissa; // with leading one
    uint8_t esBits;
    int8_t bitCount = 14; // posit bit counter, start of regime
    uint16_t tempResult = 0;

    if (a.value == 0x8000 || b.value == 0x8000) return posit16_t((uint16_t) 0x8000); // NaR
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, aExponent, aMantissa); // mantissas with leading 1
    positSplit(b, bSign, bExponent, bMantissa);

    // align smaller number (mantissa) with bigger number
    if (aExponent > bExponent) bMantissa >>= (aExponent - bExponent);
    if (aExponent < bExponent) aMantissa >>= (bExponent - aExponent);
    tempExponent = max(aExponent, bExponent);
    // TODO consider alternatives to avoid 32 bit arithmetic on AVR if possible
    long longMantissa = (long) aMantissa + bMantissa; // Sign not treated here
    if (aSign) {
      longMantissa -= aMantissa;
      longMantissa -= aMantissa; // written so to avoid unnecessary casts
    }
    if (bSign) {
      longMantissa -= bMantissa;
      longMantissa -= bMantissa;
    }

    // treat sign of result
    if (longMantissa < 0) {
      sign=1;
      longMantissa = -longMantissa; // back to positive
    }
    if (longMantissa == 0) return posit16_t((uint16_t) 0); // could use EPSILON here ?

    if (longMantissa > 0xFFFFL) tempExponent++; // one more power of two if sum carries over
    else longMantissa <<= 1; // eliminate uncoded msb otherwise (same exponent or less)
    while (longMantissa < (long) 65536) { // if msb not reached, in case of subtractions
      // would be 32768 if there were no leading ones
      //Serial.print("SmallMant ");
      tempExponent--; // divide by two
      longMantissa <<= 1;
    }
    tempMantissa = longMantissa; // cast to 16bits
#ifdef DEBUG
    //sprintf(s, "sexp=%02x mant=%05x ", tempExponent, tempMantissa); Serial.print(s);
#endif

    return posit16_t(sign, tempExponent, tempMantissa);
  } // end of posit16_add function definition

  static posit16_t posit16_sub(posit16_t a, posit16_t b) {
    b.value = -b.value; // 2's complement solves everything for us, 0 and NaR are conserved
    return posit16_add(a, b);
  }

  static posit16_t posit16_mul(posit16_t a, posit16_t b) {
    bool aSign, bSign, sign=0;
    int8_t aExponent, bExponent, tempExponent;
    uint16_t aMantissa, bMantissa, tempMantissa;
    uint8_t esBits;
    int8_t bitCount; // posit bit counter
    uint16_t tempResult = 0;

    if ((a.value == 0 && b.value != 0x8000) || a.value == 0x8000) return a; // 0, NaR
    if (b.value == 0 || b.value == 0x8000) return b;
    if (a.value == 0x4000) return b;
    if (b.value == 0x4000) return a;

    // Split posit into constituents
    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa); // Mantissas with leading 1

    // xor signs, add exponents, multiply mantissas
    sign = (aSign ^ bSign); //tempResult = 0x8000;
    tempExponent = (aExponent + bExponent);

    // TODO : consider using 16-bit fractional multiplication routine (similar to fracDiv)
    uint32_t longMantissa = ((uint32_t) aMantissa * bMantissa) >> 14; // puts result in lower 16 bits and eliminates msb
#ifdef DEBUG
    /*sprintf(s, "aexp=%02x mant=%04x ", aExponent, aMantissa); Serial.print(s);
    sprintf(s, "bexp=%02x mant=%04x ", bExponent, bMantissa); Serial.println(s);
    sprintf(s, "mexp=%02x mant=%06x ", tempExponent, (uint32_t)longMantissa); Serial.println(s); //*/
#endif

    // normalise mantissa
    if (longMantissa > 0x1FFFFL) {
      tempExponent++; // add power of two if product carried to MSB
      longMantissa >>= 1; // eliminate uncoded msb otherwise (same exponent or less)
    }
    while (longMantissa < 0x10000L) { // if msb not reached, impossible?
      tempExponent--; // divide by two
      longMantissa <<= 1; // eliminate msb uncoded
      Serial.println("ERROR posit16_mul : MSB is zero");
    }
    tempMantissa = longMantissa ; 
    return posit16_t(sign, tempExponent, tempMantissa);
  } // end of posit16_mul function definition

  static posit16_t posit16_div(posit16_t a, posit16_t b) {
    boolean aSign, bSign, sign;
    int8_t aExponent, bExponent;
    uint16_t aMantissa, bMantissa;
    uint8_t esBits;
    uint16_t tempResult = 0;

    if (b.value == 0x8000 || b.value == 0) return posit16_t((uint16_t) 0x8000); // NaR if /0 or /NaR
    if (a.value == 0 || a.value == 0x8000 || b.value == 0x4000) return a; // a==0 or NaR, b==1.0

    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa); // with leading one

    // xor signs, sub exponents, div mantissas
    sign = (aSign ^ bSign) ;
    int tempExponent = (aExponent - bExponent);
    // first solution to divide mantissas : use float (not efficient)
    // TODO consider using fracdiv routine, like sqrt
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;

    // 0x8000 <= aMantissa and bMantissa <= 0xFFF0, 11-bit mantissa
    tempValue.tempFloat = ((float) aMantissa / (float) bMantissa); // result should be between 0.5 and 2

    tempValue.tempInt <<= 1; // eliminate sign, byte-align exponent and mantissa
    uint16_t tempMantissa = tempValue.tempBytes[2] * 256 + tempValue.tempBytes[1]; // eliminate msb
    tempExponent += tempValue.tempBytes[3] - 127; // correct if necessary
    return posit16_t(sign, tempExponent, tempMantissa);
  } // end of posit16_div function definition

  // Operator overloading for Posit16
  posit16_t operator + (const posit16_t& other) const {
    return posit16_add(*this, other);
  }
  posit16_t operator - (const posit16_t& other) const {
    return posit16_sub(*this, other);
  }
  posit16_t operator * (const posit16_t& other) const {
    return posit16_mul(*this, other);
  }
  posit16_t operator / (const posit16_t& other) const {
    return posit16_div(*this, other);
  }
  posit16_t& operator += (const posit16_t& other) {
  *this= posit16_add(*this, other);
  return *this;
  }
  posit16_t& operator -= (const posit16_t& other) {
    *this= posit16_sub(*this, other);
    return *this;
  }
  posit16_t& operator *= (const posit16_t& other) {
    *this= posit16_mul(*this, other);
    return *this;
  }
  posit16_t& operator /= (const posit16_t& other) {
    *this= posit16_div(*this, other);
    return *this;
  }
}; // end of posit16_t class definition

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
static posit16_t posit16_sqrt(posit16_t& a) {
  bool aSign;
  int8_t aExponent, tempExponent;
  uint16_t aMantissa, tempMantissa;

  if (a.value > 0x7FFF) return posit16_t(0x8000); // NaR for negative and NaR
  if (a.value == 0) return posit16_t(0); // Newton-Raphson would /0
    
  posit16_t::positSplit(a, aSign, aExponent, aMantissa);
  tempExponent = aExponent >>1; // exponent sqrt = exponent square/2
  aMantissa &=0x7FFF; // exclude leading 1
  //aMantissa >>=1; // move fixed point one bit right to avoid overflows
  tempMantissa = (aMantissa>>1); // first approx, ok for even exponents  
#ifdef DEBUG
  //Serial.print("a no1(");Serial.print(aMantissa,HEX);Serial.print(") ");
  //Serial.print("b org(");Serial.print(tempMantissa,HEX);Serial.print(") ");
#endif

  if (aExponent &1) {
    tempMantissa += 0x4000; // uneven powers of 2 need carried down exponent lsb
    aMantissa += 0x4000; // both at the same bit place !
    aMantissa <<=1; // this can cause overflow, but unsigned substraction will correct it
  }   
#ifdef DEBUG
  //Serial.print("aMant(");Serial.print(aMantissa,HEX);Serial.print(") ");
  //Serial.print("guess(");Serial.print(bMantissa,HEX);Serial.print(") ");
#endif

  // Max 5 Newton-raphson iterations for 16 bits, less if no change
  uint8_t iter=0;
  while (iter++<5) {
    uint16_t oldApprox=tempMantissa;
    //approx = (approx + a / approx) /2; // Newton-Raphson equation
    //mantissa = mantissa + (aMantissa - mantissa) / (1 + mantissa) // applied to 1.xxx numbers
    tempMantissa += fracDiv(aMantissa-tempMantissa,0x8000+tempMantissa);
    tempMantissa >>=1;
    //Serial.print("(");Serial.print(bMantissa,HEX);Serial.print(") ");
    if (tempMantissa == oldApprox) break;
  }
  tempMantissa <<= 1 ; // correct for fixed point
  return posit16_t(aSign,tempExponent,tempMantissa);
}

#ifndef NOTRIG
posit16_t Pi16 = 3.141602; // closest value
posit16_t HalfPi16 = (uint16_t)0x4491; //=1.57079633+.00000445;

static posit16_t posit16_sin(posit16_t& a) {
  /*bool aSign;
  int8_t aExponent, tempExponent;
  uint16_t aMantissa, tempMantissa;*/
  
  while (a.value > Pi16.value) a -= Pi16; // < > == operators not yet in library
  while (a.value < -Pi16.value) a += Pi16;
  
  // First implementation using Taylor : sin x = x - x^3/6 = x/3 * (3-x^2/2) // more precise?
  // TODO second iteration : average with cos (PI/2-x) =  1 - x^2/2 is some range ?
  // TODO create posit<x>_half routines for posit16_t and posit8_t /2 (exponent--)
  posit16_t aHalfSquare = a*(a/2);
  posit16_t tempResult = (a/3)*((posit16_t)3-aHalfSquare);
  return tempResult;
}

static posit16_t posit16_cos(posit16_t& a) {
  // First implementation using Taylor : cos x = 1 - x^2/2
  posit16_t tempResult = (posit16_t)1- a*(a/2);
  return tempResult;
}

static posit16_t posit16_tan(posit16_t& a) {
  // first iteration : tan x = x + x^3/3 = x/3 * (3+x^2) // more precise?
  posit16_t aSquare = a*a;
  posit16_t tempResult = (a/3)*((posit16_t)3+aSquare);
  return tempResult;
}

static posit16_t posit16_atan(posit16_t& a) {
  // first iteration : atan x = x - x^3/3 = x/3 * (3-x^2)
  posit16_t aSquare = a*a;
  posit16_t tempResult = (a/3)*((posit16_t)3-aSquare);
  return tempResult;
}
#endif

posit16_t posit16_next(posit16_t& a) {
  uint16_t nextValue = a.value+1;
  return posit16_t(nextValue);
}

posit16_t posit16_prior(posit16_t& a) {
  uint16_t priorValue = a.value-1;
  return posit16_t(priorValue);
}

posit16_t posit16_sign(posit16_t a) {
  if (a.value == 0 || a.value == 0x8000) return a;
  return posit16_t(a.value & 0x8000 ? 0xC000 : 0x4000);
}

posit16_t posit16_neg(posit16_t a) {
  return posit16_t(-a.value);
}

posit16_t posit16_abs(posit16_t a) {
  return a.value&0x8000 ? posit16_neg(a) : a;
}

class posit8_t { // Class definition
  private:
  //uint8_t value; // maybe in future

  public: 
  uint8_t value; // tried int8_t as sign is msb, but nar to nan conversion bugged
                 // Still with 2's complement, int8_t should be possible/better

  posit8_t(uint8_t raw = 0): value(raw) {} // Construct from raw unsigned byte
 
  #ifdef byte // Exists in Arduino, but not in all C/C++ toolchains
      posit8_t(byte raw = 0): value(raw) {}  { // Construct from raw byte type (unsigned char)
  #endif //*/
  
  posit8_t(posit16_t v=0) { // casting from posit16_t to posit8_t
    this->value = (v.value & 128) ? (v.value >> 8) : (v.value >> 8)+1-2*(v.value<0); // first try at rounding
  }

  posit8_t(bool& sign, int8_t tempExponent, uint8_t& tempMantissa) {
    int8_t bitCount = 6; // first regime bit for Posit8
#ifdef DEBUG
    /*Serial.print(sign?"-1.":"+1.");
    Serial.print(tempMantissa,HEX); // bug, missing leading zeros if any
    Serial.print("x2^");Serial.print(tempExponent); //*/
#endif
    this->value=0;

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
    if (sign) this->value = ~ this->value +1;
    //Serial.print(" ("); Serial.print(this->value,BIN); Serial.print(") ");
  }

  posit8_t(float v = 0) { // Construct from float32, IEEE754 format
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

    this->value = posit8_t(sign, tempExponent, tempMantissa).value;
  }

  posit8_t(double v = 0.0) { // Construct from double by casting to float
    this -> value = posit8_t((float) v).value;
  }

  posit8_t(int v = 0) { // Construct from int by casting to float for simplicity
    this->value = posit8_t((float) v).value; // Now casting to float for simplicity
  }
  // End of constructors

  // Helper method to split a posit into constituents
  // arguments by reference to write to as result
  static void positSplit(posit8_t p, bool& sign, int8_t& exponent, uint8_t& mantissa) {
    bool bigNum; // true for abs(p)>=1
    int8_t bitCount=5; // posit bit counter for regime, exp and mantissa
 
    sign = (p.value & 0x80);
    if(sign) p.value = -p.value;
    exponent = -(1 << ES8); // negative exponents start at -1/-2/-4;
    bigNum = (p.value >= 0x40 && p.value < 0xC0);
    if (bigNum) exponent = 0; // positive exponents start at 0

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
    mantissa |= 0x80; // add implied one if not already set
#ifdef DEBUG
    //sprintf(s, "exp=%02x mant=%02x ", exponent, mantissa); Serial.print(s);
#endif
  } // end of positSplit

  // Methods for posit8 arithmetic
  static posit8_t posit8_add(posit8_t a, posit8_t b) {
    bool aSign, bSign, sign=0;
    int8_t aExponent, bExponent, tempExponent;
    uint8_t aMantissa, bMantissa, tempMantissa; // with leading one
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if (a.value == 0x80 || b.value == 0x80) return posit8_t((uint8_t) 0x80); // NaR
    if (a.value == 0) return b;
    if (b.value == 0) return a;

    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa);

    if (aExponent > bExponent) bMantissa >>= (aExponent - bExponent);
    if (aExponent < bExponent) aMantissa >>= (bExponent - aExponent);
    tempExponent = max(aExponent, bExponent);
    int16_t longMantissa = aMantissa + bMantissa;
    if (aSign) {
      longMantissa -= aMantissa;
      longMantissa -= aMantissa;
    }
    if (bSign) {
      longMantissa -= bMantissa;
      longMantissa -= bMantissa;
    }

    // treat sign of result
    if (longMantissa < 0) {
      sign = 1;
      longMantissa = -longMantissa; // back to positive
    }
    if (longMantissa ==0) return posit8_t((uint8_t)0);

    if (longMantissa > 255) tempExponent++; // add one power of two if sum carries to bit8
    else longMantissa <<= 1; // eliminate uncoded msb otherwise (same exponent or less)
    while (longMantissa < 256) { // if msb not reached (substration)
#ifdef DEBUG
      Serial.print("SmallMant ");
#endif
      tempExponent--; // divide by two
      longMantissa <<= 1; // eliminate msb uncoded
    }
#ifdef DEBUG
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);
#endif
    tempMantissa = longMantissa;
    return posit8_t(sign,tempExponent,tempMantissa);
  } // end of posit8_add function definition

  static posit8_t posit8_sub(posit8_t a, posit8_t b) {
    b.value = -b.value;
    return posit8_add(a, b); 
  }

  static posit8_t posit8_mul(posit8_t a, posit8_t b) {
    bool aSign, bSign, sign = false;
    int8_t aExponent, bExponent;
    uint8_t aMantissa, bMantissa;
    int8_t bitCount;
    uint8_t tempResult = 0;

    if ((a.value == 0 && b.value != 0x80) || a.value == 0x80) return a; // 0, /0, NaR
    if (b.value == 0 || b.value == 0x80) return b; // 0, NaR
    if (a.value == 0x40) return b; // 1*b
    if (b.value == 0x40) return a; // a*1

    positSplit(a, aSign, aExponent, aMantissa);
    positSplit(b, bSign, bExponent, bMantissa);

    if (aSign ^ bSign) sign = true;
    
    int tempExponent = (aExponent + bExponent);
    uint16_t tempMantissa = (uint16_t)(aMantissa * bMantissa) >>6 ; // 1xxxxxxx * 1xxxxxxx >= 01xxx...
#ifdef DEBUG
    //sprintf(s, "MUL<norm exp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);
#endif

    while (tempMantissa > 511) { 
      tempExponent++;
      tempMantissa >>= 1; // move towards bit8-alignment
      //Serial.println("OK? : MSB is 2 or more"); //*/
    }
    while (tempMantissa < 256) { // if msb not reached, possible?
      tempExponent--; // divide by two
      tempMantissa <<= 1; // eliminate msb uncoded
#ifdef DEBUG
      //sprintf(s, "MULunder exp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);
      Serial.println("ERROR? : MSB is zero");
#endif
    }
#ifdef DEBUG
    //sprintf(s, "sexp=%02x mant=%02x ", tempExponent, tempMantissa); Serial.print(s);
#endif

    uint8_t mantissa8 = tempMantissa;
    return posit8_t(sign,tempExponent,mantissa8);
  } // end of posit8_mul function definition

  static posit8_t posit8_div(posit8_t a, posit8_t b) { // args by ref doesn't compile
    bool aSign, bSign, sign = false;
    int8_t aExponent, bExponent;
    uint8_t aMantissa, bMantissa, esBits;
    int8_t bitCount; // posit bit counter
    uint8_t tempResult = 0;

    if (b.value == 0x80 || b.value == 0) return posit8_t((uint8_t) 0x80); // NaR if /0 or /NaR
    if (a.value == 0 || a.value == 0x80 || b.value == 0x40) return a; // a==0 or NaR, b==1.0

    posit8_t::positSplit(a, aSign, aExponent, aMantissa);
    posit8_t::positSplit(b, bSign, bExponent, bMantissa);

    // xor signs, sub exponents, div mantissas
    if (aSign ^ bSign) sign = true ; // tempResult = 0x80;
    int tempExponent = (aExponent - bExponent);
    // first solution to divide mantissas : use float division (not efficient)
    union float_int { // for bit manipulation
      float tempFloat; // little endian in AVR8
      uint32_t tempInt; // little-endian as well
      uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
    } tempValue;

    tempValue.tempFloat = ((float) aMantissa / (float) bMantissa); // 0.5 < 128/255 <= result <= 255/128 < 2

    tempValue.tempInt <<= 1; // eliminate sign, align exponent and mantissa
    uint8_t tempMantissa = tempValue.tempBytes[2]; // division mantissa is there
    tempExponent += tempValue.tempBytes[3] - 127; // either 0 or -1
#ifdef DEBUG
    //sprintf(s, " mant=%02x ", tempMantissa); Serial.print(s);
    /*sprintf(s, "aexp=%02x mant=%02x ", aExponent, aMantissa); Serial.print(s);
      sprintf(s, "bexp=%02x mant=%02x ", bExponent, bMantissa); Serial.println(s);
      sprintf(s, "sexp=%02x 1mant=%02x ", tempExponent, tempMantissa); Serial.println(s); //*/
#endif
    return posit8_t(sign,tempExponent,tempMantissa);
  } // end of posit8_div function definition

static posit8_t posit8_sqrt(posit8_t& a) {
  if (a.value > 0x7F) return posit8_t((uint8_t)0x80); // NaR for negative and NaR
  if (a.value == 0) return posit8_t(0); // Newton-Raphson would /0

  posit8_t approx = a; // Initial approximation, OK for small regimes
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
  posit8_t half = posit8_t(0.5); // using float constructor, value varies with ES8
  
  // Newton-raphson iterations (not converging for 100 !)
  for (int8_t iter=0; iter<9; iter++) {
    posit8_t oldApprox=approx;
    approx = (approx + a / approx) * half; // needs better rounding!
#ifdef DEBUG
    Serial.print("(");Serial.print(approx.value,BIN);
    Serial.print(") "); //*/
#endif
    if (approx.value == oldApprox.value) break; 
  }
#ifdef DEBUG
  Serial.println();
#endif
  return approx;
}

#ifndef NOTRIG
// Formulas from https://en.wikipedia.org/wiki/Taylor_series#Trigonometric_functions
//static const posit8_t Pi8 = posit8_t((uint8_t)0x4D); // 3.25 better than 3 since rounding goes down
//static const posit8_t HalfPi8 = posit8_t((uint8_t)0x45)); // = 1.625 = 1.57+0.055

static posit8_t posit8_sin(posit8_t& a) {
  // first iteration : sin x = x - x^3/6 = x/3 * (3-x^2/2) // TODO check smaller error
  posit8_t aHalfSquare = a*(a/2);
  posit8_t tempResult = (a/3)*((posit8_t)3-aHalfSquare);
  return tempResult;
}

static posit8_t posit8_cos(posit8_t& a) {
  // first iteration : cos x = 1 - x^2/2
  posit8_t tempResult = (posit8_t)1- a*(a/2);
  return tempResult;
}

static posit8_t posit8_tan(posit8_t& a) {
  // first iteration : tan x = x + x^3/3 = x/3 * (3+x^2)
  posit8_t aSquare = a*a;
  posit8_t tempResult = (a/3)*((posit8_t)3+aSquare);
  return tempResult;
}

static posit8_t posit8_atan(posit8_t& a) {
  // first iteration : atan x = x - x^3/3 = x/3 * (3-x^2)
  posit8_t aSquare = a*a;
  posit8_t tempResult = (a/3)*((posit8_t)3-aSquare);
  return tempResult;
}
#endif // NOTRIG

static posit8_t posit8_next(posit8_t& a) {
  uint8_t nextValue = a.value+1;
  return posit8_t(nextValue);
}

static posit8_t posit8_prior(posit8_t& a) {
  uint8_t priorValue = a.value-1;
  return posit8_t(priorValue);
}

static posit8_t posit8_abs(posit8_t& a) {
  if (a.value == 0 || a.value == 0x80) return a;
  uint8_t absValue = a.value&0x80 ? -a.value : a.value;
  return posit8_t(absValue);
}

static posit8_t posit8_neg(posit8_t& a) {
  return posit8_t((uint8_t)-a.value);
}

static posit8_t posit8_sign(posit8_t a) {
  if (a.value == 0 || a.value == 0x80) return a;
  uint8_t signValue=a.value & 0x80 ? 0xC0 : 0x40;
  return posit8_t(signValue);
}

  // Operator overloading for Posit8
  posit8_t operator + (const posit8_t& other) const {
    return posit8_add(*this, other);
  }
  posit8_t operator - (const posit8_t& other) const {
    return posit8_sub(*this, other);
  }
  posit8_t operator * (const posit8_t& other) const {
    return posit8_mul(*this, other);
  }
  posit8_t operator / (const posit8_t& other) const {
    return posit8_div(*this, other);
  }
  posit8_t& operator += (const posit8_t& other) {
  *this= posit8_add(*this, other);
  return *this;
  }
  posit8_t& operator -= (const posit8_t& other) {
    *this= posit8_sub(*this, other);
    return *this;
  }
  posit8_t& operator *= (const posit8_t& other) {
    *this= posit8_mul(*this, other);
    return *this;
  }
  posit8_t& operator /= (const posit8_t& other) {
    *this= posit8_div(*this, other);
    return *this;
  }
}; // end of posit8_t Class definition

posit16_t::posit16_t(posit8_t a) { // Definition of posit8_t casting to posit16_t
  this->value = (uint16_t)(a.value<<8);
}

float posit2float(posit16_t p) {// Can't be by reference since value is modified
  boolean sign = false;
  boolean bigNum = false; // 0/false between -1 and +1, 1/true otherwise
  int8_t exponent = 0; // correct if abs >= 1.0
  int8_t bitCount = 13; // posit bit counter 
  union float_int { // for bit manipulation
    float tempFloat; // little endian in AVR8
    uint32_t tempInt; // little-endian as well
    uint8_t tempBytes[4]; // [3] includes sign and exponent MSBs
  } tempValue;
  // Handle special cases first
  if (p.value == 0) return 0.0f;
  if (p.value == 0x8000) return NAN;
  if (p.value & 0x8000) {
    p.value=-p.value;
    sign = true; // negative
  }
  if (p.value & 0x4000) {
    bigNum = true; // bit14 set : abs(value) >= 1.0
  } else {
    exponent = -(1 << ES16); // negative exponents start at -1/-2/-4
  }
  while ((p.value & (1 << bitCount)) == (bigNum << bitCount--)) { // regime bits
    bigNum ? exponent += (1 << ES16) : exponent -= (1 << ES16); // add 1, 2 or 4 for each regime bit
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

float posit2float(posit8_t p) { 
#if ES8==ES16
  return posit2float((posit16_t)p); // simply casting by adding zeros
#else  
  boolean sign = false;
  boolean bigNum = false; 
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
  if (p.value & 0x40) { //bit6 set : abs(value) >= 1.0
    bigNum = true;
  } else {
    exponent = -(1 << ES8); // negative exponents start at -1/-2/-4
  }
  while ((p.value & (1 << bitCount)) == (bigNum << bitCount--)) { // regime bits
    bigNum ? exponent += (1 << ES8) : exponent -= (1 << ES8); // add 1, 2 or 4 for each regime bit
    if (bitCount == -1) break; // no marker, exponent or mantissa at all
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
#endif
} // end of posit2float 8-bit
