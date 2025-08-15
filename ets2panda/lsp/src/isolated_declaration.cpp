/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsFunctionType.h"
#include "isolated_declaration.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ets/etsNewClassInstanceExpression.h"

namespace ark::es2panda::lsp {
namespace {
constexpr std::array<std::string_view, 2> STRING_TYPE_ARRAY = {"Char", "String"};
constexpr std::array<std::string_view, 7> NUMBER_TYPE_ARRAY = {"Long",  "Float", "Double", "Byte",
                                                               "Short", "Int",   "Number"};

const std::unordered_set<std::string_view> &GetStringTypes()
{
    static const std::unordered_set<std::string_view> STRING_TYPES(std::begin(STRING_TYPE_ARRAY),
                                                                   std::end(STRING_TYPE_ARRAY));
    return STRING_TYPES;
}

const std::unordered_set<std::string_view> &GetNumberTypes()
{
    static const std::unordered_set<std::string_view> NUMBER_TYPES(std::begin(NUMBER_TYPE_ARRAY),
                                                                   std::end(NUMBER_TYPE_ARRAY));
    return NUMBER_TYPES;
}
}  // namespace

std::optional<std::string> GenReturnTypeStr(const checker::Type *checkerType, checker::ETSChecker *checker);
std::optional<std::string> GenUnionType(const checker::ETSUnionType *unionType, checker::ETSChecker *checker,
                                        const char splitChar);

template <class UnionType>
std::vector<UnionType *> FilterUnionTypes(const ArenaVector<UnionType *> &originTypes,
                                          [[maybe_unused]] checker::ETSChecker *checker)
{
    if (originTypes.empty()) {
        return {};
    }
    bool hasNumber = false;
    bool hasString = false;
    std::vector<UnionType *> filteredTypes;
    for (auto originType : originTypes) {
        std::string typeStr = originType->ToString();
        if constexpr (std::is_same_v<UnionType, ir::TypeNode>) {
            if (originType->IsTSThisType()) {
                filteredTypes.push_back(originType);
                continue;
            }
            typeStr = originType->GetType(checker)->ToString();
            typeStr[0] = std::toupper(typeStr[0]);
        }
        if (GetStringTypes().count(typeStr) != 0U) {
            if (hasString) {
                continue;
            }
            hasString = true;
        } else if (GetNumberTypes().count(typeStr) != 0U) {
            if (hasNumber) {
                continue;
            }
            hasNumber = true;
        }
        filteredTypes.push_back(originType);
    }
    return filteredTypes;
}

std::optional<std::string> HandleSpecificObjectTypes(const checker::ETSObjectType *objectType,
                                                     [[maybe_unused]] checker::ETSChecker *checker)
{
    if (objectType->IsETSStringType()) {
        return "string";
    }
    if (objectType->IsETSBigIntType()) {
        return "bigint";
    }
    if (objectType->IsETSUnboxableObject()) {
        return "number";
    }
    return std::nullopt;
}

std::optional<std::string> GenObjectType(const checker::ETSObjectType *objectType, checker::ETSChecker *checker)
{
    if (auto specificTypeStr = HandleSpecificObjectTypes(objectType, checker); specificTypeStr.has_value()) {
        return specificTypeStr.value();
    }
    std::string typeStr = objectType->Name().Mutf8();
    if (typeStr == "Exception" || typeStr == "NullPointerError") {
        return "Error";
    }
    if (typeStr == "Array") {
        const auto &typeArgs = objectType->TypeArguments();
        if (typeArgs.empty()) {
            return std::nullopt;
        }
        if (typeArgs[0]->IsETSUnionType()) {
            auto unionTypeStr = GenUnionType(typeArgs[0]->AsETSUnionType(), checker, ',');
            auto tupleTypeStr = std::string("[");
            if (unionTypeStr.has_value()) {
                tupleTypeStr += unionTypeStr.value();
            } else {
                return std::nullopt;
            }
            tupleTypeStr += "]";
            return tupleTypeStr;
        }

        if (auto elementTypeStr = GenReturnTypeStr(typeArgs[0], checker); elementTypeStr.has_value()) {
            return elementTypeStr.value() + "[]";
        }

        return std::nullopt;
    }
    if (size_t partialPos = typeStr.find("$partial"); partialPos != std::string::npos) {
        return "Partial<" + typeStr.substr(0, partialPos) + ">";
    }
    return typeStr;
}

std::optional<std::string> HandleObjectType(const checker::Type *checkerType, checker::ETSChecker *checker)
{
    std::string typeStr = checkerType->ToString();
    if (typeStr == "Boolean") {
        return "boolean";
    }
    if (GetStringTypes().count(typeStr) != 0U) {
        return "string";
    }
    if (GetNumberTypes().count(typeStr) != 0U) {
        return "number";
    }
    if (typeStr == "BigInt") {
        return "bigint";
    }
    return GenObjectType(checkerType->AsETSObjectType(), checker);
}

std::optional<std::string> GenUnionType(const checker::ETSUnionType *unionType, checker::ETSChecker *checker,
                                        const char splitChar = '|')
{
    auto originTypes = FilterUnionTypes(unionType->ConstituentTypes(), checker);
    if (originTypes.empty()) {
        return std::nullopt;
    }
    auto unionTypeStr = std::string();
    for (size_t i = 0; i < originTypes.size(); ++i) {
        auto elementTypeStr = GenReturnTypeStr(originTypes[i], checker);
        if (!elementTypeStr.has_value()) {
            return std::nullopt;
        }
        if (i == 0) {
            unionTypeStr += elementTypeStr.value();
        } else {
            if (splitChar == '|') {
                unionTypeStr += " | " + elementTypeStr.value();
            } else if (splitChar == ',') {
                unionTypeStr += ", " + elementTypeStr.value();
            }
        }
    }
    return unionTypeStr;
}

std::optional<std::string> HandleArrayType(const checker::Type *checkerType, checker::ETSChecker *checker)
{
    auto arrayTypeStr = std::string();
    const auto *elementType = checkerType->AsETSArrayType()->ElementType();
    bool needParentheses = elementType->IsETSUnionType() || elementType->IsETSFunctionType();
    if (needParentheses) {
        if (auto elementTypeStr = GenReturnTypeStr(elementType, checker); elementTypeStr.has_value()) {
            arrayTypeStr += "(" + elementTypeStr.value() + ")";
        }
    } else {
        if (auto elementTypeStr = GenReturnTypeStr(elementType, checker); elementTypeStr.has_value()) {
            arrayTypeStr += elementTypeStr.value();
        }
    }
    arrayTypeStr += "[]";
    return arrayTypeStr;
}

std::optional<std::string> HandleETSSpecificTypes(const checker::Type *checkerType, checker::ETSChecker *checker)
{
    switch (checker::ETSChecker::ETSType(checkerType)) {
        case checker::TypeFlag::ETS_VOID:
        case checker::TypeFlag::ETS_NULL:
        case checker::TypeFlag::ETS_UNDEFINED:
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::ETS_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NONNULLISH:
        case checker::TypeFlag::ETS_PARTIAL_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_READONLY:
            return checkerType->ToString();

        case checker::TypeFlag::ETS_OBJECT:
            return HandleObjectType(checkerType, checker);

        case checker::TypeFlag::ETS_ARRAY:
            return HandleArrayType(checkerType, checker);

        case checker::TypeFlag::ETS_UNION:
            return GenUnionType(checkerType->AsETSUnionType(), checker);
        default:
            return std::nullopt;
    }
    return std::nullopt;
}

std::optional<std::string> HandleBasicTypes(const checker::Type *checkerType)
{
    if (checkerType->IsETSEnumType()) {
        return checkerType->ToString();
    }
    if (checkerType->HasTypeFlag(checker::TypeFlag::CHAR)) {
        return "string";
    }
    if (checkerType->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC)) {
        return "number";
    }
    return std::nullopt;
}

