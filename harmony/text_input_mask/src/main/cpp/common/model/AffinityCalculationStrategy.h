#pragma once
#include <string>
#include <stdexcept>
#include <limits>
#include "../Mask.h"     // 确保包含 Mask 头文件
#include "CaretString.h" // 确保包含 CaretString 头文件

namespace TinpMask {

enum class AffinityCalculationStrategy { WHOLE_STRING, PREFIX, CAPACITY, EXTRACTED_VALUE_CAPACITY };

class AffinityCalculator {
public:
    static int calculateAffinityOfMask(AffinityCalculationStrategy strategy,  Mask &mask,
                                       const CaretString &text) {
        switch (strategy) {
        case AffinityCalculationStrategy::WHOLE_STRING:
            return mask.apply(text).affinity;

        case AffinityCalculationStrategy::PREFIX:
            return prefixIntersection(mask.apply(text).formattedText.string, text.string).length();

        case AffinityCalculationStrategy::CAPACITY:
            return text.string.length() > mask.totalTextLength() ? std::numeric_limits<int>::min()
                                                                 : text.string.length() - mask.totalTextLength();

        case AffinityCalculationStrategy::EXTRACTED_VALUE_CAPACITY: {
            const auto &extractedValue = mask.apply(text).extractedValue;
            return extractedValue.length() > mask.totalValueLength()
                       ? std::numeric_limits<int>::min()
                       : extractedValue.length() - mask.totalValueLength();
        }
        default:
            throw std::invalid_argument("Unknown AffinityCalculationStrategy");
        }
    }

private:
    // Helper function to find prefix intersection
    static std::string prefixIntersection(const std::string &str1, const std::string &str2) {
        size_t endIndex = 0;
        while (endIndex < str1.length() && endIndex < str2.length()) {
            if (str1[endIndex] == str2[endIndex]) {
                endIndex += 1;
            } else {
                return str1.substr(0, endIndex);
            }
        }
        return str1.substr(0, endIndex);
    }
};
} // namespace TinpMask