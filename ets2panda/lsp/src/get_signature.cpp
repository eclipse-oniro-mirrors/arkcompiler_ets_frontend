/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <vector>
#include "checker/types/signature.h"
#include "compiler/lowering/util.h"
#include "get_signature.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "ir/expression.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
namespace ark::es2panda::lsp {

size_t FindFunctionNameStartNested(const std::string &str, size_t pos)
{
    if (pos > str.length() - 1) {
        return 0;
    }
    std::stack<int> bracketStack;
    size_t leftBracketPos = 0;

    for (size_t i = pos; i > 0; --i) {
        if (str[i] == ')') {
            bracketStack.push(i);
        } else if (str[i] == '(') {
            if (bracketStack.empty()) {
                leftBracketPos = i;
                break;
            }
            bracketStack.pop();
        }
    }

    if (leftBracketPos == 0) {
        return leftBracketPos;
    }

    size_t funcNameStart = leftBracketPos;
    for (size_t i = leftBracketPos - 1; i > 0; --i) {
        if (str[i] != ' ') {
            funcNameStart = i;
            break;
        }
    }

    return funcNameStart;
}

size_t FindClassNameStart(const std::string &str, size_t pos)
{
    if (pos > str.length() - 1) {
        return 0;
    }
    size_t dotPos = 0;
    for (size_t i = pos; i > 0; --i) {
        if (str[i] == '.') {
            dotPos = i;
            break;
        }
    }
    if (dotPos == 0) {
        return dotPos;
    }

    size_t classNameStart = dotPos;
    for (size_t i = dotPos - 1; i > 0; --i) {
        if (str[i] != ' ') {
            classNameStart = i;
            break;
        }
    }
    return classNameStart;
}

ir::AstNode *ClassPropertyHandler(std::string sourceCode, size_t position, ir::AstNode *node, es2panda_Context *context)
{
    auto targetPos = FindClassNameStart(std::string(std::move(sourceCode)), position);
    auto methodName = std::string(node->AsIdentifier()->Name());
    auto targetNode = GetTouchingToken(context, targetPos, false);
    if (targetNode == nullptr || !targetNode->IsIdentifier()) {
        return nullptr;
    }
    auto declNode = compiler::DeclarationFromIdentifier(targetNode->AsIdentifier());
    if (declNode == nullptr || !declNode->IsClassProperty()) {
        return nullptr;
    }
    ir::Expression *typeRef;
    if (declNode->AsClassProperty()->TypeAnnotation() != nullptr) {
        typeRef = declNode->AsClassProperty()->TypeAnnotation();
    } else {
        auto value = declNode->AsClassProperty()->Value();
        if (value == nullptr || !value->IsETSNewClassInstanceExpression()) {
            return nullptr;
        }
        typeRef = value->AsETSNewClassInstanceExpression()->GetTypeRef();
    }
    if (typeRef == nullptr || !typeRef->IsETSTypeReference()) {
        return nullptr;
    }
    auto part = typeRef->AsETSTypeReference()->Part();
    if (part == nullptr) {
        return nullptr;
    }
    auto className = part->GetIdent();
    auto classNode = compiler::DeclarationFromIdentifier(className->AsIdentifier());
    if (classNode == nullptr) {
        return nullptr;
    }
    auto methodNode = classNode->FindChild([&methodName](ir::AstNode *childNode) {
        return childNode->IsMethodDefinition() && childNode->AsMethodDefinition()->Key()->ToString() == methodName;
    });
    return methodNode;
}

void MakeSignatureHelpItem(ir::AstNode *declNode, ir::ScriptFunction *function, SignatureHelpItems &res)
{
    if (declNode == nullptr || function == nullptr) {
        return;
    }
    auto paramNodes = function->Params();
    auto returnTypeNode = function->ReturnTypeAnnotation();

    SignatureHelpItem item;
    auto methodName = std::string(declNode->AsMethodDefinition()->Id()->Name());
    item.SetPrefixDisplayParts(SymbolDisplayPart(methodName, "functionName"));
    item.SetPrefixDisplayParts(SymbolDisplayPart("(", "punctuation"));

    item.SetSeparatorDisplayParts(SymbolDisplayPart(",", "punctuation"));
    item.SetSeparatorDisplayParts(SymbolDisplayPart(" ", "space"));

    item.SetSuffixDisplayParts(SymbolDisplayPart(")", "punctuation"));
    item.SetSuffixDisplayParts(SymbolDisplayPart(" ", "space"));
    item.SetSuffixDisplayParts(SymbolDisplayPart("=>", "punctuation"));
    item.SetSuffixDisplayParts(SymbolDisplayPart(" ", "space"));
    item.SetSuffixDisplayParts(
        SymbolDisplayPart(returnTypeNode != nullptr ? returnTypeNode->DumpEtsSrc() : "", "keyword"));

    for (auto param : paramNodes) {
        if (!param->IsETSParameterExpression()) {
            return;
        }
        auto paramTypeAnnotation = param->AsETSParameterExpression()->TypeAnnotation();
        SignatureHelpParameter paramItem;
        auto paramName = std::string(param->AsETSParameterExpression()->Name());
        auto paramType = paramTypeAnnotation != nullptr ? paramTypeAnnotation->DumpEtsSrc() : "";
        paramItem.SetName(paramName);
        paramItem.SetDisplayParts(SymbolDisplayPart(paramName, "parameterNmae"));
        paramItem.SetDisplayParts(SymbolDisplayPart(":", "punctuation"));
        paramItem.SetDisplayParts(SymbolDisplayPart(" ", "space"));
        paramItem.SetDisplayParts(SymbolDisplayPart(paramType, "keyword"));
        item.SetParameters(paramItem);
    }

    res.SetItems(item);
}

SignatureHelpItems GetSignature(es2panda_Context *context, size_t position)
{
    SignatureHelpItems res;
    if (context == nullptr || position < 1) {
        return res;
    }

    auto callExpressionNode = GetTouchingToken(context, position - 1, false);
    if (callExpressionNode == nullptr || !callExpressionNode->IsCallExpression()) {
        return res;
    }

    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    const auto sourceCode = ctx->parserProgram->SourceCode();
    if (position >= sourceCode.length()) {
        return res;
    }

    auto targetPos = FindFunctionNameStartNested(std::string(sourceCode), position - 1);
    auto node = GetTouchingToken(context, targetPos, false);
    if (node == nullptr || !node->IsIdentifier()) {
        return res;
    }

    auto declNode = compiler::DeclarationFromIdentifier(node->AsIdentifier());
    if (declNode == nullptr || !declNode->IsMethodDefinition()) {
        declNode = ClassPropertyHandler(std::string(sourceCode), targetPos, node, context);
    }
    if (declNode->Parent() != nullptr && declNode->IsMethodDefinition() && declNode->Parent()->IsMethodDefinition()) {
        declNode = declNode->Parent();
    }

    res.SetApplicableSpan(node->End().index + 1, position - node->End().index - 1);
    res.SetArgumentIndex(callExpressionNode->AsCallExpression()->Arguments().size());
    if (declNode == nullptr || !declNode->IsMethodDefinition()) {
        return res;
    }
    res.SetArgumentCount(declNode->AsMethodDefinition()->Function()->Params().size());

    MakeSignatureHelpItem(declNode, declNode->AsMethodDefinition()->Function(), res);
    for (auto overload : declNode->AsMethodDefinition()->Overloads()) {
        MakeSignatureHelpItem(declNode, overload->Function(), res);
    }

    return res;
}
}  // namespace ark::es2panda::lsp