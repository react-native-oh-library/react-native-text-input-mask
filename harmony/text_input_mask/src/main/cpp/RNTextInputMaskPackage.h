/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include "RNOH/Package.h"
#include "RNTextInputMask.h"

using namespace rnoh;
using namespace facebook;
namespace rnoh {
class RNTextInputMaskTurboModuleFactoryDelegate : public TurboModuleFactoryDelegate {
public:
    SharedTurboModule createTurboModule(Context ctx, const std::string &name) const override {
        if (name == "RNTextInputMask") {
            return std::make_shared<RNTextInputMask>(ctx, name);
        }
        return nullptr;
    }
};

class RNTextInputMaskPackage : public Package {
public:
    RNTextInputMaskPackage(Package::Context ctx) : Package(ctx) {}
    std::unique_ptr<TurboModuleFactoryDelegate> createTurboModuleFactoryDelegate() override {
        return std::make_unique<RNTextInputMaskTurboModuleFactoryDelegate>();
    }
    // std::vector<ArkTSMessageHandler::Shared> createArkTSMessageHandlers() override;
};
} // namespace rnoh