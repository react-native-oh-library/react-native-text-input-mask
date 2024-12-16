/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <memory>
#include <iostream>

namespace TinpMask {


class Next;
class State {
public:
    std::shared_ptr<State> child; // 指向下一个状态的智能指针

public:
    // 构造函数
    explicit State(std::shared_ptr<State> child = nullptr) : child(child) {}

    // 虚析构函数，以确保派生类的析构函数被正确调用
    virtual ~State() = default;

    /**
     * Abstract method.
     *
     * Defines whether the state accepts user input character or not, and which actions should take
     * place when the character is accepted.
     *
     * @param character character from the user input string.
     *
     * @returns Next object instance with a set of actions that should take place when the user
     * input character is accepted.
     *
     * @throws Fatal error, if the method is not implemented.
     */
    virtual std::shared_ptr<Next> accept(char character) = 0;

    /**
     * Automatically complete user input.
     *
     * @returns Next object instance with a set of actions to complete user input. If no
     * autocomplete available, returns nullptr.
     */
    virtual std::shared_ptr<Next> autocomplete() {
        return nullptr; // 默认返回 nullptr
    }

    /**
     * Obtain the next state.
     *
     * Sometimes it is necessary to override this behavior. For instance, State may want to
     * return self as the next state under certain conditions.
     *
     * @returns State object.
     */
    virtual std::shared_ptr<State> nextState() {
        return child; // 返回下一个状态
    }

    // 转换为字符串表示
    virtual std::string toString() const { return "BASE -> " + (child ? child->toString() : "null"); }
};

// EOLState 类的实现
class EOLState : public State {
public:
    EOLState(std::shared_ptr<State> child = nullptr) : State(child) {}

    std::shared_ptr<Next> accept(char character) override {
        return nullptr; // 该状态不接受字符
    }

    std::string toString() const override {
        return "EOL"; // 返回字符串表示
    }
};


// FixedState 类的实现
class FixedState : public State {
public:
    char ownCharacter;

public:
    FixedState(std::shared_ptr<State> child, char ownCharacter) : State(child), ownCharacter(ownCharacter) {}

    std::shared_ptr<Next> accept(char character) override {
        if (this->ownCharacter == character) {
            return std::make_shared<Next>(this->nextState(), character, true, character);
        } else {
            return std::make_shared<Next>(this->nextState(), this->ownCharacter, false, this->ownCharacter);
        }
    }

    std::shared_ptr<Next> autocomplete() override {
        return std::make_shared<Next>(this->nextState(), this->ownCharacter, false, this->ownCharacter);
    }

    std::string toString() const override {
        return "{" + std::string(1, this->ownCharacter) + "} -> " + (child ? child->toString() : "null");
    }
};

// FreeState 类的实现
class FreeState : public State {
public:
    char ownCharacter;

public:
    FreeState(std::shared_ptr<State> child, char ownCharacter) : State(child), ownCharacter(ownCharacter) {}

    std::shared_ptr<Next> accept(char character) override {
        if (this->ownCharacter == character) {
            return std::make_shared<Next>(this->nextState(), character, true, '\0');
        } else {
            return std::make_shared<Next>(this->nextState(), this->ownCharacter, false, '\0');
        }
    }

    std::shared_ptr<Next> autocomplete() override {
        return std::make_shared<Next>(this->nextState(), this->ownCharacter, false, '\0');
    }

    std::string toString() const override {
        return std::string(1, this->ownCharacter) + " -> " + (child ? child->toString() : "null");
    }
};
enum StateTypeName { Numeric, Literal, AlphaNumeric, Custom };
class OptionalValueState : public State {
public:
    class OptionalValueStateType {
    public:
        virtual StateTypeName getName() = 0;
    };

    class Numeric : public OptionalValueStateType {
    public:
        StateTypeName getName() override { return StateTypeName::Numeric; }
    };
    class Literal : public OptionalValueStateType {
    public:
        StateTypeName getName() override { return StateTypeName::Literal; }
    };
    class AlphaNumeric : public OptionalValueStateType {
    public:
        StateTypeName getName() override { return StateTypeName::AlphaNumeric; }
    };

    class Custom : public OptionalValueStateType {
    public:
        char character;
        std::string characterSet;
        StateTypeName getName() override { return StateTypeName::Custom; }
        Custom(char character, const std::string &characterSet) : character(character), characterSet(characterSet) {}
    };

public:
    std::shared_ptr<OptionalValueStateType> type;

    bool accepts(char character) {
        if (dynamic_cast<Numeric *>(type.get())) {
            return std::isdigit(character);
        } else if (dynamic_cast<Literal *>(type.get())) {
            return std::isalpha(character);
        } else if (dynamic_cast<AlphaNumeric *>(type.get())) {
            return std::isalnum(character);
        } else if (auto customType = dynamic_cast<Custom *>(type.get())) {
            return customType->characterSet.find(character) != std::string::npos;
        }
        return false;
    }

    OptionalValueState(std::shared_ptr<State> child, std::shared_ptr<OptionalValueStateType> type)
        : State(child), type(type) {}

