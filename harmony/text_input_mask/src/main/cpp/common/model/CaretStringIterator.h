/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <string>
#include <memory>
#include <string>
#include <memory>
#include "CaretString.h"

namespace TinpMask {

class CaretStringIterator {
protected:
    CaretString caretString; // 关联的 CaretString 对象
    int currentIndex;        // 当前索引

public:
    // 构造函数
    CaretStringIterator(const CaretString &caretStr, int index = 0) : caretString(caretStr), currentIndex(index) {}

    // 插入是否影响光标位置
    virtual bool insertionAffectsCaret() {

        if (dynamic_cast<CaretString::Backward *>(caretString.caretGravity.get())) {
            return currentIndex < caretString.getCaretPosition();
        } else if (dynamic_cast<CaretString::Forward *>(caretString.caretGravity.get())) {
            return currentIndex <= caretString.getCaretPosition() ||
                   (currentIndex == 0 && caretString.getCaretPosition() == 0);
        }
        return false;
    }

    // 删除是否影响光标位置
    virtual bool deletionAffectsCaret() { return currentIndex < caretString.getCaretPosition(); }

    /**
     * 遍历 CaretString.string
     * @postcondition: 迭代器位置移到下一个符号。
     * @returns 当前符号。如果迭代器到达字符串末尾，返回 nullptr。
     */
    virtual char next() {
        if (currentIndex >= caretString.getString().length()) {
            return '\0'; // 返回空指针表示到达字符串末尾
        }

//         char *charPtr = new char;                         // 创建一个字符指针
        auto charPtr = caretString.getString()[currentIndex]; // 获取当前字符
        currentIndex += 1;                                // 移动到下一个索引
        return charPtr;                                   // 返回当前字符
    }
};


} // namespace TinpMask
