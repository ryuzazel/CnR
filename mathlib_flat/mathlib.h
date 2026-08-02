#pragma once
//
// mathlib.h — single include that pulls in the whole library.
//
//   #include "mathlib.h"
//   using namespace math;
//
//   Function f = Function::parse("f(x) = sqrt(x - 2)");
//   std::cout << f.domain().toString() << "\n";   // [2, +inf)

#include "Expr.h"
#include "Parser.h"
#include "Interval.h"
#include "DomainInference.h"
#include "Numeric.h"
#include "Function.h"
#include "LinearAlgebra.h" // Matrix — determinant, inverse, solve, rank, eigenvalues/eigenvectors
#include "math.h"    // roots() — polynomial root finder (pre-existing)
#include "factor.h"  // divisors() — integer divisor finder (pre-existing)
