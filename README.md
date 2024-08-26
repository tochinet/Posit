## Posit Library for Arduino

This is an C/C++ library for posit8 and posit16 floating point arithmetic support in Arduino.

[Posit Arithmetic](https://posithub.org/docs/Posits4.pdf) was invented by John Gustafson. It is an alternative floating point format to IEEE 754 that promises a more efficient and balanced precision, especially useful for AI. Small posit numbers hare increased precision, while exact representation of big numbers are coarser. The [Posit Standard](https://posithub.org/docs/posit_standard-2.pdf) was released in 2022 and defines the storage format, operation behavior and required mathematical functions for Posits. It differs in some design choices from previous publications, and is only partially covered here, since not all decisions are equally applicable to the Arduino environment.

Posits can be any size from 2 to 32 bits or even more. Only 8-bit and 16-bit are considered in this library.

Precision extension (for example from 8 to 16 bits) can be done simply by adding zeros at the end, expressing the very same numbers. Setting some bits in the extension selects intermediate additional numbers. Posit arithmetic implies that there is never underflow or overflow, only "rounding errors", hence in posit4,0 arithmetic, 4+4 is 4 and 0.5/4 is 0.25. 

No code was coped from any existing work, but some inspiration came from the [SoftPosit C reference library](https://gitlab.com/cerlane/SoftPosit), from section IV of the https://arxiv.org/pdf/2308.03425 paper (on division algorithms and rounding, leading to the conclusion that *rounding to nearest even* is not worth pursuing on Arduino, and many other pages on the Internet (Quora, Stack Overflow, etc.).

This library is a work in progress, don't hesitate if you want to contribute. It also is my first experience in creating an Arduino library, so expect errors and mistakes, and help correct them. I tried to follow the [official guide](https://docs.arduino.cc/learn/contributions/) as much as I can.
As all WIP, expect many, frequent and breaking changes. Remember this is also a way for me to learn.

Today, it is possible to create posit8,0 (no  exponent bit) from raw signed byte, from int (16 bits), float32 and double (also 32 bits on Arduino platform), to convert a posit back to float, to add, substract and multiply posits.

Still TODO : overload of + - * / operators, making division work, previous/next, posit16, ...

## Some explanations on Floats and Posits.
### IEEE 754 float representation
The standard Arduino library supports one type of floating point numbers, 32-bit IEEE 754. This is the most common standard for floating point calculations. A float consist of one bit sign (like signed integers), 8 bits exponent (power of two, biased by adding 127), and 23 bits of mantissa (the bits after the "1", that is not coded). There is no inversion of bits (2's complement) like for signed integers.

Expressing -10.5 in float requires the following steps :
1. coding the absolute value in binary (1010.1)
2. finding the power of 2 (3) and add 127 (total 130)
3. coding the sign, the exponent and the mantissa

Hence the 32-bit float representation of -10.5is 0b1***100 0001 0***010 1000 0000 0000 0000 0000 (with exponent bits bold and italicized). 

Funny enough many simple reals cannot be expressed exactly. For example 0.1 is rounded tp 0b0 0***011 1101 1***100 1100 1100 1100 1100 1101 (positive, power -4, mantissa 1.6000000238418579), or 0,1000000014901161. There are also some special cases such as +/- infinity (exponent 255, mantissa all zeros) and two different zeros (+0 is all zeros and -0 is 1 followed by all zeros)

### Posit float representation
The posit concept adds one extra variable-length field ("regime") between the sign and the exponent. All regime bits are equal, and a different bit makes the end of the regime field. In addition, the (fixed) number of exponent bits is considered an externally defined parameter. This means there are multiple, incompatible versions of posits dependin on the es parameter.

Very small sizes are useful to understand the concept : posit2 (2 bits) can express zero, one, minus one and infinity. A 3-bit posit will also be able to express +/- 2 and +/- 0.5 (if es=0). Posit4 (again with zeroed es parameter) adds positive and negative values 1/4, 3/4, 3/2 and 4, and so on. Adding one or more exponent bits extends the limits of expressiveness, to the detriment of precision : posit3,1 numbers express exactly 4 and 0.25 instead of 2 and 0.5. Similarly, posit4,1 can express 0, 1/16, 1/4, 1/2, 1, 2, 4, 16 and infinity (and negatives).
