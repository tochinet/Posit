Documentation for the Posit library
===========
**Please be patient, this is my first experience with reST and Sphinx.**

.. This file is reSTuctured text, not MarkDown. This should be a reST comment

The `Arduino Posit Library <https://www.github.com/tochinet/Posit/>`_ provides an alternative to floating point numbers and doubles 
(although the 64-bit double format doesn't really exist in 8-bit Arduino cores)
using a *tapered* number system, meaning that the precision is higher around plus/minus one and lower for very low or very high powers of two.

The library provides 8-bit and 16-bit Posit numbers, and several operations and functions : 

* 4 arithmetic operations ( + - * / ) with operator overloading
* Square root (sqrt)
* Next and Previous values
* Trigonometric functions (work in progress)

Contributions are welcome.
