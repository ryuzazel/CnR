#include "Interval.h"

#include <algorithm>
#include <sstream>
#include <cmath>
#include <set>

namespace math {

static std::string fmt(double v) {
    if (std::isinf(v)) return v > 0 ? "+inf" : "-inf";
    std::ostringstream os;
    if (v == std::round(v)) os << (long long)std::llround(v);
    else os << v;
    return os.str();
}

std::string Interval::toString() const {
    std::ostringstream os;
    os << (loInclusive ? "[" : "(") << fmt(lo) << ", " << fmt(hi) << (hiInclusive ? "]" : ")");
    return os.str();
}

bool Set::contains(double x) const {
    for (double p : excludedPoints) if (std::abs(x - p) < 1e-12) return false;
    for (auto& iv : intervals) if (iv.contains(x)) return true;
    return false;
}

void Set::normalize() {
    std::sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) {
        return a.lo < b.lo;
    });
    std::vector<Interval> merged;
    for (auto& iv : intervals) {
        if (iv.empty()) continue;
        if (!merged.empty()) {
            Interval& last = merged.back();
            bool touching = iv.lo < last.hi || (iv.lo == last.hi && (last.hiInclusive || iv.loInclusive));
            if (touching) {
                if (iv.hi > last.hi) { last.hi = iv.hi; last.hiInclusive = iv.hiInclusive; }
                else if (iv.hi == last.hi) last.hiInclusive = last.hiInclusive || iv.hiInclusive;
                continue;
            }
        }
        merged.push_back(iv);
    }
    intervals = merged;

    std::sort(excludedPoints.begin(), excludedPoints.end());
    excludedPoints.erase(std::unique(excludedPoints.begin(), excludedPoints.end(),
                                      [](double a, double b) { return std::abs(a - b) < 1e-12; }),
                          excludedPoints.end());
    // drop excluded points that aren't actually inside any interval (redundant)
    std::vector<double> relevant;
    for (double p : excludedPoints) {
        for (auto& iv : intervals) {
            if (iv.contains(p)) { relevant.push_back(p); break; }
        }
    }
    excludedPoints = relevant;
}

Set Set::intersect(const Set& other) const {
    Set result;
    for (auto& a : intervals) {
        for (auto& b : other.intervals) {
            double lo = std::max(a.lo, b.lo);
            double hi = std::min(a.hi, b.hi);
            bool loInc = (a.lo > b.lo) ? a.loInclusive : (b.lo > a.lo) ? b.loInclusive : (a.loInclusive && b.loInclusive);
            bool hiInc = (a.hi < b.hi) ? a.hiInclusive : (b.hi < a.hi) ? b.hiInclusive : (a.hiInclusive && b.hiInclusive);
            Interval iv{lo, loInc, hi, hiInc};
            if (!iv.empty()) result.intervals.push_back(iv);
        }
    }
    for (double p : excludedPoints) result.excludedPoints.push_back(p);
    for (double p : other.excludedPoints) result.excludedPoints.push_back(p);
    result.normalize();
    return result;
}

Set Set::unite(const Set& other) const {
    Set result;
    result.intervals = intervals;
    for (auto& iv : other.intervals) result.intervals.push_back(iv);
    // excluded points only survive if excluded in BOTH sets (union removes a
    // hole if the other set covers it) — for our simple use-cases we just
    // keep points excluded in both, which is the conservative/simple choice.
    std::set<double> otherExcl(other.excludedPoints.begin(), other.excludedPoints.end());
    for (double p : excludedPoints) {
        if (otherExcl.count(p)) result.excludedPoints.push_back(p);
    }
    result.normalize();
    return result;
}

std::string Set::toString() const {
    if (intervals.empty()) return "\xe2\x88\x85"; // empty set symbol
    std::ostringstream os;
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (i) os << " \xe2\x88\xaa "; // union symbol
        os << intervals[i].toString();
    }
    if (!excludedPoints.empty()) {
        os << " minus {";
        for (size_t i = 0; i < excludedPoints.size(); ++i) {
            if (i) os << ", ";
            os << fmt(excludedPoints[i]);
        }
        os << "}";
    }
    return os.str();
}

} // namespace math
