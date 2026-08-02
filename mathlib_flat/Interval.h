#pragma once
//
// Interval.h — represents subsets of R as a finite union of intervals,
// used for domains and images. Endpoints can be +-infinity.
//
// This is intentionally simple (no fancy CAS interval arithmetic): it's
// enough to represent "R minus a few points", "[a,b]", "(-inf, 0) U (0, inf)"
// etc., which covers the domain/image needs of elementary functions.

#include <vector>
#include <string>
#include <limits>
#include <cmath>

namespace math {

struct Bound {
    double value;
    bool inclusive;   // true: <=, false: <
    bool isInf() const { return std::isinf(value); }
};

struct Interval {
    double lo;
    bool loInclusive;
    double hi;
    bool hiInclusive;

    static Interval all() {
        return {-std::numeric_limits<double>::infinity(), false,
                 std::numeric_limits<double>::infinity(), false};
    }
    static Interval point(double v) { return {v, true, v, true}; }
    static Interval closed(double a, double b) { return {a, true, b, true}; }
    static Interval open(double a, double b) { return {a, false, b, false}; }
    static Interval fromLeft(double a, bool inc = true) {
        return {a, inc, std::numeric_limits<double>::infinity(), false};
    }
    static Interval toRight(double b, bool inc = true) {
        return {-std::numeric_limits<double>::infinity(), false, b, inc};
    }

    bool contains(double x) const {
        bool okLo = loInclusive ? (x >= lo) : (x > lo);
        bool okHi = hiInclusive ? (x <= hi) : (x < hi);
        return okLo && okHi;
    }

    bool empty() const { return lo > hi || (lo == hi && !(loInclusive && hiInclusive)); }

    std::string toString() const;
};

// A domain/image = union of disjoint (after normalize()) intervals, plus
// optionally a set of explicitly excluded points (for "R minus {0, 2}"-style
// domains, which are common and annoying to express as interval unions).
class Set {
public:
    std::vector<Interval> intervals;
    std::vector<double> excludedPoints; // punched holes within `intervals`

    static Set empty() { return Set{}; }
    static Set reals() { return Set{{Interval::all()}, {}}; }
    static Set fromInterval(Interval iv) { return Set{{iv}, {}}; }

    Set& exclude(double v) { excludedPoints.push_back(v); return *this; }

    bool contains(double x) const;

    // intersect this with another set (used to combine constraints:
    // sqrt needs x>=0 AND ln needs inner>0, etc.)
    Set intersect(const Set& other) const;
    Set unite(const Set& other) const;

    std::string toString() const;

    void normalize(); // sort/merge overlapping intervals, dedupe excluded points
};

} // namespace math
