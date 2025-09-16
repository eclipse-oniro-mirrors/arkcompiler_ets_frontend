/**
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

#include <cstddef>
#include <string>
#include <vector>
#include "cancellation_token.h"
#include "compiler/lowering/util.h"
#include "inlay_hints.h"
#include "public/public.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::lsp {

/**
 * this function tries to find the first child node that includes the given span.
 * It can be used to avoid double processing of the same node, like in the following case in global scope:
 *      1)   let a = map.get("a");
 * In this case, the call expression node will be included both in the init method and in the global class property.
 */
ir::AstNode *TryGetIncludingNode(const ir::AstNode *ast, const TextSpan *span)
{
    if (ast == nullptr || span == nullptr) {
        return nullptr;
    }
    auto expectedStart = span->start;
    auto expectedEnd = expectedStart + span->length;
    auto checkFunc = [&expectedStart, &expectedEnd](ir::AstNode *node) {
        return expectedStart <= node->Range().end.index && node->Range().start.index <= expectedEnd;
    };
    auto *found = ast->FindChild(checkFunc);
    return found;
}

int GetFullWidth(const ir::AstNode *node)
{
    return node->End().index - node->Start().index;
}

void AddEnumMemberValueHints(const std::string &text, const int position, InlayHintList *result)
{
    const auto kind = InlayHintKind::ENUM;
    const auto whitespaceBerfore = true;
    const auto whitespaceAfter = false;
    const auto hint = InlayHint {text, position, kind, whitespaceBerfore, whitespaceAfter};
    result->hints.push_back(hint);
}

void AddTypeHints(const std::string &text, const int position, InlayHintList *result)
{
    const auto kind = InlayHintKind::TYPE;
    const auto whitespaceBerfore = true;
    const auto whitespaceAfter = false;
    const auto hint = InlayHint {text, position, kind, whitespaceBerfore, whitespaceAfter};
    result->hints.push_back(hint);
}

void AddParameterHints(const std::string &text, const int position, const bool isFirstVariadicArgument,
                       InlayHintList *result)
{
    const auto kind = InlayHintKind::PARAMETER;
    const auto whitespaceBerfore = false;
    const auto whitespaceAfter = true;
    std::string textForPush;
    if (isFirstVariadicArgument) {
        textForPush = "..." + text;
    } else {
        textForPush = text;
    }

    const auto hint = InlayHint {textForPush, position, kind, whitespaceBerfore, whitespaceAfter};
    result->hints.push_back(hint);
}
bool DecodedTextSpanIntersectsWith(const int start1, const int length1, const int start2, const int length2)
{
    const int end1 = start1 + length1;
    const int end2 = start2 + length2;
    return start2 <= end1 && end2 >= start1;
}

bool TextSpanIntersectsWith(const TextSpan span, const int position, const int nodeSize)
{
    return DecodedTextSpanIntersectsWith(span.start, span.length, position, nodeSize);
}

bool IsExpressionWithTypeArguments(const ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::CALL_EXPRESSION || node->Type() == ir::AstNodeType::NEW_EXPRESSION;
}

void GetVariableDeclarationTypeForHints(const ir::AstNode *decl, InlayHintList *result)
{
    const std::string listStr = "list";

    decl->FindChild([decl, &result, &listStr](ark::es2panda::ir::AstNode *node) {
        if (node->IsNumberLiteral()) {
            AddTypeHints(TokenToString(ark::es2panda::lexer::TokenType::KEYW_NUMBER), decl->End().index, result);
        }
        if (node->IsStringLiteral()) {
            AddTypeHints(TokenToString(ark::es2panda::lexer::TokenType::KEYW_STRING), decl->End().index, result);
        }
        if (node->IsBooleanLiteral()) {
            AddTypeHints(TokenToString(ark::es2panda::lexer::TokenType::KEYW_BOOLEAN), decl->End().index, result);
            TokenToString(ark::es2panda::lexer::TokenType::KEYW_BOOLEAN);
        }
        if (node->IsObjectExpression()) {
            AddTypeHints(TokenToString(ark::es2panda::lexer::TokenType::KEYW_OBJECT), decl->End().index, result);
            TokenToString(ark::es2panda::lexer::TokenType::KEYW_OBJECT);
        }
        if (node->IsArrayExpression()) {
            AddTypeHints(listStr, decl->End().index, result);
        }
        return false;
    });
}

