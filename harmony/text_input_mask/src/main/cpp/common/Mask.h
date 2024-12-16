/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "model/CaretStringIterator.h"
#include "model/common.h" // 假设这些头文件定义了相关类
#include "Compiler.h"
#include "model/State.h"

namespace TinpMask {

class Mask {
protected:
    std::vector<Notation> customNotations;

private:
    std::shared_ptr<State> initialState;

private:
    std::string format;

public:
    // 主构造函数
    Mask(const std::string &format, const std::vector<Notation> &customNotations) : customNotations(customNotations) {
        this->format = format;
        this->customNotations = customNotations;
        this->initialState = Compiler(customNotations).compile(format);
    }

    // 便利构造函数
    Mask(const std::string &format) : Mask(format, {}) { // 调用主构造函数，传入空的 customNotations
        std::vector<Notation> emptyVector;
        this->format = format;
        this->customNotations = emptyVector;
        this->initialState = Compiler(this->customNotations).compile(this->format);
    }


    class MaskFactory {
    public:
        static std::unordered_map<std::string, std::shared_ptr<Mask>> maskCache;

    public:
        /**
         * Factory constructor.
         *
         * Operates over own ``Mask`` cache where initialized ``Mask`` objects are stored under
         * corresponding format key: `[format : mask]`
         *
         * @returns Previously cached ``Mask`` object for requested format string. If such it
         * doesn't exist in cache, the object is constructed, cached and returned.
         */
        static std::shared_ptr<Mask> getOrCreate(const std::string &format,
                                                 const std::vector<Notation> &customNotations) {
            auto cachedMask = maskCache.find(format);
            if (cachedMask == maskCache.end()) {
                auto newMask = std::make_shared<Mask>(format, customNotations);
                maskCache[format] = newMask;
                return newMask;
            }
            return cachedMask->second;
        }

        /**
         * Check your mask format is valid.
         *
         * @param format mask format.
         * @param customNotations a list of custom rules to compile square bracket `[]` groups of format symbols.
         *
         * @returns `true` if this format coupled with custom notations will compile into a working ``Mask`` object.
         * Otherwise `false`.
         */
        static bool isValid(const std::string &format, const std::vector<Notation> &customNotations) {
            try {
                Mask(format, customNotations);
                return true;
            } catch (const FormatError &) {
                return false;
            }
        }
    };
    /**
     * Apply mask to the user input string.
     *
     * @param text user input string with current cursor position
     *
     * @returns Formatted text with extracted value an adjusted cursor position.
     */
    virtual Result apply(const CaretString &text) {
        auto iterator = makeIterator(text); // Assume this function is defined

        int affinity = 0;
        std::string extractedValue;
        std::string modifiedString;
        int modifiedCaretPosition = text.caretPosition;

        std::shared_ptr<State> state = initialState;
        AutocompletionStack autocompletionStack;

        bool insertionAffectsCaret = iterator->insertionAffectsCaret();
        bool deletionAffectsCaret = iterator->deletionAffectsCaret();
        char character = iterator->next();
        while (character != '\0') {
            auto next = state->accept(character);

            if (next != nullptr) {
                if (deletionAffectsCaret) {
                    auto opt = state->autocomplete();
                    if (opt != nullptr) {
                        autocompletionStack.push(*opt);
                    }
                }
                state = next->state;
                if (next->insert == '\0') {
                    modifiedString += "";
                } else {
                    modifiedString += next->insert;
                }
                if (next->value == '\0') {
                    extractedValue += "";
                } else {
                    extractedValue += next->value;
                }
                if (next->pass) {
                    insertionAffectsCaret = iterator->insertionAffectsCaret();
                    deletionAffectsCaret = iterator->deletionAffectsCaret();
                    character = iterator->next();
                    affinity += 1;
                } else {
                    if (insertionAffectsCaret && next->insert != '\0') {
                        modifiedCaretPosition += 1;
                    }
                    affinity -= 1;
                }
            } else {
                if (deletionAffectsCaret) {
                    modifiedCaretPosition -= 1;
                }
                insertionAffectsCaret = iterator->insertionAffectsCaret();
                deletionAffectsCaret = iterator->deletionAffectsCaret();
                character = iterator->next();
                affinity -= 1;
            }
        }

        while (text.caretGravity->autocomplete() && insertionAffectsCaret) {
            auto next = state->autocomplete();
            if (next == nullptr)
                break;

            state = next->state;
            if (next->insert == '\0') {
                modifiedString += "";
            } else {
                modifiedString += next->insert;
            }
            if (next->value == '\0') {
                extractedValue += "";
            } else {
                extractedValue += next->value;
            }
            if (next->insert == '\0') {
                modifiedCaretPosition += 1;
            }
        }
        std::shared_ptr<State> tailState = state;
        std::string tail;
        while (text.caretGravity->autoskip() && !autocompletionStack.isEmpty()) {
            auto skip = autocompletionStack.pop();
            if (modifiedString.length() == modifiedCaretPosition) {
                if (!skip->insert == '\0' && skip->insert == modifiedString.back()) {
                    modifiedString.pop_back();
                    modifiedCaretPosition -= 1;
                }
                if (skip.has_value() && skip.value().value == modifiedString.back()) {
                    extractedValue.pop_back();
                }
            } else {
                if (!skip->insert == '\0') {
                    modifiedCaretPosition -= 1;
                }
            }
            tailState = skip->state;
            tail += skip->insert;
        }

        std::string tailPlaceholder = appendPlaceholder(tailState.get(), tail); // Assume this function is defined

        return Result(CaretString(modifiedString, modifiedCaretPosition, text.caretGravity), extractedValue, affinity,
                      noMandatoryCharactersLeftAfterState(state.get()), // Assume this function is defined
                      tailPlaceholder);
    }


public:
    std::shared_ptr<CaretStringIterator> makeIterator(const CaretString &text) const {
        return std::make_shared<CaretStringIterator>(text);
    }

