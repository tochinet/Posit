## Posit Library for Arduino

This is an C/C++ library for posit8 and posit16 floating point arithmetic support in Arduino.

[Posit Arithmetic](https://posithub.org/docs/Posits4.pdf) was invented by John Gustafson. It is an alternative floating point format to IEEE 754 that promises a more efficient and balanced precision, especially useful for AI. Small posit numbers hare increased precision, while exact representation of big numbers are coarser. The [Posit Standard](https://posithub.org/docs/posit_standard-2.pdf) was released in 2022 and defines the storage format, operation behavior and required mathematical functions for Posits. It differs in some design choices from previous publications, and is only partially covered here, since not all decisions are equally applicable to the Arduino environment.

Posits can be any size from 2 to 32 bits or even more. Only 8-bit and 16-bit are considered in this library, and only the ATmega368 is targeted (UNO etc.).



No code was copied from any existing work, but some inspiration came from the [SoftPosit C reference library](https://gitlab.com/cerlane/SoftPosit), from section IV of the https://arxiv.org/pdf/2308.03425 paper (on division algorithms and rounding, leading to the conclusion that *rounding to nearest even* is not likely worth pursuing on Arduino), and from many other pages on the Internet (Quora, Stack Overflow, etc.).

### Status 
This library is a work in progress, and my first experience in creating an Arduino library _and_ publishing on GitHub, so expect errors and mistakes, stupid or not, and don't hesitate to contribute and propose correction and ameliorations. I tried to follow the [official guide](https://docs.arduino.cc/learn/contributions/). 
As with all WIP, expect many, frequent and breaking changes. Remember this is also a way for me to learn.

Today, the library allows to create posit8,0 objects (no  exponent bit) from raw signed byte, from int (16 bits), float32 and double (also 32 bits on Arduino platform), to convert a posit back to float, to add, subtract, multiply and divide two posits but ... long is the road.

Planned in coming iterations : overload of + - * / operators, previous/next, exponents in posit8, posit16, square root, ...

## Some explanations on Floats and Posits.
### IEEE 754 float representation
The standard Arduino library supports one type of floating point numbers, 32-bit IEEE 754. This is the most common standard for floating point calculations. A float consist of one bit sign (like signed integers), 8 bits exponent (power of two, biased by adding 127), and 23 bits of mantissa (the bits after the "1", that is not coded). There is no inversion of bits (2's complement) like for signed integers.

Expressing -10.5 in float requires the following steps :
1. coding the absolute value in binary (1010.1)
2. finding the power of 2 (3) and add 127 (total 130)
3. coding the sign, the exponent and the mantissa

Hence the 32-bit float representation of -10.5is 0b1***100 0001 0***010 1000 0000 0000 0000 0000 (with exponent bits bold and italicized). 

Funny enough many simple reals cannot be expressed exactly. For example 0.1 is rounded tp 0b0 0***011 1101 1***100 1100 1100 1100 1100 1101 (positive, power -4, mantissa 1.6000000238418579), or 0,1000000014901161. IEEE 754 also defines some exceptions such as +/- infinity (exponent 255, mantissa all zeros), two different zeros (+0 is all zeros and -0 is 1 followed by all zeros), subnormal numbers (very small), and different representations of Not a Number (NAN).

### Posit float representation
The major invention in the posit format was to add one extra variable-length field (called "regime") between the sign and the exponent. All regime bits are equal, and the first different bit marks the end of the regime field. In addition, the (fixed) number of exponent bits is considered an externally defined parameter. This means there are multiple, incompatible versions of posits depending on the es parameter. The posit standard imposes es=2.

<p align="center"><img src="posit_standard_format.png"><br>
Fig.1 : General Posit Format (from Posit Standard(2022))
</p>

Precision extension (for example from 8 to 16 bits) can be done simply by adding zeros at the end, expressing the very same numbers. Posit arithmetic implies that there is never underflow or overflow, only "rounding errors", hence in posit4,0 arithmetic, 4+4 is 4 and 0.5/4 is 0.25. 

Examples of very small posit numbers make it easier to understand the concept: posit2 (2 bits) can obviously only express 4 values. These are zero, one, minus one, and infinity. Each time a bit is added, one value is inserted between each value already in the set. So a 3-bit posit will also be able to express exactly +/- 2 and +/- 0.5 (assuming es=0, see further). Posit4 adds positive and negative values 1/4, 3/4, 3/2 and 4, and so on. 

The number of exponent bits between the regime and the mantissa field extends the limits of expressiveness, to the detriment of precision : posit3,1 numbers express exactly 4 and 0.25 instead of 2 and 0.5. Similarly, posit4,1 can express 0, 1/16, 1/4, 1/2, 1, 2, 4, 16 and infinity (and negatives).

### (Assumed) deviations from Standard
The main purpose of this library is to provide a more efficient alternative to the existing float arithmetic on constrained microcontrollers (ATmega328). 
This has several implications :
- Support of 32-bit Posits is not considered, because the increased precision (vs. float32) is a non-objective, and float64 doesn't exist.
- Posit8,0 is the first supported format although Posit8,2 is what the standard recommends. This is justified by simplicity.
- While the absence of overflow looks like a positive aspect of posits, but underflow to zero is likely desirable for IoT applications etc. Numbers smaller than 1E-6 (1ppm) are rounded down to zero by default, but this is parametrizable by defining ESPILON in your sketch before including the library.
- Rounding towards zero is preferred to "Rounding to nearest even" because it comes with no added complexity.
