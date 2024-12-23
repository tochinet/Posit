## Planned improvements
* Better rounding (with compiler option)
* Additional functions (sin,cos,1/x,1/sqrt,...)
* Comparison operators (<, <=, == ...)
* Right and left shifts + <<= and >>=
* Use of Posit16 routines for Posit8 (if ES8==ES16) to reduce library size
* Avoid 32-bit arithmetic for addition
* Avoid float casting for construction from integer
* Create custom byte storage format for 0..1 interval (probabilities) using Posit10,0

## Posit 0.1.2 - 2024.12.13 
* Additional functions (sin, cos, tan, atan, sign, abs, negate)
* Use of Posit16 routine for Posit8_2float

## Posit 0.1.1 - 2024.12.07 

* Posit16 <-> Posit8 translations
* Next and prior functions
* Sqrt for Posit8 and Posit 16 (different strategies, not always converging for Posit8)
* Operator overloading for +=, -=, *=, /=
* Now using 2's complement to store negative numbers

## Posit 0.1.0 - 2024.09.15

* Correction to meet Arduino library contribution rules
* Release and included in the official [list of Arduino libraries](https://docs.arduino.cc/libraries/)

## Posit 0.1.0rc1 - 2024.09.12

* Initial release
* test sketch example, README, CHANGELOG, ...
* Supports Posit8,0, Posit8,2 and Posit16,2
* Constructor functions from (raw) uint8_t or uint16_t
* Constructor from float32 (and int or double via float32 casting)
* Four ops : addition, substraction, multiplication, division with overloading
* Storage of negative numbers similar to IEEE754, no 2's complement


 
