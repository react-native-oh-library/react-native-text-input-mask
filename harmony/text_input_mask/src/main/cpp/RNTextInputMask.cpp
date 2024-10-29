#pragma once
#include "RNTextInputMask.h"
#include "RNOH/arkui/TextInputNode.h"
#include "RNOH/ComponentInstance.h"
#include "RNOH/RNInstanceCAPI.h"
#include "RNOHCorePackage/ComponentInstances/TextInputComponentInstance.h"
#include "common/model/AffinityCalculationStrategy.h"

#include <cstdint>
#include <iostream>
#include <jsi/jsi.h>
#include <string>
#include <thread>

using namespace facebook;
using namespace rnoh;
using namespace react;
using namespace TinpMask;
static constexpr int AVOIDENCE = 1;
std::unordered_map<std::string, std::shared_ptr<RTLMask>> TinpMask::RTLMask::cache;
std::unordered_map<std::string, std::shared_ptr<Mask>> TinpMask::Mask::MaskFactory::maskCache;
void maybeThrow(int32_t status) {
    DLOG(INFO) << "=====text change maybeThrow status: " << status;
    if (status != 0) {
        auto message = std::string("ArkUINode operation failed with status: ") + std::to_string(status);
        LOG(ERROR) << message;
        throw std::runtime_error(std::move(message));
    }
}

void myEventReceiver(ArkUI_NodeEvent *event) {
    auto self = reinterpret_cast<RNTextInputMask *>(OH_ArkUI_NodeEvent_GetUserData(event));
    if (self == nullptr) {
        return;
    }
    int32_t eventId = OH_ArkUI_NodeEvent_GetTargetId(event);
    ArkUI_NodeHandle textNode = OH_ArkUI_NodeEvent_GetNodeHandle(event);
    void *data = NativeNodeApi::getInstance()->getUserData(textNode);
    auto item = NativeNodeApi::getInstance()->getAttribute(textNode, NODE_TEXT_INPUT_TEXT);
    std::string content = item->string;
    UserData *userData = reinterpret_cast<UserData *>(data);
    bool isDelete = userData->lastInputText.size() > content.size();
    bool useAutocomplete = !isDelete ? userData->maskOptions.autocomplete.value() : false;
    bool useAutoskip = isDelete ? userData->maskOptions.autoskip.value() : false;

    // onChange 事件
    if (eventId == 110) {
        std::shared_ptr<CaretString::CaretGravity> caretGravity =
            isDelete ? std::make_shared<CaretString::CaretGravity>(CaretString::Backward(useAutoskip))
                     : std::make_shared<CaretString::CaretGravity>(CaretString::Forward(useAutocomplete));
        CaretString text(content, content.length(), caretGravity);
        try {
            auto maskObj = self->pickMask(text, userData->maskOptions, userData->primaryFormat);
            auto result = maskObj->apply(text);
            std::string resultString = result.formattedText.string;
            DLOG(INFO) << "mask result complete: " << result.complete;
            userData->lastInputText = resultString;
            std::string finalString = isDelete ? content : resultString;
            ArkUI_AttributeItem item{.string = finalString.c_str()};
            userData->lastInputText = finalString;
            maybeThrow(NativeNodeApi::getInstance()->setAttribute(userData->data, NODE_TEXT_INPUT_TEXT, &item));
        } catch (FormatError e) {
            DLOG(ERROR) << " mask compiler error " << e.what();
        }
    }
    // onFocus 事件
    if (eventId == 111) {
        if (userData->maskOptions.autocomplete.value()) {
            std::string text = "";
            text += content;
            try {
                CaretString string(text, text.length(),
                                   std::make_shared<CaretString::Forward>(userData->maskOptions.autocomplete.value()));
                auto maskObj = self->pickMask(string, userData->maskOptions, userData->primaryFormat);
                std::string resultString = maskObj.get()->apply(string).formattedText.string;
                ArkUI_AttributeItem item{.string = resultString.c_str()};
                maybeThrow(NativeNodeApi::getInstance()->setAttribute(userData->data, NODE_TEXT_INPUT_TEXT, &item));
            } catch (FormatError e) {
                DLOG(ERROR) << " mask compiler error " << e.what();
            }
        }
    }
}
// 计算亲和度
int calculateAffinity(Mask mask, const CaretString &text, std::string affinityCalculationStrategy) {
    AffinityCalculationStrategy strategy;
    if (affinityCalculationStrategy == "WHOLE_STRING") {
        strategy = AffinityCalculationStrategy::WHOLE_STRING;
    } else if (affinityCalculationStrategy == "PREFIX") {
        strategy = AffinityCalculationStrategy::PREFIX;
    } else if (affinityCalculationStrategy == "CAPACITY") {
        strategy = AffinityCalculationStrategy::CAPACITY;
    } else {
        strategy = AffinityCalculationStrategy::EXTRACTED_VALUE_CAPACITY;
    }
    int affinity = AffinityCalculator::calculateAffinityOfMask(strategy, mask, text);

    return affinity;
}
// 获取或创建 Mask
std::shared_ptr<Mask> maskGetOrCreate(const std::string &format, const std::vector<Notation> &customNotations,
                                      bool rightToLeft) {
    if (rightToLeft) {
        return RTLMask::getOrCreate(format, customNotations);
    } else {
        return Mask::MaskFactory::getOrCreate(format, customNotations);
    }
}

