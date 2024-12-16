/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include "CaretString.h"
#include "CaretStringIterator.h"

namespace TinpMask {

class RTLCaretStringIterator : public CaretStringIterator {
public:
    RTLCaretStringIterator(const CaretString& caretString)
        : CaretStringIterator(caretString) {}

    bool insertionAffectsCaret()  {
        return currentIndex <= caretString.caretPosition;
    }
};

}