#include "factor.h"

#include <algorithm>
#include <cstdlib>

namespace math
{

std::vector<std::int64_t> divisors(std::int64_t number)
{
    std::vector<std::int64_t> result;

    if (number == 0)
        return result;

    number = std::llabs(number);

    for (std::int64_t i = 1; i * i <= number; ++i)
    {
        if (number % i != 0)
            continue;

        // ±i
        result.push_back(i);
        result.push_back(-i);

        // ±(number / i)
        std::int64_t other = number / i;

        if (other != i)
        {
            result.push_back(other);
            result.push_back(-other);
        }
    }

    std::sort(result.begin(), result.end());

    return result;
}

}