    /**
     * Generate placeholder.
     *
     * @return Placeholder string.
     */
public:
    std::string placeholder() { return appendPlaceholder(initialState.get(), ""); }
    /**
     * Minimal length of the text inside the field to fill all mandatory characters in the mask.
     *
     * @return Minimal satisfying count of characters inside the text field.
     */
public:
    int acceptableTextLength() {
        std::shared_ptr<State> state = std::move(initialState);
        ;
        int length = 0;

        while (state != nullptr && dynamic_cast<EOLState *>(state.get()) == nullptr) {
            if (dynamic_cast<FixedState *>(state.get()) != nullptr ||
                dynamic_cast<FreeState *>(state.get()) != nullptr ||
                dynamic_cast<ValueState *>(state.get()) != nullptr) {
                length += 1;
            }
            state = state->child; // 移动到下一个子状态
        }

        return length;
    }


    /**
     * Maximal length of the text inside the field.
     *
     * @return Total available count of mandatory and optional characters inside the text field.
     */
public:
    int totalTextLength() const {
        std::shared_ptr<State> state = initialState;
        ;
        int length = 0;

        while (state != nullptr && dynamic_cast<EOLState *>(state.get()) == nullptr) {
            if (dynamic_cast<FixedState *>(state.get()) != nullptr ||
                dynamic_cast<FreeState *>(state.get()) != nullptr ||
                dynamic_cast<ValueState *>(state.get()) != nullptr ||
                dynamic_cast<OptionalValueState *>(state.get()) != nullptr) {
                length += 1;
            }
            state = state->child; // 移动到下一个子状态
        }

        return length;
    }
    /**
     * Minimal length of the extracted value with all mandatory characters filled.\
     *
     * @return Minimal satisfying count of characters in extracted value.
     */
public:
    int acceptableValueLength() const {
        std::shared_ptr<State> state = initialState;
        int length = 0;
        while (state != nullptr && dynamic_cast<EOLState *>(state.get()) == nullptr) {
            if (dynamic_cast<FixedState *>(state.get()) != nullptr ||
                dynamic_cast<ValueState *>(state.get()) != nullptr) {
                length += 1;
            }
            state = state->child; // 移动到下一个子状态
        }

        return length;
    }
    /**
     * Maximal length of the extracted value.
     *
     * @return Total available count of mandatory and optional characters for extracted value.
     */
public:
    int totalValueLength() const {
        std::shared_ptr<State> state = initialState;
        int length = 0;
        while (state != nullptr && dynamic_cast<EOLState *>(state.get()) == nullptr) {
            if (dynamic_cast<FixedState *>(state.get()) != nullptr ||
                dynamic_cast<ValueState *>(state.get()) != nullptr ||
                dynamic_cast<OptionalValueState *>(state.get()) != nullptr) {
                length += 1;
            }
            state = state->child; // 移动到下一个子状态
        }
        return length;
    }

private:
    std::string appendPlaceholder(State *state, const std::string &placeholder) const {
        if (state == nullptr) {
            return placeholder;
        }

        if (dynamic_cast<EOLState *>(state)) {
            return placeholder;
        }

        if (auto fixedState = dynamic_cast<FixedState *>(state)) {
            return appendPlaceholder(fixedState->child.get(), placeholder + fixedState->ownCharacter);
        }

        if (auto freeState = dynamic_cast<FreeState *>(state)) {
            return appendPlaceholder(freeState->child.get(), placeholder + freeState->ownCharacter);
        }

        if (auto optionalValueState = dynamic_cast<OptionalValueState *>(state)) {
            if (!state) {
                return placeholder; // 如果 state 为 nullptr，直接返回原始 placeholder
            }
            // 使用 if-else 替代 switch
            if (optionalValueState->type->getName() == StateTypeName::Numeric) {
                return appendPlaceholder(state->child.get(), placeholder + "0");
            } else if (optionalValueState->type->getName() == StateTypeName::Literal) {
                return appendPlaceholder(state->child.get(), placeholder + "a");
            } else if (optionalValueState->type->getName() == StateTypeName::AlphaNumeric) {
                return appendPlaceholder(state->child.get(), placeholder + "-");
            } else if (optionalValueState->type->getName() == StateTypeName::Custom) {
                auto customValueState = dynamic_cast<OptionalValueState *>(state);
                auto customStateType = dynamic_cast<OptionalValueState::Custom *>(customValueState->type.get());
                return appendPlaceholder(state->child.get(), placeholder + customStateType->character);
            } else {
                return placeholder; // 未知类型，返回原始 placeholder
            }
        }

        if (auto valueState = dynamic_cast<ValueState *>(state)) {
            if (!state) {
                return placeholder; // 如果 state 为 nullptr，直接返回原始 placeholder
            }
            // 使用 if-else 替代 switch
            if (valueState->type->getName() == StateTypeName::Numeric) {
                return appendPlaceholder(state->child.get(), placeholder + "0");
            } else if (valueState->type->getName() == StateTypeName::Literal) {
                return appendPlaceholder(state->child.get(), placeholder + "a");
            } else if (valueState->type->getName() == StateTypeName::AlphaNumeric) {
                return appendPlaceholder(state->child.get(), placeholder + "-");
            } else if (valueState->type->getName() == StateTypeName::Custom) {
                auto customValueState = dynamic_cast<ValueState *>(state);
                if (customValueState->type.get()) {
                    auto customStateType = dynamic_cast<ValueState::Custom *>(customValueState->type.get());
                    return appendPlaceholder(state->child.get(), placeholder + customStateType->character);
                }else {
                    throw FormatError("appendPlaceholder customValueState type is null"); // 未找到匹配项，抛出异常 
                }

            } else {
                return placeholder; // 未知类型，返回原始 placeholder
            }
        }

        return placeholder;
    }

    bool noMandatoryCharactersLeftAfterState(State *state) const {
        if (dynamic_cast<EOLState *>(state)) {
            return true;
        } else if (auto valueState = dynamic_cast<ValueState *>(state)) {
            return valueState->isElliptical();
        } else if (dynamic_cast<FixedState *>(state)) {
            return false;
        } else {
            return noMandatoryCharactersLeftAfterState(state->nextState().get());
        }
    }
};
} // namespace TinpMask