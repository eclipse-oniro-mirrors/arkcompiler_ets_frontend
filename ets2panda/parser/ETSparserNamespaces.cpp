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

#include "ETSparser.h"
#include "lexer/lexer.h"
#include "ir/astNode.h"
#include "ir/ets/etsModule.h"
#include "utils/arena_containers.h"
#include "util/errorRecovery.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::parser {

using namespace std::literals::string_literals;

ir::ETSModule *ETSParser::ParseNamespaceStatement(ir::ModifierFlags memberModifiers)
{
    auto modifiers = ir::ModifierFlags::NONE;
    if (((memberModifiers & ir::ModifierFlags::EXPORT) != 0)) {
        modifiers |= ir::ModifierFlags::EXPORT;
    }
    if (((memberModifiers & ir::ModifierFlags::DEFAULT_EXPORT) != 0)) {
        modifiers |= ir::ModifierFlags::DEFAULT_EXPORT;
    }
    if ((memberModifiers & ir::ModifierFlags::DECLARE) != 0 || InAmbientContext()) {
        modifiers |= ir::ModifierFlags::DECLARE;
        GetContext().Status() |= ParserStatus::IN_AMBIENT_CONTEXT;
    }
    GetContext().Status() |= ParserStatus::IN_NAMESPACE;
    IncrementNamespaceNestedRank();

    auto *result = ParseNamespace(modifiers);

    DecrementNamespaceNestedRank();
    if (GetNamespaceNestedRank() == 0) {
        GetContext().Status() &= ~ParserStatus::IN_NAMESPACE;
    }
    if ((memberModifiers & ir::ModifierFlags::DECLARE) != 0) {
        GetContext().Status() &= ~ParserStatus::IN_AMBIENT_CONTEXT;
    }
    return result->AsETSModule();
}

ir::Statement *ETSParser::ParseNamespace(ir::ModifierFlags flags)
{
    if ((GetContext().Status() & ParserStatus::IN_NAMESPACE) == 0) {
        LogError(diagnostic::NAMESPACE_ONLY_TOP_OR_IN_NAMESPACE);
    }
    ir::ETSModule *ns = ParseNamespaceImp(flags);
    ES2PANDA_ASSERT(ns != nullptr);
    return ns;
}

ir::ETSModule *ETSParser::ParseNamespaceImp(ir::ModifierFlags flags)
{
    auto nsStart = Lexer()->GetToken().Start();
    Lexer()->NextToken();
    auto *result =
        AllocNode<ir::ETSModule>(Allocator(), ArenaVector<ir::Statement *>(Allocator()->Adapter()), ExpectIdentifier(),
                                 ir::ModuleFlag::NAMESPACE, GetContext().GetLanguage(), globalProgram_);
    ir::ETSModule *parent = result;
    ir::ETSModule *child = nullptr;
    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD) {
        Lexer()->NextToken();
        auto start = Lexer()->GetToken().Start();
        child = AllocNode<ir::ETSModule>(Allocator(), ArenaVector<ir::Statement *>(Allocator()->Adapter()),
                                         ExpectIdentifier(), ir::ModuleFlag::NAMESPACE, GetContext().GetLanguage(),
                                         globalProgram_);
        child->SetParent(parent);
        child->SetRange({start, Lexer()->GetToken().Start()});
        child->AddModifier(ir::ModifierFlags::EXPORT);
        if ((flags & ir::ModifierFlags::DECLARE) != 0) {
            child->AddModifier(ir::ModifierFlags::DECLARE);
        }
        parent->AddStatement(child);
        parent = child;
    }
    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE);
    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());
    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
        util::ErrorRecursionGuard infiniteLoopBlocker(Lexer());
        if (Lexer()->GetToken().Type() == lexer::TokenType::EOS) {
            LogError(diagnostic::UNEXPECTED_TOKEN);
            break;
        }
        if (Lexer()->TryEatTokenType(lexer::TokenType::PUNCTUATOR_SEMI_COLON)) {
            continue;
        }
        auto st = ParseTopLevelStatement();
        statements.emplace_back(st);
    }
    auto nsEnd = Lexer()->GetToken().End();
    Lexer()->NextToken();
    if (child != nullptr) {
        child->SetNamespaceChainLastNode();
        child->SetStatements(std::move(statements));
    } else {
        result->SetNamespaceChainLastNode();
        result->SetStatements(std::move(statements));
    }
    result->AddModifier(flags);
    result->SetRange({nsStart, nsEnd});
    return result;
}

}  // namespace ark::es2panda::parser
