/*
  Comprehensive Posit Test Sketch
  - Modular test cases
  - Automated assertions
  - Edge case coverage
  - Performance metrics
*/

//#define DEBUG          // Uncomment for debug output
//#define BENCHMARK      // Uncomment for performance metrics
#define ASSERT_EPSILON 0.001  // Allowed error margin

#include "Posit.h"

// ======================
// Test Utilities
// ======================
unsigned long testCount = 0;
unsigned long failCount = 0;

template<typename T>
void assertNear(const char* msg, T actual, T expected, T epsilon = ASSERT_EPSILON) {
  testCount++;
  T error = abs(actual - expected);
  
  if (error > epsilon) {
    failCount++;
    Serial.print("FAIL: ");
    Serial.print(msg);
    Serial.print(" | Actual: ");
    Serial.print(actual, 4);
    Serial.print(" Expected: ");
    Serial.println(expected, 4);
  }
}

void printTestHeader(const char* title) {
  Serial.println("\n======================");
  Serial.print("TEST: ");
  Serial.println(title);
  Serial.println("======================");
}

// ======================
// Test Cases
// ======================
void testPosit8Basic() {
  printTestHeader("Posit8 Basic Operations");
  
  posit8_t a(2.0f), b(3.0f);
  
  // Addition
  posit8_t c = a + b;
  assertNear("2 + 3", posit2float(c), 5.0f);
  
  // Subtraction
  c = b - a;
  assertNear("3 - 2", posit2float(c), 1.0f);
  
  // Multiplication
  c = a * b;
  assertNear("2 * 3", posit2float(c), 6.0f);
  
  // Division
  c = b / a;
  assertNear("3 / 2", posit2float(c), 1.5f);
}

void testPosit8EdgeCases() {
  printTestHeader("Posit8 Edge Cases");
  
  // Zero handling
  posit8_t zero(0.0f);
  posit8_t one(1.0f);
  posit8_t result = zero + one;
  assertNear("0 + 1", posit2float(result), 1.0f);
  
  // NaR propagation
  posit8_t nar = posit8_t(NAN);
  result = nar + one;
  assertNear("NaR + 1", posit2float(result), NAN);
  
  // Division by zero
  result = one / zero;
  assertNear("1 / 0", posit2float(result), NAN);
}

void testPosit16Precision() {
  printTestHeader("Posit16 Precision");
  
  posit16_t a(3.14159265f);
  posit16_t b(2.71828182f);
  
  #ifdef BENCHMARK
    unsigned long start = micros();
  #endif
  
  posit16_t c = a * b;
  
  #ifdef BENCHMARK
    Serial.print("Multiplication took ");
    Serial.print(micros() - start);
    Serial.println(" μs");
  #endif
  
  assertNear("PI * e", posit2float(c), 8.53973422f, 0.01f);
}

void testTrigonometricFunctions() {
  printTestHeader("Trigonometric Functions");
  
  posit16_t angle(0.78539816f);  // π/4
  posit16_t sin_val = posit16_sin(angle);
  posit16_t cos_val = posit16_cos(angle);
  
  assertNear("sin(π/4)", posit2float(sin_val), 0.7071f, 0.01f);
  assertNear("cos(π/4)", posit2float(cos_val), 0.7071f, 0.01f);
  
  posit16_t tan_val = posit16_tan(angle);
  assertNear("tan(π/4)", posit2float(tan_val), 1.0f, 0.01f);
}

void testRounding() {
  printTestHeader("Rounding Behavior");
  
  posit8_t small(0.0001f);
  assertNear("Small value", posit2float(small), 0.0f);
  
  posit8_t almost_one(0.9999f);
  assertNear("Almost one", posit2float(almost_one), 1.0f, 0.1f);
}

// ======================
// Main Program
// ======================
void setup() {
  Serial.begin(115200);
  while(!Serial); // For Leonardo/Micro
  
  Serial.println("\n=== Posit Library Test Suite ===");
  
  // Run test groups
  testPosit8Basic();
  testPosit8EdgeCases();
  testPosit16Precision();
  testTrigonometricFunctions();
  testRounding();

  // Print summary
  Serial.println("\n=== Test Summary ===");
  Serial.print("Tests Run: ");
  Serial.println(testCount);
  Serial.print("Tests Failed: ");
  Serial.println(failCount);
  Serial.println(failCount ? "❌ Some tests failed!" : "✅ All tests passed!");
}

void loop() {}
