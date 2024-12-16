/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include "common/Mask.h"
#include "common/model/CaretString.h"
#include "common/model/Notation.h"
#include "common/model/common.h"
namespace TinpMask {
class RTLMask : public Mask   {
    
public:
     static std::unordered_map<std::string, std::shared_ptr<RTLMask>> cache ;
    RTLMask(const std::string& format, const std::vector<Notation>& customNotations)
        : Mask(reversedFormat(format), customNotations) {}

    static std::shared_ptr<RTLMask> getOrCreate(const std::string& format, const std::vector<Notation>& customNotations) {
        std::string reversed = reversedFormat(format);
        auto it = cache.find(reversed);
        if (it != cache.end()) {
            return it->second; // Return cached instance
        }

        // Create new instance and cache it
        auto newMask = std::make_shared<RTLMask>(format, customNotations);
        cache[reversed] = newMask;
        return newMask;
    }

    Result apply(const CaretString& text) override {
        return Mask::apply(text.reversed()).reversed(); // Assuming the Result class has a reversed method
    }
    

private:
   

    static std::string reversedFormat(const std::string& format) {
        std::string reversed = std::string(format.rbegin(), format.rend());
        
        // Replace logic (equivalent to Kotlin's replace)
        size_t pos = 0;
        while ((pos = reversed.find("\\[", pos)) != std::string::npos) {
            reversed.replace(pos, 2, "\\");
            pos += 1; // Advance position to prevent infinite loop
        }
        pos = 0;
        while ((pos = reversed.find("]\\", pos)) != std::string::npos) {
            reversed.replace(pos, 2, "\\[");
            pos += 1;
        }
        pos = 0;
        while ((pos = reversed.find("\\{", pos)) != std::string::npos) {
            reversed.replace(pos, 2, "\\}");
            pos += 1;
        }
        pos = 0;
        while ((pos = reversed.find("}\\", pos)) != std::string::npos) {
            reversed.replace(pos, 2, "\\{");
            pos += 1;
        }

        // Map logic for brackets
        for (char& ch : reversed) {
            switch (ch) {
                case '[': ch = ']'; break;
                case ']': ch = '['; break;
                case '{': ch = '}'; break;
                case '}': ch = '{'; break;
                default: break;
            }
        }
        return reversed;
    }
};
}