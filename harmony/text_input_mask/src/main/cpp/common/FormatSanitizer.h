/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */

#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "Compiler.h"

namespace TinpMask {


class Compiler;
class FormatSanitizer {
public:
    std::string sanitize(const std::string &formatString) {
        checkOpenBraces(formatString);
        std::vector<std::string> blocks = divideBlocksWithMixedCharacters(getFormatBlocks(formatString));
        return sortFormatBlocks(blocks);
    }

    std::vector<std::string> getFormatBlocks(const std::string &formatString) {
        std::vector<std::string> blocks;
        std::string currentBlock;
        bool escape = false;

        for (char ch : formatString) {
            if (ch == '\\') {
                if (!escape) {
                    escape = true;
                    currentBlock += ch;
                    continue;
                }
            }

            if ((ch == '[' || ch == '{') && !escape) {
                if (!currentBlock.empty()) {
                    blocks.push_back(currentBlock);
                }
                currentBlock.clear();
            }

            currentBlock += ch;

            if ((ch == ']' || ch == '}') && !escape) {
                blocks.push_back(currentBlock);
                currentBlock.clear();
            }

            escape = false;
        }

        if (!currentBlock.empty()) {
            blocks.push_back(currentBlock);
        }

        return blocks;
    }


public:
    std::vector<std::string> divideBlocksWithMixedCharacters(const std::vector<std::string> &blocks) {
        std::vector<std::string> resultingBlocks;

        for (const auto &block : blocks) {
            if (!block.empty() && block.substr(0, 1) == "[") { // 使用 substr 来检查开头
                std::string blockBuffer;
                for (char blockCharacter : block) {
                    if (blockCharacter == '[') {
                        blockBuffer += blockCharacter;
                        continue;
                    }

                    if (blockCharacter == ']' && !endsWithEscapeCharacter(blockBuffer)) {
                        blockBuffer += blockCharacter;
                        resultingBlocks.push_back(blockBuffer);
                        break;
                    }

                    if (isDigit(blockCharacter)) {
                        if (containsAny(blockBuffer, {'A', 'a', '-', '_'})) {
                            blockBuffer += "]";
                            resultingBlocks.push_back(blockBuffer);
                            blockBuffer = "[" + std::string(1, blockCharacter);
                            continue;
                        }
                    }

                    if (isAlpha(blockCharacter)) {
                        if (containsAny(blockBuffer, {'0', '9', '-', '_'})) {
                            blockBuffer += "]";
                            resultingBlocks.push_back(blockBuffer);
                            blockBuffer = "[" + std::string(1, blockCharacter);
                            continue;
                        }
                    }

                    if (isSpecialCharacter(blockCharacter)) {
                        if (containsAny(blockBuffer, {'0', '9', 'A', 'a'})) {
                            blockBuffer += "]";
                            resultingBlocks.push_back(blockBuffer);
                            blockBuffer = "[" + std::string(1, blockCharacter);
                            continue;
                        }
                    }

                    blockBuffer += blockCharacter;
                }
            } else {
                resultingBlocks.push_back(block);
            }
        }

        return resultingBlocks;
    }

private:
    bool endsWithEscapeCharacter(const std::string &str) const { return !str.empty() && str.back() == '\\'; }

    bool isDigit(char ch) const { return ch >= '0' && ch <= '9'; }

    bool isAlpha(char ch) const { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }

    bool isSpecialCharacter(char ch) const { return ch == '-' || ch == '_'; }

    bool containsAny(const std::string &str, const std::vector<char> &characters) const {
        for (char c : characters) {
            if (str.find(c) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

public:
    std::string sortFormatBlocks(const std::vector<std::string> &blocks) {
        std::vector<std::string> sortedBlocks;

        for (const auto &block : blocks) {
            std::string sortedBlock;

            if (startsWith(block, "[")) {
                std::string cleanedBlock = block.substr(1, block.size() - 2); // Remove the surrounding brackets
                if (cleanedBlock.find("0") != std::string::npos || cleanedBlock.find("9") != std::string::npos ||
                    cleanedBlock.find("a") != std::string::npos || cleanedBlock.find("A") != std::string::npos) {
                    sortedBlock = "[" + sortString(cleanedBlock) + "]";
                } else {
                    cleanedBlock = replaceChars(cleanedBlock);
                    sortedBlock = "[" + sortString(cleanedBlock) + "]";
                    sortedBlock = replaceCharsBack(sortedBlock);
                }
            } else {
                sortedBlock = block;
            }

            sortedBlocks.push_back(sortedBlock);
        }

        return join(sortedBlocks); // Ensure this is returning std::vector<std::string>
    }

    void checkOpenBraces(const std::string &inputString) {
        bool escape = false;
        bool squareBraceOpen = false;
        bool curlyBraceOpen = false;

        for (const char &ch : inputString) {
            if (ch == '\\') {
                escape = !escape;
                continue;
            }

            if (ch == '[') {
                if (squareBraceOpen) {
                    throw FormatError();
                }
                squareBraceOpen = !escape;
            }

            if (ch == ']' && !escape) {
                squareBraceOpen = false;
            }

            if (ch == '{') {
                if (curlyBraceOpen) {
                    throw FormatError();
                }
                curlyBraceOpen = !escape;
            }

            if (ch == '}' && !escape) {
                curlyBraceOpen = false;
            }
            escape = false;
        }
    }

private:
    bool startsWith(const std::string &str, const std::string &prefix) const { return str.rfind(prefix, 0) == 0; }

    std::string sortString(std::string str) {
        std::sort(str.begin(), str.end());
        return str;
    }

    std::string replaceChars(const std::string &str) {
        std::string newStr = str;
        std::replace(newStr.begin(), newStr.end(), '_', 'A');
        std::replace(newStr.begin(), newStr.end(), '-', 'a');
        return newStr;
    }

    std::string replaceCharsBack(const std::string &str) {
        std::string newStr = str;
        std::replace(newStr.begin(), newStr.end(), 'A', '_');
        std::replace(newStr.begin(), newStr.end(), 'a', '-');
        return newStr;
    }
    std::string join(const std::vector<std::string> &strings) {
        std::string result;
        for (const auto &str : strings) {
            result += str; // 将所有块连接成一个字符串
        }
        return result;
    }
};
} // namespace TinpMask