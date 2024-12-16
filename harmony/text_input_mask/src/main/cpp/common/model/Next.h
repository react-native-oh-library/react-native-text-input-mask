/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <memory>
#include "State.h"

namespace TinpMask {

class State;
class Next {
public:
    std::shared_ptr<State> state; // 存储状态的智能指针
    char insert;                  // 可插入的字符
    bool pass;                    // 是否通过
    char value;                   // 值字符，可选

    // 构造函数
    Next(std::shared_ptr<State> state, char insert, bool pass, char value)
        : state(state), insert(insert), pass(pass), value(value) {}
};
} // namespace TinpMask