std::shared_ptr<Mask> RNTextInputMask::pickMask(const CaretString &text, MaskOptions maskOptions,
                                                std::string primaryMask) {
    // 如果 affineFormats 为空，直接返回 primaryMask
    if (maskOptions.affineFormats->size() <= 0) {
        auto mask = maskGetOrCreate(primaryMask, maskOptions.customNotations.value(), maskOptions.rightToLeft.value());
        int affinity = calculateAffinity(*mask, text, maskOptions.affinityCalculationStrategy.value());
        DLOG(INFO) << " ======= pickMask calculateAffinity value： " << affinity
                   << "\n affinityCalculationStrategy: " << maskOptions.affinityCalculationStrategy.value();
        return mask;
    }
    // 定义 MaskAffinity 结构体，用于存储 Mask 和相应的亲和度
    struct MaskAffinity {
        Mask mask;    // Mask 对象
        int affinity; // 亲和度
        MaskAffinity(const Mask &m, int a) : mask(m), affinity(a) {}
    };

    // 计算 primaryMask 的亲和度
    int primaryAffinity = calculateAffinity(primaryMask, text, maskOptions.affinityCalculationStrategy.value());
    DLOG(INFO) << " ======= pickMask calculateAffinity value： " << primaryAffinity << " \n mask: " << primaryMask
               << "\n affinityCalculationStrategy: " << maskOptions.affinityCalculationStrategy.value();
    // 存储所有 mask 和亲和度的列表
    std::vector<MaskAffinity> masksAndAffinities;

    // 遍历 affineFormats 以获取每个格式的 Mask 和亲和度
    for (const auto &format : *maskOptions.affineFormats) {
        std::shared_ptr<Mask> mask =
            maskGetOrCreate(format, maskOptions.customNotations.value(), maskOptions.rightToLeft.value());
        int affinity = calculateAffinity(*mask, text, maskOptions.affinityCalculationStrategy.value());
        DLOG(INFO) << " ======= pickMask calculateAffinity value： " << affinity << "\n affineFormat: " << format
                   << "\n affinityCalculationStrategy: " << maskOptions.affinityCalculationStrategy.value();
        masksAndAffinities.emplace_back(*mask, affinity);
    }

    // 按亲和度降序排序
    std::sort(masksAndAffinities.begin(), masksAndAffinities.end(),
              [](const MaskAffinity &a, const MaskAffinity &b) { return a.affinity > b.affinity; });

    // 寻找插入位置
    int insertIndex = -1;
    for (size_t index = 0; index < masksAndAffinities.size(); ++index) {
        if (primaryAffinity >= masksAndAffinities[index].affinity) {
            insertIndex = static_cast<int>(index);
            break;
        }
    }

    // 插入 primaryMask
    if (insertIndex >= 0) {
        masksAndAffinities.insert(masksAndAffinities.begin() + insertIndex, MaskAffinity(primaryMask, primaryAffinity));
    } else {
        masksAndAffinities.emplace_back(primaryMask, primaryAffinity);
    }

    // 返回亲和度最高的 Mask
    return std::make_shared<Mask>(masksAndAffinities.front().mask);
}