void AddParamIfTypeRef(const ir::MethodDefinition *decl, const ArenaVector<ir::Expression *> &args,
                       InlayHintList *result)
{
    int paramIndex = 0;
    auto params = decl->Function()->Params();
    for (const auto &param : params) {
        if (param->IsETSParameterExpression()) {
            auto part = param->AsETSParameterExpression()->Name().Utf8();
            std::string text {part.data(), part.size()};
            AddParameterHints(text, args.at(paramIndex)->Start().index, false, result);
            paramIndex++;
        }
    }
}

void GetCallExpTypeForHints(const ir::AstNode *expr, [[maybe_unused]] const ir::AstNode *parent, InlayHintList *result)
{
    const ir::Expression *callee;
    if (expr->IsCallExpression()) {
        callee = expr->AsCallExpression()->Callee();
    } else if (expr->IsNewExpression()) {
        callee = expr->AsNewExpression()->Callee();
    } else {
        return;
    }
    if (!(callee->IsIdentifier() || callee->IsMemberExpression())) {
        return;
    }
    const auto args =
        expr->IsCallExpression() ? expr->AsCallExpression()->Arguments() : expr->AsNewExpression()->Arguments();
    if (args.empty()) {
        return;
    }

    auto *decl = callee->IsIdentifier()
                     ? compiler::DeclarationFromIdentifier(callee->AsIdentifier())
                     : compiler::DeclarationFromIdentifier(callee->AsMemberExpression()->Property()->AsIdentifier());
    if (decl != nullptr && decl->IsMethodDefinition()) {
        AddParamIfTypeRef(decl->AsMethodDefinition(), args, result);
    }
}

bool ShouldShowParameterNameHints(const UserPreferences &preferences)
{
    return preferences.GetIncludeInlayParameterNameHints() ==
               UserPreferences::IncludeInlayParameterNameHints::LITERALS ||
           preferences.GetIncludeInlayParameterNameHints() == UserPreferences::IncludeInlayParameterNameHints::ALL;
}
int GetIndexForEnum(const ir::AstNode *mem, const std::string &assignName)
{
    auto const list = mem->AsClassProperty()->Value()->AsArrayExpression()->Elements();
    for (size_t index = 0; index < list.size(); index++) {
        const auto nameD = std::string(list.at(index)->AsStringLiteral()->Str());
        if (strstr(assignName.c_str(), nameD.c_str()) != nullptr) {
            return index;
        }
    }
    return -1;
}
void SaveNumEnums(ir::AstNode *mem, const ir::AstNode *member, InlayHintList *result, int &enumValueIndex)
{
    const int exitNum = -1;
    const auto stringValuesArray = "#StringValuesArray";
    if (!mem->IsClassProperty() || !mem->AsClassProperty()->Key()->IsIdentifier()) {
        return;
    }
    if (mem->AsClassProperty()->Key()->AsIdentifier()->Name() != stringValuesArray) {
        return;
    }
    auto const list = mem->AsClassProperty()->Value()->AsArrayExpression()->Elements();
    SaveToList(list.at(enumValueIndex), member, result);
    enumValueIndex = exitNum;
}

void SaveStringEnums(ir::AstNode *mem, const ir::AstNode *member, InlayHintList *result, int &enumValueIndex)
{
    const int exitNum = -1;
    auto const list = mem->AsClassProperty()->Value()->AsArrayExpression()->Elements();
    SaveToList(list.at(enumValueIndex), member, result);
    enumValueIndex = exitNum;
}