    OptionalValueState(std::shared_ptr<State> child, std::shared_ptr<OptionalValueStateType> &type)
        : State(child), type(type) {}

    std::shared_ptr<Next> accept(char character) override {
        if (this->accepts(character)) {
            return std::make_shared<Next>(this->nextState(), character, true, character);
        } else {
            return std::make_shared<Next>(this->nextState(), '\0', false, '\0');
        }
    }

    std::string toString() const override {
        if (dynamic_cast<Literal *>(type.get())) {
            return "[a] -> " + (child ? child->toString() : "null");
        } else if (dynamic_cast<Numeric *>(type.get())) {
            return "[9] -> " + (child ? child->toString() : "null");
        } else if (dynamic_cast<AlphaNumeric *>(type.get())) {
            return "[-] -> " + (child ? child->toString() : "null");
        } else if (auto customType = dynamic_cast<Custom *>(type.get())) {
            return "[" + std::string(1, customType->character) + "] -> " + (child ? child->toString() : "null");
        }
        return "unknown -> null";
    }
};

// ValueState 类定义
class ValueState : public State, public std::enable_shared_from_this<ValueState> {
public:
    class ValueStateType {
    public:
        virtual StateTypeName getName() = 0;
    };

    class Numeric : public ValueStateType {
    public:
        StateTypeName getName() override { return StateTypeName::Numeric; }
    };
    class Literal : public ValueStateType {
    public:
        StateTypeName getName() override { return StateTypeName::Literal; }
    };
    class AlphaNumeric : public ValueStateType {
    public:
        StateTypeName getName() override { return StateTypeName::AlphaNumeric; }
    };
    class Ellipsis : public ValueStateType {
    public:
        std::shared_ptr<ValueStateType> inheritedType;
        explicit Ellipsis(std::shared_ptr<ValueStateType> inheritedType) : inheritedType(inheritedType) {}
        StateTypeName getName() override { return StateTypeName::Custom; }
    };
    class Custom : public ValueStateType {

    public:
        char character;
        std::string characterSet;
        Custom(char character, const std::string &characterSet) : character(character), characterSet(characterSet) {}
        StateTypeName getName() override { return StateTypeName::Custom; }
    };

public:
    std::shared_ptr<ValueStateType> type;
    bool accepts(char character) {
        if (dynamic_cast<Numeric *>(type.get())) {
            return std::isdigit(character);
        } else if (dynamic_cast<Literal *>(type.get())) {
            return std::isalpha(character);
        } else if (dynamic_cast<AlphaNumeric *>(type.get())) {
            return std::isalnum(character);
        } else if (auto ellipsisType = dynamic_cast<Ellipsis *>(type.get())) {
            return acceptsWithInheritedType(ellipsisType->inheritedType, character);
        } else if (auto customType = dynamic_cast<Custom *>(type.get())) {
            return customType->characterSet.find(character) != std::string::npos;
        }
        return false;
    }

    bool acceptsWithInheritedType(std::shared_ptr<ValueStateType> inheritedType, char character) {
        if (dynamic_cast<Numeric *>(inheritedType.get())) {
            return std::isdigit(character);
        } else if (dynamic_cast<Literal *>(inheritedType.get())) {
            return std::isalpha(character);
        } else if (dynamic_cast<AlphaNumeric *>(inheritedType.get())) {
            return std::isalnum(character);
        } else if (auto customType = dynamic_cast<Custom *>(inheritedType.get())) {
            return customType->characterSet.find(character) != std::string::npos;
        }
        return false;
    }

public:
    // 构造函数用于创建 Ellipsis 类型的 ValueState
    ValueState(std::shared_ptr<ValueStateType> inheritedType)
        : State(nullptr), type(std::make_shared<Ellipsis>(inheritedType)) {}

    ValueState(std::shared_ptr<State> child, std::shared_ptr<ValueStateType> type) : State(child), type(type) {}

    std::shared_ptr<Next> accept(char character) override {
        if (!accepts(character))
            return nullptr;
        return std::make_shared<Next>(nextState(), character, true, character);
    }

    bool isElliptical() const { return dynamic_cast<Ellipsis *>(type.get()) != nullptr; }

    std::shared_ptr<State> nextState() override { return isElliptical() ? shared_from_this() : State::nextState(); }

    std::string toString() const override {
        if (dynamic_cast<Literal *>(type.get())) {
            return "[A] -> " + (child ? child->toString() : "null");
        } else if (dynamic_cast<Numeric *>(type.get())) {
            return "[0] -> " + (child ? child->toString() : "null");
        } else if (dynamic_cast<AlphaNumeric *>(type.get())) {
            return "[_] -> " + (child ? child->toString() : "null");
        } else if (dynamic_cast<Ellipsis *>(type.get())) {
            return "[…] -> " + (child ? child->toString() : "null");
        } else if (auto customType = dynamic_cast<Custom *>(type.get())) {
            return "[" + std::string(1, customType->character) + "] -> " + (child ? child->toString() : "null");
        }
        return "unknown -> null";
    }
};

}