std::optional<std::string> GenReturnTypeStr(const checker::Type *checkerType, checker::ETSChecker *checker)
{
    if (checkerType == nullptr) {
        return std::nullopt;
    }

    if (auto basicTypeStr = HandleBasicTypes(checkerType); basicTypeStr.has_value()) {
        return basicTypeStr.value();
    }

    if (checkerType->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC)) {
        return "number";
    }
    if (checkerType->IsETSStringEnumType()) {
        return "string";
    }

    if (auto specificTypeStr = HandleETSSpecificTypes(checkerType, checker); specificTypeStr.has_value()) {
        return specificTypeStr.value();
    }
    return std::nullopt;
}

std::string GetReturnTypeStr(const checker::Type *checkerType, checker::ETSChecker *checker)
{
    if (auto typeStr = GenReturnTypeStr(checkerType, checker); typeStr.has_value()) {
        return typeStr.value();
    }
    return "void";
}

const checker::Signature *GetFuncSignature(const checker::ETSFunctionType *etsFunctionType,
                                           const ir::MethodDefinition *methodDef)
{
    if (etsFunctionType->IsETSArrowType()) {
        return etsFunctionType->ArrowSignature();
    }
    if (methodDef != nullptr) {
        return methodDef->Function()->Signature();
    }
    return etsFunctionType->CallSignatures()[0];
}

