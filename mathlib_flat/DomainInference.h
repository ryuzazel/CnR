#pragma once
//
// DomainInference.h — best-effort automatic domain inference for an Expr,
// by walking the tree and collecting constraints from sqrt/ln/division/
// inverse trig subexpressions. This is heuristic, not a full solver: it
// handles the common elementary cases well (polynomials, rational
// functions, sqrt/ln of polynomials, trig, inverse trig) and falls back to
// "all reals" (with a warning flag) for anything it can't reason about.

#include "Expr.h"
#include "Interval.h"

namespace math {

struct DomainResult {
    Set domain;
    bool exact;              // false if we had to bail out on some subexpression
    std::vector<std::string> notes; // human-readable explanation of constraints found
};

// Infers the natural domain of `e` treating `var` as the free variable and
// all other symbols as fixed parameters.
DomainResult inferDomain(const ExprPtr& e, const std::string& var = "x");

} // namespace math
