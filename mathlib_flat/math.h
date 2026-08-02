#pragma once

#include <vector>
#include <complex>

namespace math
{

std::vector<std::complex<double>> roots(
    const std::vector<double>& coefficients
);

}