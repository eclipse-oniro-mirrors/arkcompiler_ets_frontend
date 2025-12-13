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

#include "suggestion_diagnostics.h"
#include "internal_api.h"
#include "libarkbase/utils/arena_containers.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace ark::es2panda::lsp {

std::vector<FileDiagnostic> GetSuggestionDiagnosticsImpl(ir::AstNode *astNode, es2panda_Context *context)
{
    std::unordered_map<std::string, bool> visitedNestedConvertibleFunctions;
    std::vector<FileDiagnostic> diags;

    if (astNode != nullptr) {
        Check(astNode, diags, visitedNestedConvertibleFunctions, context);
        if (!diags.empty()) {
            std::sort(diags.begin(), diags.end(), [](const FileDiagnostic &d1, const FileDiagnostic &d2) {
                return d1.diagnostic.range_.start.line_ < d2.diagnostic.range_.start.line_;
            });
        }
    }
    return diags;
}

void Check(ir::AstNode *node, std::vector<FileDiagnostic> &diag, std::unordered_map<std::string, bool> &visitedFunc,
           es2panda_Context *context)
{
    if (CanBeConvertedToAsync(node)) {
        AddConvertToAsyncFunctionDiagnostics(node, diag, visitedFunc, context);
    }

    node->FindChild([&diag, &visitedFunc, &node, &context](ir::AstNode *childNode) {
        // It should only Check direct child node istead of all child and grandchild node, according to tsc
        if (childNode->Parent() == node) {
            Check(childNode, diag, visitedFunc, context);
        }
        return false;
    });
}

bool CheckGivenTypeExistInChilds(ir::AstNode *node, ir::AstNodeType type)
{
    auto desiredNode = node->FindChild([type](ir::AstNode *n) {
        return n->Type() != type;
        return false;
    });
    return desiredNode != nullptr;
}

void AddConvertToAsyncFunctionDiagnostics(ir::AstNode *node, std::vector<FileDiagnostic> &diag,
                                          std::unordered_map<std::string, bool> &visitedFunc, es2panda_Context *context)
{
    if (IsConvertibleFunction(node, visitedFunc) && (visitedFunc.count(GetKeyFromNode(node)) == 0)) {
        auto ctx = reinterpret_cast<public_lib::Context *>(context);
        // The line and col should start from 1 istead of 0
        auto index = lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto sourceStartLocation = index.GetLocation(node->Range().start);
        auto sourceEndLocation = index.GetLocation(node->Range().end);
        Position posStart(sourceStartLocation.line, sourceStartLocation.col);
        Position posEnd(sourceEndLocation.line, sourceEndLocation.col);
        Range range(posStart, posEnd);
        const std::string message = "This_may_be_converted_to_an_async_function";
        Diagnostic diagnostic(range, {}, {}, DiagnosticSeverity::Hint, 0, message, {}, {}, {});

        diag.push_back(lsp::CreateDiagnosticForNode(reinterpret_cast<es2panda_AstNode *>(node), diagnostic, context));
    }
}

bool IsConvertibleFunction(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc)
{
    return !(node->IsAsync()) && CheckGivenTypeExistInChilds(node, ir::AstNodeType::BLOCK_STATEMENT) &&
           HasReturnStatementWithPromiseHandler(node, visitedFunc);
}

bool HasReturnStatementWithPromiseHandler(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc)
{
    auto desiredNode = node->FindChild([&visitedFunc](ir::AstNode *childNode) {
        return IsReturnStatementWithFixablePromiseHandler(childNode, visitedFunc);
    });
    return desiredNode != nullptr;
}

bool IsReturnStatementWithFixablePromiseHandler(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc)
{
    auto desiredNode = node->FindChild([&visitedFunc](ir::AstNode *childNode) {
        if (childNode->Type() == ir::AstNodeType::RETURN_STATEMENT) {
            return childNode->IsReturnStatement() &&
                   CheckGivenTypeExistInChilds(childNode, ir::AstNodeType::CALL_EXPRESSION) &&
                   IsFixablePromiseHandler(childNode, visitedFunc);
        }
        return false;
    });
    return desiredNode != nullptr;
}

