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

// original_header.h
#ifndef FORWARD_DECL_FOR_PARSERIMPL_H
#define FORWARD_DECL_FOR_PARSERIMPL_H

#include "ir/base/scriptFunctionSignature.h"
#include "parser/context/parserContext.h"
#include "varbinder/scope.h"

namespace ark::es2panda::lexer {
enum class TokenFlags : uint32_t;
class LexerPosition;
class Token;
class Lexer;
}  // namespace ark::es2panda::lexer

namespace ark::es2panda::ir {
class ArrowFunctionExpression;
class AstNode;
class BlockStatement;
class BreakStatement;
class CallExpression;
class ClassDeclaration;
class ClassDefinition;
class ContinueStatement;
class DoWhileStatement;
class ExportAllDeclaration;
class ExportDefaultDeclaration;
class ExportNamedDeclaration;
class ExportNamedDeclaration;
class Expression;
class FunctionDeclaration;
class FunctionExpression;
class Identifier;
class IfStatement;
class ImportDeclaration;
class LabelledStatement;
class NewExpression;
class ObjectExpression;
class ReturnStatement;
class ScriptFunction;
class SequenceExpression;
class SpreadElement;
class Statement;
class StringLiteral;
class SwitchCaseStatement;
class SwitchStatement;
class TemplateLiteral;
class ThrowStatement;
class TryStatement;
class VariableDeclaration;
class WhileStatement;
class WithStatement;
class MemberExpression;
class MethodDefinition;
class Property;
class YieldExpression;
class MetaProperty;
class EmptyStatement;
class DebuggerStatement;
class CatchClause;
class VariableDeclarator;
class ClassElement;

enum class PropertyKind;
enum class MethodDefinitionKind : uint8_t;
enum class ModifierFlags : uint32_t;
enum class Primitives;
enum class ClassDefinitionModifiers : uint32_t;
enum class CatchClauseType;
enum class VariableDeclaratorFlag : uint8_t;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {

class ETSParser;

using FunctionSignature = std::tuple<ir::FunctionSignature, ark::es2panda::ir::ScriptFunctionFlags>;

// NOLINTBEGIN(modernize-avoid-c-arrays)
inline constexpr char const ARRAY_FORMAT_NODE = '[';
inline constexpr char const EXPRESSION_FORMAT_NODE = 'E';
inline constexpr char const STATEMENT_FORMAT_NODE = 'S';
// NOLINTEND(modernize-avoid-c-arrays)

class ClassElementDescriptor {
public:
    explicit ClassElementDescriptor(ArenaAllocator *allocator) : decorators(allocator->Adapter()) {}

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    ArenaVector<ir::Decorator *> decorators;
    ir::MethodDefinitionKind methodKind {};
    ParserStatus newStatus {};
    ir::ModifierFlags modifiers {};
    lexer::SourcePosition methodStart {};
    lexer::SourcePosition propStart {};
    bool isPrivateIdent {};
    bool hasSuperClass {};
    bool isGenerator {};
    bool invalidComputedProperty {};
    bool isComputed {};
    bool isIndexSignature {};
    bool classMethod {};
    bool classField {};
    varbinder::LocalScope *staticFieldScope {};
    varbinder::LocalScope *staticMethodScope {};
    varbinder::LocalScope *instanceFieldScope {};
    varbinder::LocalScope *instanceMethodScope {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class ArrowFunctionDescriptor {
public:
    explicit ArrowFunctionDescriptor(ArenaVector<ir::Expression *> &&p, lexer::SourcePosition sl, ParserStatus ns)
        : params(p), startLoc(sl), newStatus(ns)
    {
    }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    ArenaVector<ir::Expression *> params;
    lexer::SourcePosition startLoc;
    ParserStatus newStatus;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class ArrowFunctionContext;
}  // namespace ark::es2panda::parser
#endif
