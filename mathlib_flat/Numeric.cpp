#include "Numeric.h"

#include <cmath>
#include <stdexcept>

namespace math {

static bool sameSignBool(double a, double b) { return (a > 0) == (b > 0); }

std::optional<double> bisect(const RealFn& f, double lo, double hi, int maxIter, double tol) {
    double flo, fhi;
    try { flo = f(lo); } catch (...) { return std::nullopt; }
    try { fhi = f(hi); } catch (...) { return std::nullopt; }
    if (std::isnan(flo) || std::isnan(fhi)) return std::nullopt;
    if (flo == 0.0) return lo;
    if (fhi == 0.0) return hi;
    if ((flo > 0) == (fhi > 0)) return std::nullopt; // no sign change

    for (int i = 0; i < maxIter; ++i) {
        double mid = 0.5 * (lo + hi);
        double fmid;
        try { fmid = f(mid); } catch (...) { return std::nullopt; }
        if (std::isnan(fmid)) return std::nullopt;
        if (std::abs(fmid) < tol || (hi - lo) < tol) return mid;
        if ((fmid > 0) == (flo > 0)) { lo = mid; flo = fmid; }
        else { hi = mid; fhi = fmid; }
    }
    return 0.5 * (lo + hi);
}

std::optional<double> newton(const RealFn& f, const RealFn& fprime, double x0, int maxIter, double tol) {
    double x = x0;
    for (int i = 0; i < maxIter; ++i) {
        double fx, dfx;
        try { fx = f(x); dfx = fprime(x); } catch (...) { return std::nullopt; }
        if (std::isnan(fx) || std::isnan(dfx)) return std::nullopt;
        if (std::abs(fx) < tol) return x;
        if (std::abs(dfx) < 1e-14) return std::nullopt;
        double next = x - fx / dfx;
        if (std::isnan(next) || std::isinf(next)) return std::nullopt;
        if (std::abs(next - x) < tol) return next;
        x = next;
    }
    return std::nullopt;
}

std::vector<double> findRootsScan(const RealFn& f, double lo, double hi, int steps) {
    std::vector<double> roots;
    double step = (hi - lo) / steps;
    double prevX = lo;
    double prevY;
    bool havePrev = false;
    try { prevY = f(prevX); havePrev = !std::isnan(prevY); } catch (...) { havePrev = false; }

    for (int i = 1; i <= steps; ++i) {
        double x = lo + step * i;
        double y;
        bool ok = true;
        try { y = f(x); if (std::isnan(y)) ok = false; } catch (...) { ok = false; }

        if (havePrev && ok) {
            if (prevY == 0.0) {
                roots.push_back(prevX);
            } else if ((prevY > 0) != (y > 0)) {
                auto r = bisect(f, prevX, x);
                if (r) roots.push_back(*r);
            }
        }
        if (ok) { prevX = x; prevY = y; havePrev = true; }
        else havePrev = false;
    }
    return roots;
}

double numericDerivative(const RealFn& f, double x, double h) {
    // central difference
    return (f(x + h) - f(x - h)) / (2 * h);
}

static double simpson(const RealFn& f, double a, double b) {
    double c = (a + b) / 2;
    return (b - a) / 6.0 * (f(a) + 4 * f(c) + f(b));
}

static double adaptiveSimpson(const RealFn& f, double a, double b, double whole, double tol, int depth) {
    double c = (a + b) / 2;
    double left = simpson(f, a, c);
    double right = simpson(f, c, b);
    if (depth <= 0 || std::abs(left + right - whole) <= 15 * tol) {
        return left + right + (left + right - whole) / 15.0;
    }
    return adaptiveSimpson(f, a, c, left, tol / 2, depth - 1) +
           adaptiveSimpson(f, c, b, right, tol / 2, depth - 1);
}

double numericIntegral(const RealFn& f, double a, double b, double tol) {
    if (a == b) return 0.0;
    if (a > b) return -numericIntegral(f, b, a, tol);
    double whole = simpson(f, a, b);
    return adaptiveSimpson(f, a, b, whole, tol, 30);
}

LimitResult numericLimit(const RealFn& f, double at, int direction) {
    auto tryEval = [&](double x) -> std::optional<double> {
        try {
            double y = f(x);
            if (std::isnan(y)) return std::nullopt;
            return y;
        } catch (...) {
            return std::nullopt;
        }
    };

    std::vector<double> steps = {1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
    std::vector<double> leftVals, rightVals;
    for (double h : steps) {
        if (direction <= 0) { auto v = tryEval(at - h); if (v) leftVals.push_back(*v); }
        if (direction >= 0) { auto v = tryEval(at + h); if (v) rightVals.push_back(*v); }
    }

    auto converged = [](const std::vector<double>& vals, double& outVal, bool& outInf, double& outSign) {
        if (vals.size() < 3) return false;
        double last = vals.back();

        // Blow-up detection: magnitude should be monotonically growing as h
        // shrinks (i.e. as we approach the limit point), all with the same
        // sign, and the last value should already be "large" in an absolute
        // sense relative to where we started. This catches 1/x-style poles
        // without needing values to reach an arbitrary fixed cutoff like 1e8.
        bool growing = true;
        bool sameSign = true;
        for (size_t i = 1; i < vals.size(); ++i) {
            if (std::abs(vals[i]) < std::abs(vals[i-1]) * 1.5) growing = false;
            if ((vals[i] > 0) != (vals.front() > 0)) sameSign = false;
        }
        if (growing && sameSign && std::abs(last) > 1e3) {
            outInf = true;
            outSign = last > 0 ? 1.0 : -1.0;
            return true;
        }

        // check the last few values are converging to a finite value
        double diff1 = std::abs(vals[vals.size()-1] - vals[vals.size()-2]);
        double diff2 = std::abs(vals[vals.size()-2] - vals[vals.size()-3]);
        if (diff1 < 1e-4 && diff1 <= diff2 * 1.5) {
            outVal = vals.back();
            outInf = false;
            return true;
        }
        return false;
    };

    double lv = 0, rv = 0, ls = 0, rs = 0;
    bool linf = false, rinf = false;
    bool lok = direction > 0 ? true : converged(leftVals, lv, linf, ls);
    bool rok = direction < 0 ? true : converged(rightVals, rv, rinf, rs);

    if (direction < 0) return lok ? LimitResult{true, lv, linf, ls} : LimitResult{false, 0, false, 0};
    if (direction > 0) return rok ? LimitResult{true, rv, rinf, rs} : LimitResult{false, 0, false, 0};

    if (!lok || !rok) return LimitResult{false, 0, false, 0};
    if (linf && rinf && sameSignBool(ls, rs)) return LimitResult{true, 0, true, ls};
    if (linf || rinf) return LimitResult{false, 0, false, 0}; // one side infinite, other finite: no two-sided limit
    if (std::abs(lv - rv) < 1e-4) return LimitResult{true, (lv + rv) / 2, false, 0};
    return LimitResult{false, 0, false, 0};
}

LimitResult numericLimitAtInfinity(const RealFn& f, int direction) {
    std::vector<double> xs;
    double sign = direction >= 0 ? 1.0 : -1.0;
    for (double m : {1e2, 1e3, 1e4, 1e5, 1e6, 1e7}) xs.push_back(sign * m);

    std::vector<double> vals;
    for (double x : xs) {
        try {
            double y = f(x);
            if (!std::isnan(y)) vals.push_back(y);
        } catch (...) {}
    }
    if (vals.size() < 3) return LimitResult{false, 0, false, 0};

    double last = vals.back();
    if (std::abs(last) > 1e8) {
        bool allSame = true;
        for (double v : vals) if ((v > 0) != (last > 0)) allSame = false;
        if (allSame) return LimitResult{true, 0, true, last > 0 ? 1.0 : -1.0};
    }
    double diff1 = std::abs(vals[vals.size()-1] - vals[vals.size()-2]);
    double diff2 = std::abs(vals[vals.size()-2] - vals[vals.size()-3]);
    if (diff1 < 1e-4 && diff1 <= diff2 * 1.5 + 1e-12) {
        return LimitResult{true, vals.back(), false, 0};
    }
    return LimitResult{false, 0, false, 0};
}

} // namespace math