ir::AstNode *HasSpecificIdentifier(ir::AstNode *node)
{
    std::vector<ark::es2panda::ir::AstNode *> memberExpNodes;
    std::vector<ark::es2panda::ir::AstNode *> identNodes;
    node->FindChild([&memberExpNodes](ir::AstNode *n) {
        if (n->Type() == ark::es2panda::ir::AstNodeType::MEMBER_EXPRESSION) {
            memberExpNodes.push_back(n);
        }
        return false;
    });

    if (memberExpNodes.empty()) {
        return nullptr;
    }
    memberExpNodes.at(0)->FindChild([&identNodes](ir::AstNode *in) {
        if (in->Type() == ark::es2panda::ir::AstNodeType::IDENTIFIER) {
            identNodes.push_back(in);
        }
        return false;
    });
    if (identNodes.empty()) {
        return nullptr;
    }
    for (auto n : identNodes) {
        if (n->AsIdentifier()->Name() == "then" || n->AsIdentifier()->Name() == "catch" ||
            n->AsIdentifier()->Name() == "finally") {
            return n;
        }
    }
    return identNodes.at(0);
}

bool IsFixablePromiseHandler(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc)
{
    auto desiredNode = node->FindChild([&visitedFunc](ir::AstNode *childNode) {
        if (childNode->Type() == ark::es2panda::ir::AstNodeType::CALL_EXPRESSION &&
            (!IsPromiseHandler(childNode) || !HasSupportedNumberOfArguments(childNode) ||
             childNode->FindChild(
                 [&visitedFunc](ir::AstNode *n) { return IsFixablePromiseArgument(n, visitedFunc); }) == nullptr)) {
            return false;
        }
        while (IsPromiseHandler(childNode) || HasSpecificIdentifier(childNode) != nullptr) {
            if (childNode->Type() == ir::AstNodeType::CALL_EXPRESSION &&
                (!HasSupportedNumberOfArguments(childNode) || childNode->FindChild([&visitedFunc](ir::AstNode *n) {
                    return IsFixablePromiseHandler(n, visitedFunc);
                }) != nullptr)) {
                return false;
            }
            if (childNode->IsCallExpression()) {
                childNode = childNode->AsCallExpression()->Callee();
            } else if (childNode->IsExpression()) {
                childNode->AsExpression();
            }
            return false;
        }
        return true;
    });
    return desiredNode != nullptr;
}

bool HasPropertyAccessExpressionWithName(ir::AstNode *node, const std::string &func)
{
    return node->IsIdentifier() && node->AsIdentifier()->Name().Utf8() == func;
}

bool IsPromiseHandler(ir::AstNode *node)
{
    ir::AstNode *currentIdentNode = nullptr;
    if (HasSpecificIdentifier(node) == nullptr) {
        return false;
    }
    currentIdentNode = HasSpecificIdentifier(node);
    return ir::AstNodeType::CALL_EXPRESSION == node->Type() &&
           (HasPropertyAccessExpressionWithName(currentIdentNode, "then") ||
            HasPropertyAccessExpressionWithName(currentIdentNode, "catch") ||
            HasPropertyAccessExpressionWithName(currentIdentNode, "finally"));
}

bool IsExist(ArenaVector<ir::Expression *> args)
{
    return std::any_of(args.begin(), args.end(), [](ir::Expression *e) {
        return e->Type() == ir::AstNodeType::TS_NULL_KEYWORD ||
               (e->IsIdentifier() && e->AsIdentifier()->Name() == "undefined");
    });
}

bool HasSupportedNumberOfArguments(ir::AstNode *node)
{
    ir::AstNode *currentIdentNode = nullptr;
    if (HasSpecificIdentifier(node) == nullptr) {
        return false;
    }
    currentIdentNode = HasSpecificIdentifier(node);
    auto const &name = currentIdentNode->AsIdentifier()->Name();
    auto const maxArguments = name == "then" ? 2 : name == "catch" ? 1 : name == "finally" ? 1 : 0;
    if ((int)(node->AsCallExpression()->Arguments().size()) > maxArguments) {
        return false;
    }
    if ((int)(node->AsCallExpression()->Arguments().size()) < maxArguments) {
        return true;
    }
    auto args = node->AsCallExpression()->Arguments();

    return maxArguments == 1 || IsExist(args);
}

bool IsFixablePromiseArgument(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc)
{
    switch (node->Type()) {
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::IDENTIFIER:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            visitedFunc[GetKeyFromNode(node)] = true;
            return true;
        case ir::AstNodeType::TS_NULL_KEYWORD:
            return true;
        default:
            return false;
    }
}

std::string GetKeyFromNode(ir::AstNode *node)
{
    return std::to_string(node->Start().index) + ":" + std::to_string(node->End().index);
}

bool CanBeConvertedToAsync(ir::AstNode *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            return true;
        default:
            return false;
    }
}
}  // namespace ark::es2panda::lsp