void RNTextInputMask::setMask(int reactNode, std::string primaryFormat, MaskOptions maskOptions) {
    auto task = [this, reactNode, primaryFormat, maskOptions] {
        auto weakInstance = m_ctx.instance;
        auto instance = weakInstance.lock();
        auto instanceCAPI = std::dynamic_pointer_cast<RNInstanceCAPI>(instance);
        if (!instanceCAPI) {
            return;
        }
        auto componentInstance = instanceCAPI->findComponentInstanceByTag(reactNode);
        if (!componentInstance) {
            return;
        }
        auto input = std::dynamic_pointer_cast<TextInputComponentInstance>(componentInstance);
        if (!input) {
            throw std::runtime_error("find ComponentInstance failed,check the reactNode is Valid ");
        }
        ArkUINode &node = input->getLocalRootArkUINode();
        TextInputNode *textInputNode = dynamic_cast<TextInputNode *>(&node);
        NativeNodeApi::getInstance()->registerNodeEvent(textInputNode->getArkUINodeHandle(), NODE_TEXT_INPUT_ON_CHANGE,
                                                        110, this);
        NativeNodeApi::getInstance()->registerNodeEvent(textInputNode->getArkUINodeHandle(), NODE_ON_FOCUS, 111, this);

        UserData *userData = new UserData({.data = textInputNode->getArkUINodeHandle(),
                                           .maskOptions = maskOptions,
                                           .primaryFormat = primaryFormat,
                                           .node = reactNode});
        this->m_userDatas.insert(userData);
        NativeNodeApi::getInstance()->setUserData(textInputNode->getArkUINodeHandle(), userData);
        NativeNodeApi::getInstance()->addNodeEventReceiver(textInputNode->getArkUINodeHandle(), myEventReceiver);
    };
    this->m_ctx.taskExecutor->runTask(TaskThread::MAIN, std::move(task));

}
std::string getString(std::string maskValue, std::string value, bool autocomplete, bool isMask) {

    auto maskObj = Mask::MaskFactory::getOrCreate(maskValue, {});
    CaretString text(value, value.length(), std::make_shared<CaretString::Forward>(autocomplete));
    auto r = maskObj->apply(text);
    std::string result;
    if (isMask) {
        result = r.formattedText.string;
    } else {
        result = r.extractedValue;
    }
    return result;
}
static jsi::Value __hostFunction_RNTextInputMask_unmask(jsi::Runtime &rt, react::TurboModule &turboModule,
                                                        const jsi::Value *args, size_t count) {
    std::string maskValue = args[0].getString(rt).utf8(rt);
    std::string value = args[1].getString(rt).utf8(rt);
    bool autocomplete = args[2].getBool();
    return createPromiseAsJSIValue(
        rt, [maskValue, value, autocomplete](jsi::Runtime &rt2, std::shared_ptr<facebook::react::Promise> promise) {
            try {
                auto start = std::chrono::high_resolution_clock::now();
                std::string result = getString(maskValue, value, autocomplete, 0);
                promise->resolve(jsi::String::createFromUtf8(rt2, result));
                // 获取结束时间点
                auto end = std::chrono::high_resolution_clock::now();
                // 计算延迟
                std::chrono::duration<double, std::milli> latency = end - start;
                DLOG(INFO) << "=======unmask 响应时长: " << latency.count() << " 毫秒" << std::endl;
            } catch (FormatError e) {
                promise->reject(e.what());
            }
        });
}
static jsi::Value __hostFunction_RNTextInputMask_mask(jsi::Runtime &rt, react::TurboModule &turboModule,
                                                      const jsi::Value *args, size_t count) {
    std::string maskValue = args[0].getString(rt).utf8(rt);
    std::string value = args[1].getString(rt).utf8(rt);
    bool autocomplete = args[2].getBool();
    return createPromiseAsJSIValue(
        rt, [maskValue, value, autocomplete](jsi::Runtime &rt2, std::shared_ptr<facebook::react::Promise> promise) {
            try {
                auto start = std::chrono::high_resolution_clock::now();
                std::string result = getString(maskValue, value, autocomplete, 1);
                promise->resolve(jsi::String::createFromUtf8(rt2, result));
                // 获取结束时间点
                auto end = std::chrono::high_resolution_clock::now();
                // 计算延迟
                std::chrono::duration<double, std::milli> latency = end - start;
                DLOG(INFO) << "=======mask 响应时长: " << latency.count() << " 毫秒" << std::endl;
            } catch (FormatError e) {
                promise->reject(e.what());
            }
        });
}