void GetEnumIndexForSave(const ir::AstNode *enumMember, const ir::AstNode *member, InlayHintList *result)
{
    const int miles = -1;
    int enumValueIndex = miles;
    const auto assignName = std::string(member->AsAssignmentExpression()->Right()->ToString());

    enumMember->AsClassDefinition()->FindChild([member, assignName, &enumValueIndex, &result](ir::AstNode *mem) {
        const int exitNum = -1;
        const auto namesArray = "#NamesArray";
        const auto valuesArray = "#ValuesArray";
        const auto stringValuesArray = "#StringValuesArray";
        if (!mem->IsClassProperty() || !mem->AsClassProperty()->Key()->IsIdentifier()) {
            return false;
        }
        const auto keyName = mem->AsClassProperty()->Key()->AsIdentifier()->Name();
        if (keyName == namesArray) {
            enumValueIndex = GetIndexForEnum(mem, assignName);
        } else if (enumValueIndex != exitNum && keyName == valuesArray) {
            SaveNumEnums(mem, member, result, enumValueIndex);
        } else if (enumValueIndex != exitNum && keyName == stringValuesArray) {
            SaveStringEnums(mem, member, result, enumValueIndex);
        }
        return false;
    });
}

void SaveToList(const ir::Expression *mem, const ir::AstNode *asignNode, InlayHintList *list)
{
    std::string enumValue;
    if (mem->IsNumberLiteral()) {
        if (mem->AsNumberLiteral()->Number().IsInt()) {
            enumValue = std::to_string(mem->AsNumberLiteral()->Number().GetInt());
        } else if (mem->AsNumberLiteral()->Number().IsDouble()) {
            enumValue = std::to_string(mem->AsNumberLiteral()->Number().GetDouble());
        } else if (mem->AsNumberLiteral()->Number().IsFloat()) {
            enumValue = std::to_string(mem->AsNumberLiteral()->Number().GetFloat());
        } else if (mem->AsNumberLiteral()->Number().IsLong()) {
            enumValue = std::to_string(mem->AsNumberLiteral()->Number().GetLong());
        }
    } else if (mem->IsStringLiteral()) {
        enumValue = std::string(mem->AsStringLiteral()->Str().Utf8());
    }
    if (!enumValue.empty()) {
        AddEnumMemberValueHints(enumValue, asignNode->End().index, list);
    }
}

void GetEnumTypeForHints(const ir::AstNode *member, const ir::AstNode *parent, InlayHintList *result)
{
    parent->FindChild([&result, member](ark::es2panda::ir::AstNode *enumMember) {
        if (!enumMember->IsClassDefinition()) {
            return false;
        }
        const auto assignName = std::string(member->AsAssignmentExpression()->Right()->ToString());
        const auto findName = std::string(enumMember->AsClassDefinition()->Ident()->Name());
        if (assignName.empty() || findName.empty()) {
            return false;
        }
        if (strstr(assignName.c_str(), findName.c_str()) == nullptr) {
            return false;
        }
        GetEnumIndexForSave(enumMember, member, result);
        return false;
    });
}

void AddReturnTypeTextFromFunc(const ir::ScriptFunction *func, const ir::AstNode *decl, InlayHintList *result)
{
    AddTypeHints(func->Signature()->ReturnType()->ToString(), decl->End().index, result);
}

void GetFunctionReturnTypeForHints(const ir::AstNode *decl, InlayHintList *result)
{
    std::string retType;
    if (decl->IsArrowFunctionExpression()) {
        AddReturnTypeTextFromFunc(decl->AsArrowFunctionExpression()->Function(), decl, result);
    } else if (decl->IsMethodDefinition()) {
        AddReturnTypeTextFromFunc(decl->AsMethodDefinition()->Function(), decl, result);
    } else if (decl->IsFunctionDeclaration()) {
        AddReturnTypeTextFromFunc(decl->AsFunctionDeclaration()->Function(), decl, result);
    }
}

void AddTypeParamIfTypeRef(const ir::AstNode *childNode, const ir::AstNode *param, InlayHintList *result)
{
    if (childNode->IsETSTypeReference()) {
        AddTypeHints(std::string(childNode->AsETSTypeReference()->Part()->Name()->ToString()), param->End().index,
                     result);
    }
}

