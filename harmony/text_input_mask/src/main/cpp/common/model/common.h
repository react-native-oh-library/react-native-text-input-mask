/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include "CaretString.h"
#include <string>
#include <vector>
#include "State.h"
#include "Next.h"

namespace TinpMask {

class Result {
public:
    // 属性
    CaretString formattedText;
    std::string extractedValue;
    int affinity;
    bool complete;
    std::string tailPlaceholder;

    // 构造函数
    Result(const CaretString &formattedText, const std::string &extractedValue, int affinity, bool complete,
           const std::string &tailPlaceholder)
        : formattedText(formattedText), extractedValue(extractedValue), affinity(affinity), complete(complete),
          tailPlaceholder(tailPlaceholder) {}

    // reversed 方法
    Result reversed() const {
        return Result(this->formattedText.reversed(), reverseString(this->extractedValue), this->affinity,
                      this->complete, reverseString(this->tailPlaceholder));
    }

private:
    // 辅助函数，用于反转字符串
    std::string reverseString(const std::string &str) const { return std::string(str.rbegin(), str.rend()); }
};
/**
 * While scanning through the input string in the `.apply(…)` method, the mask builds a graph of
 * autocompletion steps.
 *
 * This graph accumulates the results of `.autocomplete()` calls for each consecutive ``State``,
 * acting as a `stack` of ``Next`` object instances.
 *
 * Each time the ``State`` returns `null` for its `.autocomplete()`, the graph resets empty.
 */
class AutocompletionStack {
private:
    std::vector<Next> stack;

public:
    // Push 方法
    std::optional<Next> push(const std::optional<Next> &item) {
        if (item.has_value()) {
            stack.push_back(item.value());
            return item;
        } else {
            clear();             // 清空栈
            return std::nullopt; // 返回 std::nullopt
        }
    }

    // 清空栈的方法
    void clear() { stack.clear(); }

    // Optional: 获取栈的元素（用于检查或调试）
    std::vector<Next> getStack() const {
        return stack; // 返回当前栈的拷贝
    }

    // Optional: 从栈中弹出一个元素
    std::optional<Next> pop() {
        if (stack.empty()) {
            return std::nullopt; // 栈为空时返回 std::nullopt
        }
        Next item = stack.back();
        stack.pop_back();
        return item;
    }

    // 检查栈是否为空
    bool isEmpty() const { return stack.empty(); }

    // Optional: 获取栈顶元素而不移除它
    std::optional<Next> peek() const {
        if (stack.empty()) {
            return std::nullopt; // 栈为空时返回 std::nullopt
        }
        return stack.back();
    }
};
} // namespace TinpMask