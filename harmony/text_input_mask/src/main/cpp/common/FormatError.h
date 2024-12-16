/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
// 定义 FormatError 类
#include <string>

namespace TinpMask {
class FormatError : public std::exception {
public:
    // 构造函数接受错误消息
    explicit FormatError(const std::string &message = "An error occurred") : msg(message) {}

    // 重写 what() 方法，返回错误消息
    const char *what() const noexcept { return msg.c_str(); }

private:
    std::string msg; // 存储错误消息
};
} // namespace TinpMask