void GetFunctionParameterTypeForHints(const ir::AstNode *node, InlayHintList *result)
{
    const auto nodeParams = node->AsMethodDefinition()->Function()->Params();
    if (nodeParams.empty()) {
        return;
    }
    for (const auto param : nodeParams) {
        if (param->IsETSParameterExpression()) {
            param->AsETSParameterExpression()->FindChild([param, &result](ark::es2panda::ir::AstNode *childNode) {
                AddTypeParamIfTypeRef(childNode, param, result);
                return false;
            });
        }
    }
}

bool IsSignatureSupportingReturnAnnotation(const ir::AstNode *node)
{
    return node->IsArrowFunctionExpression() || node->IsFunctionExpression() || node->IsFunctionDeclaration() ||
           node->IsMethodDefinition() || (node->IsMethodDefinition());
}

bool ShouldProcessNode(const ir::AstNode *node, const TextSpan *span)
{
    if (node == nullptr || GetFullWidth(node) == 0) {
        return false;
    }

    if (!TextSpanIntersectsWith(*span, node->Start().index, GetFullWidth(node))) {
        return false;
    }

    if (node->IsTyped() && !IsExpressionWithTypeArguments(node)) {
        return false;
    }

    return true;
}
bool IsCancellableNode(const ir::AstNode *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::TS_MODULE_DECLARATION:
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::CLASS_EXPRESSION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            return true;
        default:
            return false;
    }
}

void ProcessFunctionRelatedHints(const ir::AstNode *node, const UserPreferences &preferences, InlayHintList *result)
{
    if (preferences.GetIncludeInlayFunctionParameterTypeHints() && node->IsFunctionDeclaration()) {
        GetFunctionParameterTypeForHints(node, result);
    }
    if (preferences.GetIncludeInlayFunctionLikeReturnTypeHints() && IsSignatureSupportingReturnAnnotation(node)) {
        GetFunctionReturnTypeForHints(node, result);
    }
}

void ProcessNodeBasedOnPreferences(const ir::AstNode *node, const ir::AstNode *parent,
                                   const UserPreferences &preferences, InlayHintList *result)
{
    if (preferences.GetIncludeInlayVariableTypeHints() && node->IsAssignmentExpression()) {
        GetVariableDeclarationTypeForHints(node->AsAssignmentExpression(), result);
    } else if (preferences.GetIncludeInlayEnumMemberValueHints() && node->IsAssignmentExpression()) {
        GetEnumTypeForHints(node, parent, result);
    } else if (ShouldShowParameterNameHints(preferences) && (node->IsCallExpression() || node->IsNewExpression())) {
        GetCallExpTypeForHints(node, parent, result);
    } else {
        ProcessFunctionRelatedHints(node, preferences, result);
    }
}

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct InlayHintProcessingContext {
    const TextSpan *span = nullptr;
    const ir::AstNode *ast = nullptr;
    CancellationToken &cancellationToken;
    UserPreferences &preferences;

    InlayHintProcessingContext(const TextSpan *s, const ir::AstNode *a, CancellationToken &c, UserPreferences &p)
        : span(s), ast(a), cancellationToken(c), preferences(p)
    {
    }
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

void Visitor(const ir::AstNode *node, InlayHintProcessingContext &context, InlayHintList *result)
{
    if (!ShouldProcessNode(node, context.span)) {
        return;
    }
    if (context.cancellationToken.IsCancellationRequested() && IsCancellableNode(node)) {
        return;
    }
    ProcessNodeBasedOnPreferences(node, context.ast, context.preferences, result);
}

InlayHintList ProvideInlayHintsImpl(es2panda_Context *context, const TextSpan *span,
                                    CancellationToken &cancellationToken, UserPreferences &preferences)
{
    auto result = InlayHintList();
    if (context == nullptr) {
        return {};
    }
    const auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx == nullptr) {
        return {};
    }
    const auto *parent = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    if (parent == nullptr) {
        return {};
    }
    InlayHintProcessingContext processingContext = {span, parent, cancellationToken, preferences};
    const auto *searchNode = TryGetIncludingNode(parent, span);
    searchNode = searchNode == nullptr ? parent : searchNode;
    searchNode->FindChild([&processingContext, &result](ir::AstNode *childNode) {
        Visitor(childNode, processingContext, &result);
        return false;
    });

    return result;
}

}  // namespace ark::es2panda::lsp