static jsi::Value __hostFunction_RNTextInputMask_setMask(jsi::Runtime &rt, react::TurboModule &turboModule,
                                                         const jsi::Value *args, size_t count) {

    auto turbo = static_cast<RNTextInputMask *>(&turboModule);
    if (turbo->grt == nullptr) {
        turbo->grt = &rt;
    }
    int reactNode = args[0].getNumber();
    std::string primaryFormat = args[1].getString(rt).utf8(rt);
    jsi::Object obj = args[2].asObject(rt);
    std::vector<std::string> affineFormatsValues; // 用于存储数组的值
    if (obj.hasProperty(rt, "affineFormats") && !obj.getProperty(rt, "affineFormats").isUndefined()) {
        jsi::Object affineFormats = obj.getProperty(rt, "affineFormats").asObject(rt);
        // 确保它是一个数组
        if (affineFormats.isArray(rt)) {
            // 获取数组的长度
            jsi::Array arrayAffineFormats = affineFormats.asArray(rt);
            size_t length = arrayAffineFormats.size(rt);
            // 遍历数组
            for (size_t i = 0; i < length; ++i) {
                // 获取数组元素
                jsi::Value value = arrayAffineFormats.getValueAtIndex(rt, i);
                std::cout << "===affineFormats index" + std::to_string(i) << "--" << value.getString(rt).utf8(rt)
                          << std::endl;
                affineFormatsValues.push_back(value.getString(rt).utf8(rt));
            }
        } else {
            std::cerr << "====The property 'myArray' is not an array." << std::endl;
        }
    }
    std::vector<Notation> customNotationsValues;
    if (obj.hasProperty(rt, "customNotations") && !obj.getProperty(rt, "customNotations").isUndefined()) {
        jsi::Object customNotations = obj.getProperty(rt, "customNotations").asObject(rt);
        // 确保它是一个数组
        if (customNotations.isArray(rt)) {
            // 获取数组的长度
            jsi::Array arrayCustomNotations = customNotations.asArray(rt);
            size_t length = arrayCustomNotations.size(rt);
            // 遍历数组
            for (size_t i = 0; i < length; ++i) {
                // 获取数组元素
                jsi::Object value = arrayCustomNotations.getValueAtIndex(rt, i).asObject(rt);
                std::string character;
                if (value.hasProperty(rt, "character")) {
                    character = value.getProperty(rt, "character").getString(rt).utf8(rt);
                }
                std::string characterSet;
                if (value.hasProperty(rt, "character")) {
                    characterSet = value.getProperty(rt, "characterSet").getString(rt).utf8(rt);
                }
                bool isOptional;
                if (value.hasProperty(rt, "isOptional")) {
                    isOptional = value.getProperty(rt, "isOptional").getBool();
                }
                Notation notation(character[0], characterSet, isOptional);
                customNotationsValues.push_back(notation);
            }
        } else {
            std::cerr << "====The property 'myArray' is not an array." << std::endl;
        }
    }


    std::string affinityCalculationStrategy;
    if (obj.hasProperty(rt, "affinityCalculationStrategy") &&
        !obj.getProperty(rt, "affinityCalculationStrategy").isUndefined()) {
        affinityCalculationStrategy = obj.getProperty(rt, "affinityCalculationStrategy").asString(rt).utf8(rt);
    }

    bool autocomplete = true;
    if (obj.hasProperty(rt, "autocomplete") && !obj.getProperty(rt, "autocomplete").isUndefined()) {
        autocomplete = obj.getProperty(rt, "autocomplete").asBool();
    }

    bool autoskip = false;
    if (obj.hasProperty(rt, "autoskip") && !obj.getProperty(rt, "autoskip").isUndefined()) {
        autoskip = obj.getProperty(rt, "autoskip").asBool();
    }

    bool rightToLeft = false;
    if (obj.hasProperty(rt, "rightToLeft") && !obj.getProperty(rt, "rightToLeft").isUndefined()) {
        rightToLeft = obj.getProperty(rt, "rightToLeft").asBool();
    }

    auto maskOptions = new MaskOptions(affineFormatsValues, customNotationsValues, affinityCalculationStrategy,
                                       autocomplete, autoskip, rightToLeft);
    static_cast<RNTextInputMask *>(&turboModule)->setMask(reactNode, primaryFormat, *maskOptions);
    return jsi::Value::undefined();
}

RNTextInputMask::RNTextInputMask(const ArkTSTurboModule::Context ctx, const std::string name)
    : ArkTSTurboModule(ctx, name) {
    // methodMap_ = {{"setMask", {3, setMask}}};
    methodMap_["setMask"] = MethodMetadata{3, __hostFunction_RNTextInputMask_setMask};
    methodMap_["mask"] = MethodMetadata{3, __hostFunction_RNTextInputMask_mask};
    methodMap_["unmask"] = MethodMetadata{3, __hostFunction_RNTextInputMask_unmask};
}


// namespace rnoh