std::optional<TextChange> GetTextChange(ir::ScriptFunction *func, std::string &typeStr, parser::Program *program)
{
    ES2PANDA_ASSERT(func != nullptr);

    auto returnTypeAnnotation = func->ReturnTypeAnnotation();
    if (returnTypeAnnotation != nullptr) {
        auto start = returnTypeAnnotation->Start().index;
        auto end = returnTypeAnnotation->End().index;
        return TextChange {TextSpan(start, end - start), typeStr};
    }
    // try to find the position of ')' before the function body
    auto bodyStart = func->Body()->Start().index;
    auto sourceCode = program->SourceCode().Utf8();
    auto i = bodyStart;
    for (; i >= func->Start().index; --i) {
        if (i >= 1 && sourceCode[i - 1] == ')') {
            break;
        }
    }
    // set the length to 0, meaning we should insert the text instead of replacing
    return TextChange {TextSpan(i, 0), ": " + typeStr};
}

std::optional<TextChange> ProcessMethodDefinition(ir::MethodDefinition *method, checker::ETSChecker *checker,
                                                  parser::Program *program)
{
    ES2PANDA_ASSERT(method != nullptr);

    auto signature = GetFuncSignature(method->TsType()->AsETSFunctionType(), method);
    auto typeStr = GetReturnTypeStr(signature->ReturnType(), checker);

    auto textChange = GetTextChange(method->Function(), typeStr, program);
    return textChange;
}

std::optional<TextChange> ProcessArrowFunction(ir::ArrowFunctionExpression *arrowFunc, checker::ETSChecker *checker,
                                               parser::Program *program)
{
    ES2PANDA_ASSERT(arrowFunc != nullptr);

    auto signature = arrowFunc->Function()->Signature();
    auto typeStr = GetReturnTypeStr(signature->ReturnType(), checker);

    auto textChange = GetTextChange(arrowFunc->Function(), typeStr, program);
    return textChange;
}

std::optional<TextChange> ProcessIdentifier(ir::Identifier *identifier, checker::ETSChecker *checker,
                                            parser::Program *program)
{
    ES2PANDA_ASSERT(identifier != nullptr);

    auto decl = compiler::DeclarationFromIdentifier(identifier);
    if (decl != nullptr && decl->IsMethodDefinition()) {
        return ProcessMethodDefinition(decl->AsMethodDefinition(), checker, program);
    }
    if (decl != nullptr && decl->IsArrowFunctionExpression()) {
        return ProcessArrowFunction(decl->AsArrowFunctionExpression(), checker, program);
    }
    if (decl != nullptr && decl->IsClassProperty()) {
        auto propertyValue = decl->AsClassProperty()->Value();
        if (propertyValue != nullptr && propertyValue->IsArrowFunctionExpression()) {
            return ProcessArrowFunction(propertyValue->AsArrowFunctionExpression(), checker, program);
        }
    }
    return std::nullopt;
}
}  // namespace ark::es2panda::lsp