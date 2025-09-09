/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "astDump.h"

#include "ir/astNode.h"
#include "util/helpers.h"
#include "parser/program/program.h"

#include <cmath>
#include <iostream>

namespace ark::es2panda::ir {
AstDumper::AstDumper(const ir::AstNode *node, util::StringView sourceCode) : index_(sourceCode)
{
    isSrcEmpty_ = sourceCode.Empty();
    SerializeObject(node);
}

void AstDumper::Add(std::initializer_list<AstDumper::Property> props)
{
    AddList<std::initializer_list<AstDumper::Property>>(props);
}

void AstDumper::Add(const AstDumper::Property &prop)
{
    Serialize(prop);
}

const char *AstDumper::ModifierToString(ModifierFlags flags)
{
    if ((flags & ModifierFlags::INTERNAL) != 0) {
        return "internal";
    }

    if ((flags & ModifierFlags::PRIVATE) != 0) {
        return "private";
    }

    if ((flags & ModifierFlags::PROTECTED) != 0) {
        return "protected";
    }

    if ((flags & ModifierFlags::PUBLIC) != 0) {
        return "public";
    }

    return nullptr;
}

const char *AstDumper::TypeOperatorToString(TSOperatorType operatorType)
{
    if (operatorType == TSOperatorType::KEYOF) {
        return "keyof";
    }

    if (operatorType == TSOperatorType::READONLY) {
        return "readonly";
    }

    if (operatorType == TSOperatorType::UNIQUE) {
        return "unique";
    }

    return nullptr;
}

void AstDumper::Serialize(const AstDumper::Property &prop)
{
    SerializePropKey(prop.Key());
    const auto &value = prop.Value();

    if (std::holds_alternative<const char *>(value)) {
        SerializeString(std::get<const char *>(value));
    } else if (std::holds_alternative<util::StringView>(value)) {
        SerializeString(std::get<util::StringView>(value));
    } else if (std::holds_alternative<bool>(value)) {
        SerializeBoolean(std::get<bool>(value));
    } else if (std::holds_alternative<lexer::Number>(value)) {
        SerializeNumber(std::get<lexer::Number>(value));
    } else if (std::holds_alternative<char16_t>(value)) {
        SerializeChar16(std::get<char16_t>(value));
    } else if (std::holds_alternative<const ir::AstNode *>(value)) {
        SerializeObject(std::get<const ir::AstNode *>(value));
    } else if (std::holds_alternative<std::vector<const ir::AstNode *>>(value)) {
        SerializeArray(std::get<std::vector<const ir::AstNode *>>(value));
    } else if (std::holds_alternative<lexer::TokenType>(value)) {
        SerializeToken(std::get<lexer::TokenType>(value));
    } else if (std::holds_alternative<std::initializer_list<Property>>(value)) {
        SerializePropList(std::get<std::initializer_list<Property>>(value));
    } else if (std::holds_alternative<Property::Constant>(value)) {
        SerializeConstant(std::get<Property::Constant>(value));
    }
}

void AstDumper::SerializeToken(lexer::TokenType token)
{
    ss_ << "\"" << lexer::TokenToString(token) << "\"";
}

void AstDumper::SerializePropKey(const char *str)
{
    ss_ << std::endl;
    Indent();
    SerializeString(str);
    ss_ << ": ";
}

void AstDumper::SerializeString(const char *str)
{
    ss_ << "\"" << util::Helpers::CreateEscapedString(str) << "\"";
}

void AstDumper::SerializeString(const util::StringView &str)
{
    ss_ << "\"" << util::Helpers::CreateEscapedString(str.Utf8()) << "\"";
}

void AstDumper::SerializeNumber(size_t number)
{
    ss_ << number;
}

void AstDumper::SerializeNumber(lexer::Number number)
{
    if (number.IsInt()) {
        ss_ << number.GetInt();
    } else if (number.IsLong()) {
        ss_ << number.GetLong();
    } else if (number.IsFloat()) {
        if (std::isinf(number.GetFloat())) {
            ss_ << "\"Infinity\"";
        } else {
            ss_ << number.GetFloat();
        }
    } else {
        if (std::isinf(number.GetDouble())) {
            ss_ << "\"Infinity\"";
        } else {
            ss_ << number.GetDouble();
        }
    }
}

void AstDumper::SerializeChar16(char16_t c16)
{
    SerializeString(util::Helpers::UTF16toUTF8(c16).c_str());
}

void AstDumper::SerializeBoolean(bool boolean)
{
    ss_ << (boolean ? "true" : "false");
}

void AstDumper::SerializeConstant(Property::Constant constant)
{
    switch (constant) {
        case Property::Constant::PROP_NULL: {
            ss_ << "null";
            break;
        }
        case Property::Constant::PROP_UNDEFINED: {
            ss_ << "\"undefined\"";
            break;
        }
        case Property::Constant::EMPTY_ARRAY: {
            ss_ << "[]";
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void AstDumper::SerializePropList(std::initializer_list<AstDumper::Property> props)
{
    Wrap([this, &props]() -> void {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (const auto *it = props.begin(); it != props.end(); ++it) {
            Serialize(*it);
            if (std::next(it) != props.end()) {
                ss_ << ',';
            }
        }
    });
}

void AstDumper::SerializeArray(std::vector<const ir::AstNode *> array)
{
    Wrap(
        [this, &array]() -> void {
            for (auto it = array.begin(); it != array.end(); ++it) {
                ss_ << std::endl;
                Indent();

                SerializeObject(*it);

                if (std::next(it) != array.end()) {
                    ss_ << ',';
                }
            }
        },
        '[', ']');
}

void AstDumper::SerializeObject(const ir::AstNode *object)
{
    Wrap([this, object]() -> void {
        object->Dump(this);
        if (!isSrcEmpty_) {
            SerializeLoc(object->Range());
        }
    });
}

void AstDumper::Wrap(const WrapperCb &cb, char delimStart, char delimEnd)
{
    ss_ << delimStart;
    indent_++;

    cb();
    ss_ << std::endl;
    indent_--;
    Indent();
    ss_ << delimEnd;
}

void AstDumper::SerializeLoc(const lexer::SourceRange &loc)
{
    ss_ << ',';
    SerializePropKey("loc");

    Wrap([this, &loc]() -> void {
        SerializePropKey("start");
        SerializeSourcePosition(loc.start);
        ss_ << ',';
        SerializePropKey("end");
        SerializeSourcePosition(loc.end);
    });
}

void AstDumper::SerializeSourcePosition(const lexer::SourcePosition &pos)
{
    lexer::SourceLocation loc = index_.GetLocation(pos);

    Wrap([this, &loc]() -> void {
        SerializePropKey("line");
        SerializeNumber(loc.line);
        ss_ << ',';
        SerializePropKey("column");
        SerializeNumber(loc.col);
        ss_ << ',';
        SerializePropKey("program");
        if (loc.Program() != nullptr) {
            SerializeString(loc.Program()->FileNameWithExtension().Utf8());
        } else {
            SerializeConstant(Property::Constant::PROP_NULL);
        }
    });
}

void AstDumper::Indent()
{
    for (int32_t i = 0; i < indent_; i++) {
        ss_ << "  ";
    }
}
}  // namespace ark::es2panda::ir
