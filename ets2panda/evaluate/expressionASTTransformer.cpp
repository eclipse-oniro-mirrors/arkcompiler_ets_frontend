/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "evaluate/expressionASTTransformer.h"
#include "checker/ETSchecker.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/conditionalExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/unaryExpression.h"
#include "ir/expressions/updateExpression.h"
#include "ir/expressions/thisExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/booleanLiteral.h"
#include "ir/expressions/literals/nullLiteral.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "ir/expressions/literals/bigIntLiteral.h"
#include "ir/expressions/literals/charLiteral.h"
#include "ir/expressions/sequenceExpression.h"
#include "ir/expressions/typeofExpression.h"
#include "ir/ts/tsNonNullExpression.h"
#include "ir/ts/tsQualifiedName.h"
#include "ir/ts/tsStringKeyword.h"
#include "ir/ts/tsNumberKeyword.h"
#include "ir/ts/tsBooleanKeyword.h"
#include "ir/ts/tsObjectKeyword.h"
#include "ir/ts/tsAnyKeyword.h"
#include "ir/ts/tsVoidKeyword.h"
#include "ir/ts/tsUndefinedKeyword.h"
#include "ir/ts/tsUnknownKeyword.h"
#include "ir/ts/tsNullKeyword.h"
#include "ir/ts/tsBigintKeyword.h"
#include "ir/ts/tsNeverKeyword.h"
#include "ir/ts/tsParenthesizedType.h"
#include "ir/ts/tsArrayType.h"
#include "ir/ts/tsUnionType.h"
#include "ir/statements/returnStatement.h"
#include "ir/statements/throwStatement.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/ifStatement.h"
#include "ir/statements/expressionStatement.h"
#include "evaluate/helpers.h"
#include "generated/tokenType.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/templateLiteral.h"
#include "ir/base/spreadElement.h"
#include "ir/base/property.h"
#include "ir/ets/etsPrimitiveType.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsNewArrayInstanceExpression.h"
#include "ir/ts/tsTypeReference.h"

#include <atomic>
#include <securec.h>

namespace ark::es2panda::evaluate {

namespace {

// Unique counter for generated temp variable names ($eval_*).
static std::atomic<uint64_t> g_evalVarCounter {0};

// TokenType -> DebuggerAPI method name (runtime instanceof dispatch).
const std::unordered_map<lexer::TokenType, std::string_view> kBinaryOpMap = {
    {lexer::TokenType::PUNCTUATOR_PLUS, "add"},
    {lexer::TokenType::PUNCTUATOR_MINUS, "sub"},
    {lexer::TokenType::PUNCTUATOR_MULTIPLY, "mul"},
    {lexer::TokenType::PUNCTUATOR_DIVIDE, "div"},
    {lexer::TokenType::PUNCTUATOR_MOD, "mod"},
    {lexer::TokenType::PUNCTUATOR_EXPONENTIATION, "pow"},
    {lexer::TokenType::PUNCTUATOR_LEFT_SHIFT, "shl"},
    {lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT, "shr"},
    {lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT, "ushr"},
    {lexer::TokenType::PUNCTUATOR_LESS_THAN, "lt"},
    {lexer::TokenType::PUNCTUATOR_GREATER_THAN, "gt"},
    {lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL, "le"},
    {lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL, "ge"},
    {lexer::TokenType::PUNCTUATOR_BITWISE_AND, "bitAnd"},
    {lexer::TokenType::PUNCTUATOR_BITWISE_OR, "bitOr"},
    {lexer::TokenType::PUNCTUATOR_BITWISE_XOR, "bitXor"},
};

// Unary operator TokenType -> DebuggerAPI method name
const std::unordered_map<lexer::TokenType, std::string_view> kUnaryOpMap = {
    {lexer::TokenType::PUNCTUATOR_MINUS, "neg"},
    {lexer::TokenType::PUNCTUATOR_PLUS, "pos"},
    {lexer::TokenType::PUNCTUATOR_TILDE, "bitNot"},
};

// ETS primitive -> DebuggerAPI toXxx method; empty view for VOID.
static std::string_view GetPrimitiveCastMethodName(ir::PrimitiveType type)
{
    switch (type) {
        case ir::PrimitiveType::DOUBLE:
            return "toDouble";
        case ir::PrimitiveType::FLOAT:
            return "toFloat";
        case ir::PrimitiveType::LONG:
            return "toLong";
        case ir::PrimitiveType::INT:
            return "toInt";
        case ir::PrimitiveType::BYTE:
            return "toByte";
        case ir::PrimitiveType::SHORT:
            return "toShort";
        case ir::PrimitiveType::CHAR:
            return "toChar";
        case ir::PrimitiveType::BOOLEAN:
            return "toBoolean";
        case ir::PrimitiveType::VOID:
            return "";
    }
    return "";
}

bool IsLogicalOp(lexer::TokenType op)
{
    return op == lexer::TokenType::PUNCTUATOR_LOGICAL_AND || op == lexer::TokenType::PUNCTUATOR_LOGICAL_OR;
}

bool IsEqualityOp(lexer::TokenType op)
{
    return op == lexer::TokenType::PUNCTUATOR_EQUAL || op == lexer::TokenType::PUNCTUATOR_NOT_EQUAL ||
           op == lexer::TokenType::PUNCTUATOR_STRICT_EQUAL || op == lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL;
}

bool IsMemberCallee(ir::Expression *callee)
{
    return callee->Type() == ir::AstNodeType::MEMBER_EXPRESSION;
}

// Boxed stdlib names shared by the `as` and array-element whitelists.
static bool IsBoxedTypeName(util::StringView name)
{
    return name.Is("String") || name.Is("Object") || name.Is("Number") || name.Is("Boolean") || name.Is("BigInt") ||
           name.Is("Int") || name.Is("Double") || name.Is("Float") || name.Is("Long") || name.Is("Byte") ||
           name.Is("Short") || name.Is("Char");
}

// Whether a simple type-reference name resolves here; user names fail BOUND.
static bool IsBuiltinTypeRefName(util::StringView name)
{
    return IsBoxedTypeName(name) || name.Is("undefined") || name.Is("null") || name.Is("void") || name.Is("never") ||
           name.Is("unknown") || name.Is("Any");
}

static bool IsBuiltinCompositeType(ir::TypeNode *typeNode);

// Whether a TypeNode is a compiler-builtin keyword type (no import needed).
static bool IsBuiltinTypeNode(ir::TypeNode *typeNode)
{
    switch (typeNode->Type()) {
        case ir::AstNodeType::TS_STRING_KEYWORD:
        case ir::AstNodeType::TS_NUMBER_KEYWORD:
        case ir::AstNodeType::TS_BOOLEAN_KEYWORD:
        case ir::AstNodeType::TS_OBJECT_KEYWORD:
        case ir::AstNodeType::TS_ANY_KEYWORD:
        case ir::AstNodeType::TS_VOID_KEYWORD:
        case ir::AstNodeType::TS_UNDEFINED_KEYWORD:
        case ir::AstNodeType::TS_UNKNOWN_KEYWORD:
        case ir::AstNodeType::TS_NULL_KEYWORD:
        case ir::AstNodeType::TS_BIGINT_KEYWORD:
        case ir::AstNodeType::TS_NEVER_KEYWORD:
            return true;

        // composite types are builtin iff every member/element is builtin
        case ir::AstNodeType::TS_ARRAY_TYPE:
        case ir::AstNodeType::TS_UNION_TYPE:
        case ir::AstNodeType::TS_PARENT_TYPE:
            return IsBuiltinCompositeType(typeNode);

        case ir::AstNodeType::TS_TYPE_REFERENCE: {
            auto *tn = static_cast<ir::TSTypeReference *>(typeNode)->TypeName();
            return tn != nullptr && tn->IsIdentifier() && IsBuiltinTypeRefName(tn->AsIdentifier()->Name());
        }

        case ir::AstNodeType::ETS_PRIMITIVE_TYPE:
            // ETS primitives
            return true;

        case ir::AstNodeType::ETS_UNDEFINED_TYPE:
        case ir::AstNodeType::ETS_NULL_TYPE:
        case ir::AstNodeType::ETS_NEVER_TYPE:
            // ETS keyword types
            return true;

        case ir::AstNodeType::ETS_TYPE_REFERENCE:
            // ETS named type references
            return true;

        default:
            return false;
    }
}

static bool IsBuiltinCompositeType(ir::TypeNode *typeNode)
{
    switch (typeNode->Type()) {
        case ir::AstNodeType::TS_ARRAY_TYPE:
            return IsBuiltinTypeNode(const_cast<ir::TypeNode *>(
                static_cast<const ir::TypeNode *>(static_cast<ir::TSArrayType *>(typeNode)->ElementType())));
        case ir::AstNodeType::TS_UNION_TYPE:
            for (auto *member : static_cast<ir::TSUnionType *>(typeNode)->Types()) {
                if (!IsBuiltinTypeNode(member)) {
                    return false;
                }
            }
            return true;
        case ir::AstNodeType::TS_PARENT_TYPE:
            return IsBuiltinTypeNode(static_cast<ir::TypeNode *>(
                const_cast<ir::Expression *>(static_cast<ir::TSParenthesizedType *>(typeNode)->Type())));
        default:
            ES2PANDA_UNREACHABLE();
    }
}

// Graceful degrade: NullLiteral + LOG(ERROR).
static ir::Expression *ReportUnsupported(util::StringView message, checker::ETSChecker *checker)
{
    LOG(ERROR, ES2PANDA) << "Debugger evaluation: " << message;
    return checker->AllocNode<ir::NullLiteral>();
}

// Bridges into an Object parameter: emits no code for Object sources; Any
// sources (lifted-conditional results) get a real cast to Object.
static ir::Expression *EnsureObject(checker::ETSChecker *checker, ir::Expression *expr)
{
    return checker->AllocNode<ir::TSAsExpression>(expr, helpers::CreateETSTypeReference(checker, "Object"), false);
}

// Joins dotted name segments (arena-allocated), left to right.
static util::StringView JoinNameSegments(checker::ETSChecker *checker, std::vector<util::StringView> &&segments)
{
    std::string joined;
    for (const auto &seg : segments) {
        if (!joined.empty()) {
            joined += ".";
        }
        joined += std::string(seg.Utf8());
    }
    auto *buf = static_cast<char *>(checker->Allocator()->Alloc(joined.size() + 1));
    [[maybe_unused]] auto err = memcpy_s(buf, joined.size() + 1, joined.c_str(), joined.size() + 1);
    ES2PANDA_ASSERT(err == EOK);
    return util::StringView(buf);
}

// Collects the qualified name of an ETSTypeReference: a folded TSQualifiedName
// in one part, or a multi-part chain. Type arguments are dropped (erasure).
static util::StringView ExtractQualifiedPartChainName(checker::ETSChecker *checker, ir::ETSTypeReference *typeRef)
{
    auto collectNameSegments = [](ir::Expression *name, std::vector<util::StringView> &segments) -> bool {
        // TSQualifiedName nests leftwards (a.b.Foo = QN(QN(a, b), Foo)):
        // collect right identifiers along the left spine, append reversed.
        std::vector<util::StringView> reversed;
        while (name->IsTSQualifiedName()) {
            reversed.push_back(name->AsTSQualifiedName()->Right()->Name());
            name = name->AsTSQualifiedName()->Left();
        }
        if (!name->IsIdentifier()) {
            return false;
        }
        segments.push_back(name->AsIdentifier()->Name());
        segments.insert(segments.end(), reversed.rbegin(), reversed.rend());
        return true;
    };

    std::vector<ir::ETSTypeReferencePart *> parts;
    for (auto *part = typeRef->Part(); part != nullptr; part = part->Previous()) {
        parts.push_back(part);
    }
    std::vector<util::StringView> segments;
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        if ((*it)->Name() == nullptr || !collectNameSegments((*it)->Name(), segments)) {
            return util::StringView();
        }
    }
    if (segments.empty()) {
        return util::StringView();
    }
    return JoinNameSegments(checker, std::move(segments));
}

// Class name of a new-expression / instanceof type reference, including
// dotted qualified names; empty for non-reference forms.
static util::StringView ExtractNewClassTypeName(checker::ETSChecker *checker, ir::Expression *typeRef)
{
    if (typeRef == nullptr) {
        return util::StringView();
    }
    if (typeRef->IsIdentifier()) {
        return typeRef->AsIdentifier()->Name();
    }
    if (typeRef->IsETSTypeReference()) {
        return ExtractQualifiedPartChainName(checker, typeRef->AsETSTypeReference());
    }
    return util::StringView();
}

// Qualified name of a type annotation; empty for other forms. Single-segment
// results are identical to the previous simple-name behavior.
static util::StringView ExtractTypeRefName(checker::ETSChecker *checker, ir::TypeNode *typeRef)
{
    if (typeRef == nullptr) {
        return util::StringView();
    }
    if (typeRef->IsETSTypeReference()) {
        return ExtractQualifiedPartChainName(checker, typeRef->AsETSTypeReference());
    } else if (typeRef->IsTSTypeReference()) {
        auto *tn = typeRef->AsTSTypeReference()->TypeName();
        if (tn != nullptr && tn->IsIdentifier()) {
            return tn->AsIdentifier()->Name();
        }
    }
    return util::StringView();
}

// Whether an array-element type annotation resolves in the isolated eval module
// (primitives/keywords/builtin boxed/Array; recursive; under-inclusion is safe).
static ir::TypeNode *UnparenthesizeType(ir::TypeNode *typeNode);

// Keyword type nodes accepted in array-element position (both the TS_*_KEYWORD
// and the dedicated ETS_*_TYPE node families).
static bool IsKeywordTypeNode(ir::TypeNode *elemType)
{
    switch (elemType->Type()) {
        case ir::AstNodeType::TS_STRING_KEYWORD:
        case ir::AstNodeType::TS_NUMBER_KEYWORD:
        case ir::AstNodeType::TS_BOOLEAN_KEYWORD:
        case ir::AstNodeType::TS_OBJECT_KEYWORD:
        case ir::AstNodeType::TS_ANY_KEYWORD:
        case ir::AstNodeType::TS_VOID_KEYWORD:
        case ir::AstNodeType::TS_UNDEFINED_KEYWORD:
        case ir::AstNodeType::TS_UNKNOWN_KEYWORD:
        case ir::AstNodeType::TS_NULL_KEYWORD:
        case ir::AstNodeType::TS_BIGINT_KEYWORD:
        case ir::AstNodeType::TS_NEVER_KEYWORD:
        case ir::AstNodeType::ETS_UNDEFINED_TYPE:
        case ir::AstNodeType::ETS_NULL_TYPE:
        case ir::AstNodeType::ETS_NEVER_TYPE:
            return true;
        default:
            return false;
    }
}

// Whether an ETS type-reference element is a whitelisted single-segment name
// with recursively bindable type arguments.
bool IsElementBindableHere(ir::TypeNode *elemType);
bool IsBindableTypeReference(ir::TypeNode *elemType)
{
    auto *part = elemType->AsETSTypeReference()->Part();
    if (part == nullptr || part->Previous() != nullptr || part->Name() == nullptr || !part->Name()->IsIdentifier()) {
        return false;  // qualified / folded names, broken parts
    }
    auto name = part->Name()->AsIdentifier()->Name();
    if (!IsBuiltinTypeRefName(name) && !name.Is("Array")) {
        return false;
    }
    // type arguments must be bindable too, else the native path fails BIND
    if (part->TypeParams() != nullptr) {
        for (auto *param : part->TypeParams()->Params()) {
            if (!IsElementBindableHere(param)) {
                return false;
            }
        }
    }
    return true;
}

bool IsElementBindableHere(ir::TypeNode *elemType)
{
    elemType = UnparenthesizeType(elemType);
    if (elemType->IsETSPrimitiveType()) {
        return true;
    }
    if (elemType->IsTSArrayType()) {
        return IsElementBindableHere(
            static_cast<ir::TypeNode *>(const_cast<ir::TypeNode *>(elemType->AsTSArrayType()->ElementType())));
    }
    if (elemType->IsETSTypeReference()) {
        return IsBindableTypeReference(elemType);
    }
    if (elemType->IsTSTypeReference()) {
        auto *tn = elemType->AsTSTypeReference()->TypeName();
        return tn != nullptr && tn->IsIdentifier() && IsBuiltinTypeRefName(tn->AsIdentifier()->Name());
    }
    if (elemType->IsTSUnionType()) {
        for (auto *member : elemType->AsTSUnionType()->Types()) {
            if (!IsElementBindableHere(member)) {
                return false;
            }
        }
        return !elemType->AsTSUnionType()->Types().empty();
    }
    return IsKeywordTypeNode(elemType);
}

// Literal kinds that pass through unchanged.
static bool IsLiteralNode(const ir::Expression *expr)
{
    return expr->IsStringLiteral() || expr->IsNumberLiteral() || expr->IsBooleanLiteral() || expr->IsCharLiteral() ||
           expr->IsNullLiteral() || expr->IsUndefinedLiteral() || expr->IsBigIntLiteral();
}

// Unwraps parenthesized type nodes (`new (int)[2](0)`).
static ir::TypeNode *UnparenthesizeType(ir::TypeNode *typeNode)
{
    while (typeNode->IsTSParenthesizedType()) {
        typeNode = static_cast<ir::TypeNode *>(const_cast<ir::Expression *>(typeNode->AsTSParenthesizedType()->Type()));
    }
    return typeNode;
}

// Whether `new T[...]`'s element type resolves here; user types fail BIND
// (ESE0371).
static bool IsEvaluatableElementType(ir::TypeNode *typeNode)
{
    typeNode = UnparenthesizeType(typeNode);
    if (typeNode->IsETSPrimitiveType()) {
        return typeNode->AsETSPrimitiveType()->GetPrimitiveType() != ir::PrimitiveType::VOID;
    }
    if (typeNode->IsETSTypeReference()) {
        // single-segment boxed names only (boot classes for Array.create)
        auto *part = typeNode->AsETSTypeReference()->Part();
        if (part == nullptr || part->Previous() != nullptr || part->Name() == nullptr ||
            !part->Name()->IsIdentifier()) {
            return false;
        }
        return IsBoxedTypeName(part->Name()->AsIdentifier()->Name());
    }
    return false;  // generics, nested arrays
}

// Whether a DebuggerAPI static call carries an unknown-typed result:
// bare get / getThis, or castAs with the "Array" class-name argument.
bool IsUnknownTypedDebuggerApiCall(ir::CallExpression *call)
{
    auto *callee = call->Callee();
    if (callee == nullptr || !callee->IsMemberExpression()) {
        return false;
    }
    auto *prop = callee->AsMemberExpression()->Property();
    if (prop == nullptr || !prop->IsIdentifier()) {
        return false;
    }
    auto methodName = prop->AsIdentifier()->Name();
    auto *obj = callee->AsMemberExpression()->Object();
    if (obj == nullptr || !obj->IsIdentifier() || !obj->AsIdentifier()->Name().Is("DebuggerAPI")) {
        return false;
    }
    if (methodName.Is("get") || methodName.Is("getThis")) {
        return true;
    }
    if (!methodName.Is("castAs")) {
        return false;
    }
    auto &callArgs = call->Arguments();
    return !callArgs.empty() && callArgs.back()->IsStringLiteral() &&
           callArgs.back()->AsStringLiteral()->Str().Is("Array");
}

// Whether an `as` wrapper's annotation is exactly the single-segment Object
// reference (the member / call / super / string-subscript / optional-chain /
// new-class-instance transform outputs).
bool IsObjectAnnotation(ir::Expression *arg)
{
    auto *ann = arg->AsTSAsExpression()->TypeAnnotation();
    if (ann == nullptr || !ann->IsETSTypeReference()) {
        return false;
    }
    auto *part = ann->AsETSTypeReference()->Part();
    return part != nullptr && part->Previous() == nullptr && part->Name() != nullptr && part->Name()->IsIdentifier() &&
           part->Name()->AsIdentifier()->Name().Is("Object");
}

// Whether a transformed spread argument needs bridging to `as Array<Object>`
// (ESE0049: Object is not iterable). Matches the four Object-typed outputs:
// bare get/getThis, castAs(..., "Array"), proxy $_get, `as Object` wrappers.
bool NeedsSpreadArrayBridge(ir::Expression *arg)
{
    if (arg->IsCallExpression()) {
        auto *callee = arg->AsCallExpression()->Callee();
        if (IsUnknownTypedDebuggerApiCall(arg->AsCallExpression())) {
            return true;
        }
        // proxy $_get calls (numeric element access; '$_' cannot start a
        // user identifier, so no user method can collide)
        auto *prop =
            callee != nullptr && callee->IsMemberExpression() ? callee->AsMemberExpression()->Property() : nullptr;
        return prop != nullptr && prop->IsIdentifier() && prop->AsIdentifier()->Name().Is("$_get");
    }
    return arg->IsTSAsExpression() && IsObjectAnnotation(arg);
}

// Arena-allocates a unique temp variable name.
static util::StringView MakeUniqueName(checker::ETSChecker *checker, util::StringView prefix)
{
    // Atomic with relaxed order reason: unique id generation only, no
    // synchronization or ordering with other memory is required
    std::string name =
        std::string(prefix.Utf8()) + "_" + std::to_string(g_evalVarCounter.fetch_add(1, std::memory_order_relaxed));
    auto *buf = static_cast<char *>(checker->Allocator()->Alloc(name.size() + 1));
    [[maybe_unused]] auto err = memcpy_s(buf, name.size() + 1, name.c_str(), name.size() + 1);
    ES2PANDA_ASSERT(err == EOK);
    return util::StringView(buf);
}

}  // namespace

ExpressionASTTransformer::ExpressionASTTransformer(checker::ETSChecker *checker) : checker_(checker) {}

// Harvests a TransformAssignment block's statements into *stmts; returns its
// return-value expression.
static ir::Expression *HarvestBlock(ir::Statement *blockStmt, ArenaVector<ir::Statement *> *stmts)
{
    auto *block = blockStmt->AsBlockStatement();
    auto &inner = block->Statements();
    ES2PANDA_ASSERT(!inner.empty());
    for (size_t i = 0; i + 1 < inner.size(); i++) {
        stmts->push_back(inner[i]);
    }
    ES2PANDA_ASSERT(inner.back()->IsReturnStatement());
    return inner.back()->AsReturnStatement()->Argument();
}

ir::Statement *ExpressionASTTransformer::Transform(ir::Expression *expression)
{
    if (expression->IsAssignmentExpression()) {
        auto *assign = expression->AsAssignmentExpression();
        if (assign->Left()->IsArrayExpression() || assign->Left()->IsObjectExpression()) {
            return checker_->AllocNode<ir::ReturnStatement>(
                ReportUnsupported("Destructuring assignment is not supported in debugger evaluation", checker_));
        }
        return TransformAssignment(assign);
    }

    ArenaVector<ir::Statement *> stmts(checker_->Allocator()->Adapter());
    ir::Expression *transformed = TransformExpression(expression, &stmts);
    if (stmts.empty()) {
        return checker_->AllocNode<ir::ReturnStatement>(transformed);
    }
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(transformed));
    return BuildAssignReturnBlock(std::move(stmts));
}

ir::Expression *ExpressionASTTransformer::TransformExpression(ir::Expression *node, ArenaVector<ir::Statement *> *stmts)
{
    switch (node->Type()) {
        // Values and operators; the rest delegates further.
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::NUMBER_LITERAL:
        case ir::AstNodeType::BOOLEAN_LITERAL:
        case ir::AstNodeType::NULL_LITERAL:
        case ir::AstNodeType::UNDEFINED_LITERAL:
        case ir::AstNodeType::BIGINT_LITERAL:
        case ir::AstNodeType::CHAR_LITERAL:
            return TransformLiteral(node);

        case ir::AstNodeType::IDENTIFIER:
            return TransformIdentifier(node->AsIdentifier());

        case ir::AstNodeType::BINARY_EXPRESSION:
            return TransformBinary(node->AsBinaryExpression(), stmts);

        case ir::AstNodeType::UNARY_EXPRESSION:
            return TransformUnary(node->AsUnaryExpression(), stmts);

        case ir::AstNodeType::UPDATE_EXPRESSION:
            return TransformUpdateExpression(node->AsUpdateExpression(), stmts);

        case ir::AstNodeType::THIS_EXPRESSION:
            return TransformThisExpression(stmts);

        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            auto *assign = node->AsAssignmentExpression();
            if (assign->Left()->IsArrayExpression() || assign->Left()->IsObjectExpression()) {
                return ReportUnsupported("Destructuring assignment is not supported in debugger evaluation", checker_);
            }
            return HarvestBlock(TransformAssignment(assign), stmts);
        }

        default:
            return TransformCompoundExpression(node, stmts);
    }
}

ir::Expression *ExpressionASTTransformer::TransformCompoundExpression(ir::Expression *node,
                                                                      ArenaVector<ir::Statement *> *stmts)
{
    switch (node->Type()) {
        // Accesses, calls and type operations.
        case ir::AstNodeType::MEMBER_EXPRESSION:
            return TransformMember(node->AsMemberExpression(), stmts);

        case ir::AstNodeType::CALL_EXPRESSION:
            return TransformCall(node->AsCallExpression(), stmts);

        case ir::AstNodeType::CONDITIONAL_EXPRESSION:
            return TransformConditional(node->AsConditionalExpression(), stmts);

        case ir::AstNodeType::CHAIN_EXPRESSION:
            return TransformOptionalChain(node->AsChainExpression()->GetExpression(), stmts);

        case ir::AstNodeType::TS_NON_NULL_EXPRESSION:
            // compile-time assertion; runtime passes through
            return TransformExpression(node->AsTSNonNullExpression()->Expr(), stmts);

        case ir::AstNodeType::SEQUENCE_EXPRESSION:
            return TransformSequenceExpression(node->AsSequenceExpression(), stmts);

        case ir::AstNodeType::TYPEOF_EXPRESSION:
            return TransformTypeofExpression(node->AsTypeofExpression(), stmts);

        case ir::AstNodeType::TS_AS_EXPRESSION:
            return TransformAsExpression(node->AsTSAsExpression(), stmts);

        case ir::AstNodeType::AWAIT_EXPRESSION:
            return ReportUnsupported("'await' is not supported in debugger evaluation", checker_);

        case ir::AstNodeType::SUPER_EXPRESSION:
            return ReportUnsupported("'super' is not available in debugger evaluation", checker_);

        case ir::AstNodeType::YIELD_EXPRESSION:
            return ReportUnsupported("'yield' is not supported in debugger evaluation", checker_);

        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            // the %% lambda class cannot be resolved at artifact load
            return ReportUnsupported("Arrow function is not supported in debugger evaluation", checker_);

        default:
            return TransformAggregateExpression(node, stmts);
    }
}

ir::Expression *ExpressionASTTransformer::TransformAggregateExpression(ir::Expression *node,
                                                                       ArenaVector<ir::Statement *> *stmts)
{
    switch (node->Type()) {
        case ir::AstNodeType::ARRAY_EXPRESSION:
            return TransformArrayLiteral(node->AsArrayExpression(), stmts);

        case ir::AstNodeType::OBJECT_EXPRESSION: {
            // no target type from the Any wrapper (ESE0063)
            return ReportUnsupported("Object literal is not supported in debugger evaluation", checker_);
        }

        case ir::AstNodeType::TEMPLATE_LITERAL:
            return TransformTemplateLiteral(node->AsTemplateLiteral(), stmts);

        case ir::AstNodeType::ETS_NEW_ARRAY_INSTANCE_EXPRESSION:
            return TransformNewArrayInstance(node->AsETSNewArrayInstanceExpression(), stmts);

        case ir::AstNodeType::ETS_NEW_MULTI_DIM_ARRAY_INSTANCE_EXPRESSION:
            // not a language feature (ESY97506)
            return ReportUnsupported("Multi-dimensional array creation is not supported", checker_);

        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION:
            return TransformNewClassInstance(node->AsETSNewClassInstanceExpression(), stmts);

        case ir::AstNodeType::ETS_CLASS_LITERAL:
            return ReportUnsupported("Class literal is not supported in debugger evaluation", checker_);

        default:
            LOG(WARNING, ES2PANDA) << "Unhandled expression type in debugger evaluation: "
                                   << static_cast<int>(node->Type());
            return checker_->AllocNode<ir::NullLiteral>();
    }
}

ir::Expression *ExpressionASTTransformer::TransformThisExpression(ArenaVector<ir::Statement *> *stmts)
{
    (void)stmts;
    auto *getThis = MakeStaticMemberAccess("DebuggerAPI", "getThis");
    ArenaVector<ir::Expression *> getThisArgs(checker_->Allocator()->Adapter());
    getThisArgs.push_back(MakeIdentifier("thread"));
    getThisArgs.push_back(MakeIdentifier("frame"));
    return checker_->AllocNode<ir::CallExpression>(getThis, std::move(getThisArgs), nullptr, false);
}

ir::Expression *ExpressionASTTransformer::TransformSequenceExpression(ir::SequenceExpression *node,
                                                                      ArenaVector<ir::Statement *> *stmts)
{
    auto &seqs = node->Sequence();
    ArenaVector<ir::Expression *> transformed(checker_->Allocator()->Adapter());
    for (size_t i = 0; i < seqs.size(); i++) {
        transformed.push_back(TransformExpression(seqs[i], stmts));
    }
    return checker_->AllocNode<ir::SequenceExpression>(std::move(transformed));
}

ir::Expression *ExpressionASTTransformer::TransformTypeofExpression(ir::TypeofExpression *node,
                                                                    ArenaVector<ir::Statement *> *stmts)
{
    // instanceof dispatch for boxed primitives vs plain Object
    auto *arg = TransformExpression(node->Argument(), stmts);
    ArenaVector<ir::Expression *> typeofArgs(checker_->Allocator()->Adapter());
    typeofArgs.push_back(EnsureObject(checker_, arg));
    return MakeDebuggerAPIStaticCall("typeofValue", std::move(typeofArgs));
}

ir::Expression *ExpressionASTTransformer::TransformUpdateExpression(ir::UpdateExpression *node,
                                                                    ArenaVector<ir::Statement *> *stmts)
{
    auto op = node->OperatorType();
    auto *one = checker_->AllocNode<ir::NumberLiteral>(lexer::Number(1));
    auto compoundOp = (op == lexer::TokenType::PUNCTUATOR_PLUS_PLUS) ? lexer::TokenType::PUNCTUATOR_PLUS_EQUAL
                                                                     : lexer::TokenType::PUNCTUATOR_MINUS_EQUAL;
    auto *assign = checker_->AllocNode<ir::AssignmentExpression>(node->Argument(), one, compoundOp);
    auto *block = TransformAssignment(assign);
    if (node->IsPrefix()) {
        return HarvestBlock(block, stmts);
    }
    // postfix: save old value, compound-assign, return old
    auto oldName = MakeUniqueName(checker_, "$eval_old");
    // Independent Identifier nodes: the checker's Id()->Parent()->Parent()
    // walk breaks if the declaration and return share one node (AST DAG).
    auto *oldDeclId = MakeIdentifier(oldName);
    auto *oldRefId = MakeIdentifier(oldName);
    auto *oldVal = TransformExpression(node->Argument(), stmts);
    auto *oldDecl = checker_->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, oldDeclId, oldVal);
    ArenaVector<ir::VariableDeclarator *> decls(checker_->Allocator()->Adapter());
    decls.push_back(oldDecl);
    stmts->push_back(checker_->AllocNode<ir::VariableDeclaration>(ir::VariableDeclaration::VariableDeclarationKind::LET,
                                                                  checker_->Allocator(), std::move(decls)));
    // Reap the writeback statements; discard the new value.
    (void)HarvestBlock(block, stmts);
    return oldRefId;
}

ir::Expression *ExpressionASTTransformer::TransformArrayLiteral(ir::ArrayExpression *node,
                                                                ArenaVector<ir::Statement *> *stmts)
{
    if (node->Elements().empty()) {
        // no element type to infer from the Any wrapper (ESE0301)
        return ReportUnsupported("Empty array literal is not supported in debugger evaluation", checker_);
    }
    ArenaVector<ir::Expression *> elements(checker_->Allocator()->Adapter());
    for (auto *elem : node->Elements()) {
        if (elem == nullptr) {
            elements.push_back(nullptr);  // sparse slot
        } else if (elem->IsSpreadElement()) {
            auto *arg = TransformExpression(elem->AsSpreadElement()->Argument(), stmts);
            // unknown-typed values bridge to Array<Object> for the checker
            if (NeedsSpreadArrayBridge(arg)) {
                arg = MakeArrayOfObjectType(arg);
            }
            elements.push_back(
                checker_->AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, checker_->Allocator(), arg));
        } else {
            elements.push_back(TransformExpression(elem, stmts));
        }
    }
    return checker_->AllocNode<ir::ArrayExpression>(std::move(elements), checker_->Allocator());
}

ir::Expression *ExpressionASTTransformer::TransformTemplateLiteral(ir::TemplateLiteral *node,
                                                                   ArenaVector<ir::Statement *> *stmts)
{
    auto &quasis = node->Quasis();
    auto &expressions = node->Expressions();
    ir::Expression *result = checker_->AllocNode<ir::StringLiteral>(quasis[0]->Cooked());
    for (size_t i = 0; i < expressions.size(); i++) {
        auto *expr = TransformExpression(expressions[i], stmts);
        ArenaVector<ir::Expression *> addArgs1(checker_->Allocator()->Adapter());
        addArgs1.push_back(EnsureObject(checker_, result));
        addArgs1.push_back(EnsureObject(checker_, expr));
        result = MakeDebuggerAPIStaticCall("add", std::move(addArgs1));
        if (i + 1 < quasis.size()) {
            auto *quasi = checker_->AllocNode<ir::StringLiteral>(quasis[i + 1]->Cooked());
            ArenaVector<ir::Expression *> addArgs2(checker_->Allocator()->Adapter());
            addArgs2.push_back(EnsureObject(checker_, result));
            addArgs2.push_back(quasi);
            result = MakeDebuggerAPIStaticCall("add", std::move(addArgs2));
        }
    }
    return result;
}

// Array types: bindable elements keep the native `as T[]`; unbindable ones
// erase to `as Array<Object>` (native checkcasts std.core.Array only).
ir::Expression *ExpressionASTTransformer::EraseArrayCast(ir::TSAsExpression *node, ir::Expression *transformedExpr)
{
    auto *unparenType = UnparenthesizeType(node->TypeAnnotation());
    if (!unparenType->IsTSArrayType()) {
        return nullptr;  // not an array cast: keep the other paths
    }
    auto *elemType =
        static_cast<ir::TypeNode *>(const_cast<ir::TypeNode *>(unparenType->AsTSArrayType()->ElementType()));
    if (IsElementBindableHere(elemType)) {
        return nullptr;  // bindable element (readonly included): native path
    }
    // readonly + erasure would checkcast the wrong runtime class
    if ((unparenType->Modifiers() & ir::ModifierFlags::READONLY_PARAMETER) != 0) {
        return ReportUnsupported(
            "Cast to readonly array of user-defined element type is not supported in debugger evaluation", checker_);
    }
    return MakeArrayOfObjectType(EnsureObject(checker_, transformedExpr));
}

ir::Expression *ExpressionASTTransformer::TransformAsExpression(ir::TSAsExpression *node,
                                                                ArenaVector<ir::Statement *> *stmts)
{
    auto *transformedExpr = TransformExpression(node->Expr(), stmts);
    if (!IsBuiltinTypeNode(node->TypeAnnotation())) {
        return ReportUnsupported("Type cast to user-defined type is not supported in debugger evaluation", checker_);
    }

    // the checker rejects Object -> primitive; use toXxx
    if (node->TypeAnnotation()->IsETSPrimitiveType()) {
        auto methodName = GetPrimitiveCastMethodName(node->TypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType());
        if (methodName.empty()) {
            return ReportUnsupported("Cast to 'void' is not supported in debugger evaluation", checker_);
        }
        ArenaVector<ir::Expression *> castArgs(checker_->Allocator()->Adapter());
        castArgs.push_back(EnsureObject(checker_, transformedExpr));
        return MakeDebuggerAPIStaticCall(methodName, std::move(castArgs));
    }

    if (auto *erased = EraseArrayCast(node, transformedExpr); erased != nullptr) {
        return erased;
    }

    // Named types (including dotted qualified names) route through runtime
    // castAs (user types unresolvable at BIND); keywords keep the native path.
    auto typeName = ExtractTypeRefName(checker_, node->TypeAnnotation());
    bool isKeywordType = typeName.Is("undefined") || typeName.Is("null") || typeName.Is("void") ||
                         typeName.Is("never") || typeName.Is("unknown") || typeName.Is("Any");
    if (!typeName.Empty() && !isKeywordType) {
        ArenaVector<ir::Expression *> castArgs(checker_->Allocator()->Adapter());
        castArgs.push_back(MakeIdentifier("thread"));
        castArgs.push_back(MakeIdentifier("frame"));
        castArgs.push_back(EnsureObject(checker_, transformedExpr));
        castArgs.push_back(MakeStringLiteral(typeName));
        return MakeDebuggerAPIStaticCall("castAs", std::move(castArgs));
    }

    // keyword types and bindable array annotations: native `as` with the
    // cloned annotation, validated by the standard checker
    auto *clonedType = static_cast<ir::TypeNode *>(node->TypeAnnotation()->Clone(checker_->Allocator(), nullptr));
    return checker_->AllocNode<ir::TSAsExpression>(transformedExpr, clonedType, node->IsConst());
}

// Bridges a non-literal initializer: primitive -> toXxx, reference -> `as T`.
ir::Expression *ExpressionASTTransformer::BridgeValueByElementType(ir::TypeNode *elemType, ir::Expression *value)
{
    if (elemType->IsETSPrimitiveType()) {
        auto methodName = GetPrimitiveCastMethodName(elemType->AsETSPrimitiveType()->GetPrimitiveType());
        ArenaVector<ir::Expression *> castArgs(checker_->Allocator()->Adapter());
        castArgs.push_back(EnsureObject(checker_, value));
        return MakeDebuggerAPIStaticCall(methodName, std::move(castArgs));
    }
    // single-segment names only: the `as T` bridge requires T bindable here
    auto elemName = ExtractTypeRefName(checker_, elemType);
    if (!elemName.Empty() && elemName.Utf8().find('.') == std::string_view::npos && !elemName.Is("Object")) {
        return checker_->AllocNode<ir::TSAsExpression>(value, helpers::CreateETSTypeReference(checker_, elemName),
                                                       false);
    }
    return value;
}

ir::Expression *ExpressionASTTransformer::TransformNewArrayInstance(ir::ETSNewArrayInstanceExpression *node,
                                                                    ArenaVector<ir::Statement *> *stmts)
{
    // the lambda degrade would silently create Array.create(n, null)
    if (node->Initializer() == nullptr || node->Initializer()->IsArrowFunctionExpression()) {
        return ReportUnsupported("Lambda array initializer is not supported in debugger evaluation", checker_);
    }

    // Unbindable element types fail BIND (ESE0371) in the isolated eval module
    // -- erase to Object: identical bytecode either way (element type erased).
    auto *elemTypeRef = node->TypeReference();
    if (!IsEvaluatableElementType(elemTypeRef)) {
        elemTypeRef = helpers::CreateETSTypeReference(checker_, "Object");
    }

    // a NumberLiteral passes through; other forms bridge through toInt
    // (a bare Object operand is ESE0127)
    auto *dim = TransformExpression(node->Dimension(), stmts);
    if (!dim->IsNumberLiteral()) {
        ArenaVector<ir::Expression *> toIntArgs(checker_->Allocator()->Adapter());
        toIntArgs.push_back(EnsureObject(checker_, dim));
        dim = MakeDebuggerAPIStaticCall("toInt", std::move(toIntArgs));
    }

    auto *init = TransformExpression(node->Initializer(), stmts);
    if (!IsLiteralNode(init)) {
        init = BridgeValueByElementType(UnparenthesizeType(elemTypeRef), init);
    }

    // ArrayConversionLowering desugars to Array.create<T>(dim, init) with
    // boot classes only
    auto *result = checker_->AllocNode<ir::ETSNewArrayInstanceExpression>(elemTypeRef, dim, init);
    result->SetRange(node->Range());
    return result;
}

ir::Expression *ExpressionASTTransformer::TransformNewClassInstance(ir::ETSNewClassInstanceExpression *node,
                                                                    ArenaVector<ir::Statement *> *stmts)
{
    auto className = ExtractNewClassTypeName(checker_, node->GetTypeRef());
    if (className.Empty()) {
        return ReportUnsupported("Non-reference class instantiation is not supported in debugger evaluation", checker_);
    }
    ArenaVector<ir::Expression *> newArgs(checker_->Allocator()->Adapter());
    newArgs.push_back(MakeIdentifier("thread"));
    newArgs.push_back(MakeIdentifier("frame"));
    newArgs.push_back(MakeStringLiteral(className));
    for (auto *arg : node->GetArguments()) {
        newArgs.push_back(TransformCallArgument(arg, stmts));
    }
    auto *newCall = MakeDebuggerAPIStaticCall("newInstance", std::move(newArgs));
    return checker_->AllocNode<ir::TSAsExpression>(newCall, helpers::CreateETSTypeReference(checker_, "Object"), false);
}

ir::Expression *ExpressionASTTransformer::TransformLiteral(ir::Expression *node)
{
    if (node->IsStringLiteral()) {
        return checker_->AllocNode<ir::StringLiteral>(node->AsStringLiteral()->Str());
    }
    if (node->IsNumberLiteral()) {
        return checker_->AllocNode<ir::NumberLiteral>(node->AsNumberLiteral()->Number());
    }
    if (node->IsBooleanLiteral()) {
        return checker_->AllocNode<ir::BooleanLiteral>(node->AsBooleanLiteral()->Value());
    }
    if (node->IsNullLiteral()) {
        return checker_->AllocNode<ir::NullLiteral>();
    }
    if (node->IsUndefinedLiteral()) {
        return checker_->AllocNode<ir::UndefinedLiteral>();
    }
    if (node->IsBigIntLiteral()) {
        return checker_->AllocNode<ir::BigIntLiteral>(node->AsBigIntLiteral()->Str());
    }
    if (node->IsCharLiteral()) {
        return checker_->AllocNode<ir::CharLiteral>(node->AsCharLiteral()->Char());
    }
    return node;
}

ir::Expression *ExpressionASTTransformer::TransformIdentifier(ir::Identifier *node)
{
    ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
    args.push_back(MakeIdentifier("thread"));
    args.push_back(MakeIdentifier("frame"));
    args.push_back(MakeStringLiteral(node->Name()));

    return MakeDebuggerAPIStaticCall("get", std::move(args));
}

ir::Expression *ExpressionASTTransformer::TransformNullishCoalescing(ir::BinaryExpression *node,
                                                                     ArenaVector<ir::Statement *> *stmts)
{
    // a ?? b → (let $tmp = get("a"); ($tmp as Any != null) ? $tmp : get("b"))
    auto *leftExpr = TransformExpression(node->Left(), stmts);
    auto *right = TransformExpression(node->Right(), stmts);

    auto tmpName = MakeUniqueName(checker_, "$eval_coalesce");
    auto *tmpId = checker_->AllocNode<ir::Identifier>(tmpName, checker_->Allocator());
    auto *tmpDecl = checker_->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, tmpId, leftExpr);
    ArenaVector<ir::VariableDeclarator *> decls(checker_->Allocator()->Adapter());
    decls.push_back(tmpDecl);
    stmts->push_back(checker_->AllocNode<ir::VariableDeclaration>(ir::VariableDeclaration::VariableDeclarationKind::LET,
                                                                  checker_->Allocator(), std::move(decls)));

    // Create independent Identifier leaves so no AST DAG.
    auto *nullCheckLeft = MakeIdentifier(tmpName);
    auto *consequentLeft = MakeIdentifier(tmpName);
    auto *leftAsAny =
        checker_->AllocNode<ir::TSAsExpression>(nullCheckLeft, helpers::CreateETSTypeReference(checker_, "Any"), false);
    auto *nullCheck = checker_->AllocNode<ir::BinaryExpression>(leftAsAny, checker_->AllocNode<ir::NullLiteral>(),
                                                                lexer::TokenType::PUNCTUATOR_NOT_EQUAL);
    return checker_->AllocNode<ir::ConditionalExpression>(nullCheck, consequentLeft, right);
}

ir::Expression *ExpressionASTTransformer::TransformInstanceof(ir::BinaryExpression *node,
                                                              ArenaVector<ir::Statement *> *stmts)
{
    auto *left = TransformExpression(node->Left(), stmts);
    auto className = ExtractNewClassTypeName(checker_, node->Right());
    if (!className.Empty()) {
        ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
        args.push_back(MakeIdentifier("thread"));
        args.push_back(MakeIdentifier("frame"));
        args.push_back(EnsureObject(checker_, left));
        args.push_back(MakeStringLiteral(className));
        return MakeDebuggerAPIStaticCall("instanceofOp", std::move(args));
    }
    auto *clonedRight = static_cast<ir::Expression *>(node->Right()->Clone(checker_->Allocator(), nullptr));
    return checker_->AllocNode<ir::BinaryExpression>(left, clonedRight, node->OperatorType());
}

// Arithmetic/bitwise/shift via DebuggerAPI; unknown operators stay native.
ir::Expression *ExpressionASTTransformer::TransformArithmeticBinary(lexer::TokenType op, ir::Expression *left,
                                                                    ir::Expression *right)
{
    auto it = kBinaryOpMap.find(op);
    if (it != kBinaryOpMap.end()) {
        ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
        // DebuggerAPI operators take Object: bridge Any-typed operands.
        args.push_back(EnsureObject(checker_, left));
        args.push_back(EnsureObject(checker_, right));
        return MakeDebuggerAPIStaticCall(it->second, std::move(args));
    }
    return checker_->AllocNode<ir::BinaryExpression>(left, right, op);
}

ir::Expression *ExpressionASTTransformer::TransformBinary(ir::BinaryExpression *node,
                                                          ArenaVector<ir::Statement *> *stmts)
{
    auto op = node->OperatorType();
    // `as Any`: Object is non-nullable, so the != null branch would be
    // optimized away otherwise
    if (op == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING) {
        return TransformNullishCoalescing(node, stmts);
    }

    // a simple type reference routes through runtime instanceofOp (user types
    // unresolvable at BIND); the parser produces an ETSTypeReference RHS
    if (op == lexer::TokenType::KEYW_INSTANCEOF) {
        return TransformInstanceof(node, stmts);
    }

    auto *left = TransformExpression(node->Left(), stmts);
    auto *right = TransformExpression(node->Right(), stmts);

    // native operators: equality on Object operands emits dynamic equality
    // (values for boxed primitives, content for strings)
    if (IsLogicalOp(op) || IsEqualityOp(op)) {
        return checker_->AllocNode<ir::BinaryExpression>(left, right, op);
    }
    return TransformArithmeticBinary(op, left, right);
}

ir::Expression *ExpressionASTTransformer::TransformUnary(ir::UnaryExpression *node, ArenaVector<ir::Statement *> *stmts)
{
    auto op = node->OperatorType();
    auto *operand = TransformExpression(node->Argument(), stmts);

    if (op == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK) {
        return checker_->AllocNode<ir::UnaryExpression>(operand, op);
    }

    auto it = kUnaryOpMap.find(op);
    if (it != kUnaryOpMap.end()) {
        ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
        args.push_back(EnsureObject(checker_, operand));
        return MakeDebuggerAPIStaticCall(it->second, std::move(args));
    }

    return checker_->AllocNode<ir::UnaryExpression>(operand, op);
}

ir::Expression *ExpressionASTTransformer::TransformMember(ir::MemberExpression *node,
                                                          ArenaVector<ir::Statement *> *stmts)
{
    if (node->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS || node->Kind() == ir::MemberExpressionKind::NONE) {
        // BuildMemberChain transforms node->Object() itself; pre-transforming
        // here would double-evaluate it
        auto *chain = BuildMemberChain(node, stmts);
        auto *valueCall = WrapInValueCall(chain);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                       false);
    }
    return TransformElementAccess(node, stmts);
}

ir::Expression *ExpressionASTTransformer::TransformElementAccess(ir::MemberExpression *node,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    auto *obj = TransformExpression(node->Object(), stmts);
    auto *arrProxy = obj;

    auto *idxExpr = node->Property();

    // string subscript == field access: getField via ValueProxy reflection
    if (idxExpr->IsStringLiteral()) {
        auto *propStr = TransformExpression(idxExpr, stmts)->AsStringLiteral();
        ArenaVector<ir::Expression *> fargs(checker_->Allocator()->Adapter());
        fargs.push_back(propStr);
        auto *wrapped = WrapInWrap(arrProxy);
        auto *getFieldCall = MakeProxyInstanceCall(wrapped, "getField", std::move(fargs));
        auto *valueCall = WrapInValueCall(getFieldCall);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                       false);
    }

    auto *arrAsObj = MakeArrayOfObjectType(arrProxy);

    ir::Expression *idx = nullptr;
    if (idxExpr->IsNumberLiteral()) {
        idx = TransformExpression(idxExpr, stmts);
    } else {
        // `Object as int` is invalid for the checker: bridge via toInt
        auto *idxRaw = TransformExpression(idxExpr, stmts);
        ArenaVector<ir::Expression *> toIntArgs(checker_->Allocator()->Adapter());
        toIntArgs.push_back(EnsureObject(checker_, idxRaw));
        idx = MakeDebuggerAPIStaticCall("toInt", std::move(toIntArgs));
    }

    ArenaVector<ir::Expression *> getArgs(checker_->Allocator()->Adapter());
    getArgs.push_back(idx);
    return MakeProxyInstanceCall(arrAsObj, "$_get", std::move(getArgs));
}

ir::Expression *ExpressionASTTransformer::BuildMemberChain(ir::MemberExpression *node,
                                                           ArenaVector<ir::Statement *> *stmts)
{
    auto *propStr = MakeStringLiteral(node->Property()->AsIdentifier()->Name());
    ir::Expression *base = nullptr;

    if (node->Object()->IsIdentifier()) {
        auto *obj = TransformExpression(node->Object(), stmts);
        base = WrapInWrap(obj);
    } else if (node->Object()->IsMemberExpression() &&
               (node->Object()->AsMemberExpression()->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS ||
                node->Object()->AsMemberExpression()->Kind() == ir::MemberExpressionKind::NONE)) {
        // recurse directly; no .value() on intermediate results
        base = BuildMemberChain(node->Object()->AsMemberExpression(), stmts);
    } else {
        auto *obj = TransformExpression(node->Object(), stmts);
        base = WrapInWrap(obj);
    }

    ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
    args.push_back(propStr);
    return MakeProxyInstanceCall(base, "getField", std::move(args));
}

// super.foo(): resolved on the superclass of the frame's `this`.
ir::Expression *ExpressionASTTransformer::TransformSuperCall(ir::Expression *methodName, ir::CallExpression *node,
                                                             ArenaVector<ir::Statement *> *stmts)
{
    ArenaVector<ir::Expression *> superArgs(checker_->Allocator()->Adapter());
    superArgs.push_back(MakeIdentifier("thread"));
    superArgs.push_back(MakeIdentifier("frame"));
    superArgs.push_back(methodName);
    for (auto *arg : node->Arguments()) {
        superArgs.push_back(TransformCallArgument(arg, stmts));
    }
    auto *superCall = MakeDebuggerAPIStaticCall("callSuper", std::move(superArgs));
    auto *valueCall = WrapInValueCall(superCall);
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                   false);
}

ir::Expression *ExpressionASTTransformer::TransformMemberCalleeCall(ir::MemberExpression *memberExpr,
                                                                    ir::CallExpression *node,
                                                                    ArenaVector<ir::Statement *> *stmts)
{
    // ELEMENT_ACCESS callee: evaluate the member expression, then invoke it
    if (memberExpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS) {
        auto *elemVal = TransformExpression(memberExpr, stmts);
        auto *wrapped = WrapInWrap(elemVal);
        ArenaVector<ir::Expression *> invokeArgs(checker_->Allocator()->Adapter());
        for (auto *arg : node->Arguments()) {
            invokeArgs.push_back(TransformCallArgument(arg, stmts));
        }
        auto *invokeResult = MakeProxyInstanceCall(wrapped, "invoke", std::move(invokeArgs));
        auto *valueCall = WrapInValueCall(invokeResult);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                       false);
    }

    auto *methodName = MakeStringLiteral(memberExpr->Property()->AsIdentifier()->Name());
    if (memberExpr->Object()->IsSuperExpression()) {
        return TransformSuperCall(methodName, node, stmts);
    }

    auto *obj = TransformExpression(memberExpr->Object(), stmts);
    auto *wrappedObj = WrapInWrap(obj);

    ArenaVector<ir::Expression *> callArgs(checker_->Allocator()->Adapter());
    callArgs.push_back(methodName);

    for (auto *arg : node->Arguments()) {
        callArgs.push_back(TransformCallArgument(arg, stmts));
    }

    auto *callResult = MakeProxyInstanceCall(wrappedObj, "call", std::move(callArgs));
    auto *valueCall = WrapInValueCall(callResult);
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                   false);
}

ir::Expression *ExpressionASTTransformer::TransformCall(ir::CallExpression *node, ArenaVector<ir::Statement *> *stmts)
{
    auto *callee = node->Callee();

    if (IsMemberCallee(callee)) {
        return TransformMemberCalleeCall(callee->AsMemberExpression(), node, stmts);
    }

    // callFunction resolves function-typed variables first, then module-level
    // functions (module class static methods)
    if (callee->IsIdentifier()) {
        ArenaVector<ir::Expression *> fnArgs(checker_->Allocator()->Adapter());
        fnArgs.push_back(MakeIdentifier("thread"));
        fnArgs.push_back(MakeIdentifier("frame"));
        fnArgs.push_back(MakeStringLiteral(callee->AsIdentifier()->Name()));
        for (auto *arg : node->Arguments()) {
            fnArgs.push_back(TransformCallArgument(arg, stmts));
        }
        auto *fnCall = MakeDebuggerAPIStaticCall("callFunction", std::move(fnArgs));
        auto *valueCall = WrapInValueCall(fnCall);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                       false);
    }

    auto *obj = TransformExpression(callee, stmts);
    auto *wrappedObj = WrapInWrap(obj);

    ArenaVector<ir::Expression *> invokeArgs(checker_->Allocator()->Adapter());
    for (auto *arg : node->Arguments()) {
        invokeArgs.push_back(TransformCallArgument(arg, stmts));
    }

    auto *invokeResult = MakeProxyInstanceCall(wrappedObj, "invoke", std::move(invokeArgs));
    auto *valueCall = WrapInValueCall(invokeResult);
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Object"),
                                                   false);
}

ir::Expression *ExpressionASTTransformer::BuildChainMemberAccess(ir::MemberExpression *me, ir::Expression *target,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    if (me->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS || me->Kind() == ir::MemberExpressionKind::NONE) {
        auto *wrapped = WrapInWrap(target);
        auto *propStr = MakeStringLiteral(me->Property()->AsIdentifier()->Name());
        ArenaVector<ir::Expression *> fargs(checker_->Allocator()->Adapter());
        fargs.push_back(propStr);
        auto *getField = MakeProxyInstanceCall(wrapped, "getField", std::move(fargs));
        auto *val = WrapInValueCall(getField);
        return checker_->AllocNode<ir::TSAsExpression>(val, helpers::CreateETSTypeReference(checker_, "Object"), false);
    }
    auto *arrAsObj = MakeArrayOfObjectType(target);
    auto *idx = TransformExpression(me->Property(), stmts);
    if (!me->Property()->IsNumberLiteral()) {
        ArenaVector<ir::Expression *> toIntArgs(checker_->Allocator()->Adapter());
        toIntArgs.push_back(EnsureObject(checker_, idx));
        idx = MakeDebuggerAPIStaticCall("toInt", std::move(toIntArgs));
    }
    ArenaVector<ir::Expression *> getArgs(checker_->Allocator()->Adapter());
    getArgs.push_back(idx);
    return MakeProxyInstanceCall(arrAsObj, "$_get", std::move(getArgs));
}

ir::Expression *ExpressionASTTransformer::BuildChainInvokeValue(ir::Expression *receiver,
                                                                ArenaVector<ir::Expression *> &&args)
{
    auto *wrapped = WrapInWrap(receiver);
    auto *invokeResult = MakeProxyInstanceCall(wrapped, "invoke", std::move(args));
    auto *accessValue = WrapInValueCall(invokeResult);
    return checker_->AllocNode<ir::TSAsExpression>(accessValue, helpers::CreateETSTypeReference(checker_, "Object"),
                                                   false);
}

// Declares `let $t = value` and returns the null-check expression
// `($t as Any) != null`; *guardedIdent receives an INDEPENDENT identifier of
// $t for the guarded (consequent) expression, so no AST node is shared.
ir::Expression *ExpressionASTTransformer::DeclareNullCheckedTemp(ir::Expression *value, util::StringView prefix,
                                                                 ir::Identifier **guardedIdent,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    auto tmpName = MakeUniqueName(checker_, prefix);
    auto *tmpId = checker_->AllocNode<ir::Identifier>(tmpName, checker_->Allocator());
    auto *tmpDecl = checker_->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, tmpId, value);
    ArenaVector<ir::VariableDeclarator *> decls(checker_->Allocator()->Adapter());
    decls.push_back(tmpDecl);
    stmts->push_back(checker_->AllocNode<ir::VariableDeclaration>(ir::VariableDeclaration::VariableDeclarationKind::LET,
                                                                  checker_->Allocator(), std::move(decls)));

    auto *checkId = MakeIdentifier(tmpName);
    *guardedIdent = MakeIdentifier(tmpName);
    auto *asAny =
        checker_->AllocNode<ir::TSAsExpression>(checkId, helpers::CreateETSTypeReference(checker_, "Any"), false);
    return checker_->AllocNode<ir::BinaryExpression>(asAny, checker_->AllocNode<ir::NullLiteral>(),
                                                     lexer::TokenType::PUNCTUATOR_NOT_EQUAL);
}

ir::Expression *ExpressionASTTransformer::TransformOptionalMember(ir::MemberExpression *me,
                                                                  ArenaVector<ir::Statement *> *stmts)
{
    // recurse the base so nested optional flags survive
    auto *base = TransformOptionalChain(me->Object(), stmts);

    if (!me->IsOptional()) {
        // nest inside the base conditional's consequent to short-circuit
        if (base->IsConditionalExpression()) {
            auto *cond = base->AsConditionalExpression();
            auto *accessValue = BuildChainMemberAccess(me, cond->Consequent(), stmts);
            return checker_->AllocNode<ir::ConditionalExpression>(cond->Test(), accessValue, cond->Alternate());
        }
        return BuildChainMemberAccess(me, base, stmts);
    }

    // save base to temp + null check + conditional
    ir::Identifier *tempRef = nullptr;
    auto *nullCheck = DeclareNullCheckedTemp(base, "$eval_opt", &tempRef, stmts);
    auto *accessValue = BuildChainMemberAccess(me, tempRef, stmts);
    return checker_->AllocNode<ir::ConditionalExpression>(nullCheck, accessValue,
                                                          checker_->AllocNode<ir::UndefinedLiteral>());
}

ir::Expression *ExpressionASTTransformer::TransformOptionalCall(ir::CallExpression *call,
                                                                ArenaVector<ir::Statement *> *stmts)
{
    auto *callee = TransformOptionalChain(call->Callee(), stmts);

    auto buildArgs = [&]() {
        ArenaVector<ir::Expression *> invokeArgs(checker_->Allocator()->Adapter());
        for (auto *arg : call->Arguments()) {
            invokeArgs.push_back(TransformCallArgument(arg, stmts));
        }
        return invokeArgs;
    };

    if (!call->IsOptional()) {
        // nest inside the base conditional's consequent so the outer
        // short-circuit extends through the call
        if (callee->IsConditionalExpression()) {
            auto *cond = callee->AsConditionalExpression();
            auto *accessValue = BuildChainInvokeValue(cond->Consequent(), buildArgs());
            return checker_->AllocNode<ir::ConditionalExpression>(cond->Test(), accessValue, cond->Alternate());
        }
        return BuildChainInvokeValue(callee, buildArgs());
    }

    ir::Identifier *tempRef = nullptr;
    auto *nullCheck = DeclareNullCheckedTemp(callee, "$eval_opt", &tempRef, stmts);
    auto *accessValue = BuildChainInvokeValue(tempRef, buildArgs());
    return checker_->AllocNode<ir::ConditionalExpression>(nullCheck, accessValue,
                                                          checker_->AllocNode<ir::UndefinedLiteral>());
}

ir::Expression *ExpressionASTTransformer::TransformOptionalChain(ir::Expression *node,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    if (node->IsMemberExpression()) {
        return TransformOptionalMember(node->AsMemberExpression(), stmts);
    }

    if (node->IsCallExpression()) {
        return TransformOptionalCall(node->AsCallExpression(), stmts);
    }

    return TransformExpression(node, stmts);
}

ir::Expression *ExpressionASTTransformer::TransformCallArgument(ir::Expression *arg,
                                                                ArenaVector<ir::Statement *> *stmts)
{
    if (arg->IsSpreadElement()) {
        // restArgsLowering expands the spread at the rest-parameter call site;
        // cast to Array<Object> satisfies the checker's iterable requirement
        auto *transformed = TransformExpression(arg->AsSpreadElement()->Argument(), stmts);
        auto *asArray = MakeArrayOfObjectType(transformed);
        return checker_->AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, checker_->Allocator(), asArray);
    }
    return TransformExpression(arg, stmts);
}

// One lifted branch block. The `as Any` keeps the temp's smart-cast state at
// Any across the join; a narrowed join would throw on null/undefined values.
ir::BlockStatement *ExpressionASTTransformer::BuildConditionalBranch(ArenaVector<ir::Statement *> &&branchStmts,
                                                                     ir::Expression *branchValue,
                                                                     ir::Expression *target)
{
    auto *asAny =
        checker_->AllocNode<ir::TSAsExpression>(branchValue, helpers::CreateETSTypeReference(checker_, "Any"), false);
    auto *assign =
        checker_->AllocNode<ir::AssignmentExpression>(target, asAny, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    branchStmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(assign));
    return checker_->AllocNode<ir::BlockStatement>(checker_->Allocator(), std::move(branchStmts));
}

ir::Expression *ExpressionASTTransformer::LiftConditionalBranches(ir::Expression *test, ir::Expression *consequent,
                                                                  ArenaVector<ir::Statement *> &&consStmts,
                                                                  ir::Expression *alternate,
                                                                  ArenaVector<ir::Statement *> &&altStmts,
                                                                  ArenaVector<ir::Statement *> *stmts)
{
    // Lift: let $t = undefined; if (test) {...; $t = c as Any} else {...};
    // yield $t. The if condition shares the native conditional's truthiness
    // check, so the test is evaluated the same way.
    auto resultName = MakeUniqueName(checker_, "$eval_cond");
    auto *declId = MakeIdentifier(resultName);
    auto *consId = MakeIdentifier(resultName);
    auto *altId = MakeIdentifier(resultName);
    auto *refId = MakeIdentifier(resultName);

    declId->SetTypeAnnotation(helpers::CreateETSTypeReference(checker_, "Any"));
    auto *decl = checker_->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, declId,
                                                             checker_->AllocNode<ir::UndefinedLiteral>());
    ArenaVector<ir::VariableDeclarator *> decls(checker_->Allocator()->Adapter());
    decls.push_back(decl);
    stmts->push_back(checker_->AllocNode<ir::VariableDeclaration>(ir::VariableDeclaration::VariableDeclarationKind::LET,
                                                                  checker_->Allocator(), std::move(decls)));

    auto *consBlock = BuildConditionalBranch(std::move(consStmts), consequent, consId);
    auto *altBlock = BuildConditionalBranch(std::move(altStmts), alternate, altId);
    stmts->push_back(checker_->AllocNode<ir::IfStatement>(test, consBlock, altBlock));
    return refId;
}

ir::Expression *ExpressionASTTransformer::TransformConditional(ir::ConditionalExpression *node,
                                                               ArenaVector<ir::Statement *> *stmts)
{
    auto *test = TransformExpression(node->Test(), stmts);

    // separate statement vectors per branch: writebacks must execute only in
    // the taken branch
    ArenaVector<ir::Statement *> consStmts(checker_->Allocator()->Adapter());
    auto *consequent = TransformExpression(node->Consequent(), &consStmts);
    ArenaVector<ir::Statement *> altStmts(checker_->Allocator()->Adapter());
    auto *alternate = TransformExpression(node->Alternate(), &altStmts);

    // fast path: no settling
    if (consStmts.empty() && altStmts.empty()) {
        return checker_->AllocNode<ir::ConditionalExpression>(test, consequent, alternate);
    }

    return LiftConditionalBranches(test, consequent, std::move(consStmts), alternate, std::move(altStmts), stmts);
}

ir::Statement *ExpressionASTTransformer::TransformAssignment(ir::AssignmentExpression *node)
{
    auto *left = node->Left();
    auto op = node->OperatorType();

    ArenaVector<ir::Statement *> stmts(checker_->Allocator()->Adapter());

    ir::Expression *lhsProxy = nullptr;
    ir::Expression *rawForArith = nullptr;  // Object for compound arithmetic

    if (left->IsIdentifier()) {
        // Identifier: BuildAssignWriteback emits the DebuggerAPI.set() writeback.
        rawForArith = TransformExpression(left, &stmts);  // copy for compound arithmetic
    } else if (left->IsMemberExpression()) {
        auto *me = left->AsMemberExpression();
        if (me->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS || me->Kind() == ir::MemberExpressionKind::NONE) {
            lhsProxy = BuildMemberLHS(me, &stmts);
            // clone for the compound read; re-transforming would double-evaluate
            auto *readProxy = static_cast<ir::Expression *>(lhsProxy->Clone(checker_->Allocator(), nullptr));
            rawForArith = WrapInValueCall(readProxy);
        } else {
            return HandleSubscriptAssignment(me, node, op, std::move(stmts));
        }
    } else {
        LOG(ERROR, ES2PANDA) << "Debugger evaluation: Unsupported assignment target";
        stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(checker_->AllocNode<ir::NullLiteral>()));
        return BuildAssignReturnBlock(std::move(stmts));
    }

    return BuildAssignWriteback(lhsProxy, rawForArith, left, node, op, std::move(stmts));
}

ir::Statement *ExpressionASTTransformer::HandleSubscriptAssignment(ir::MemberExpression *me,
                                                                   ir::AssignmentExpression *node, lexer::TokenType op,
                                                                   ArenaVector<ir::Statement *> &&stmts)
{
    auto *arrExpr = me->Object();
    auto *idxExpr = me->Property();

    // string subscript: getField("x").setValue(wrap(val))
    if (idxExpr->IsStringLiteral()) {
        return HandleSubscriptStringAssignment(me, node, arrExpr, std::move(stmts));
    }

    return EmitSubscriptWriteback(arrExpr, idxExpr, node, op, std::move(stmts));
}

// Compound-assignment RHS via the DebuggerAPI arithmetic dispatch.
ir::Expression *ExpressionASTTransformer::BuildCompoundRhs(ir::AssignmentExpression *node, ir::Expression *curValue,
                                                           ArenaVector<ir::Statement *> *stmts)
{
    auto opMethod = GetCompoundOpMethod(node);
    if (opMethod.empty()) {
        ES2PANDA_UNREACHABLE();
    }
    auto *rightRaw = TransformExpression(node->Right(), stmts);
    ArenaVector<ir::Expression *> opArgs(checker_->Allocator()->Adapter());
    opArgs.push_back(curValue);
    opArgs.push_back(rightRaw);
    return MakeDebuggerAPIStaticCall(opMethod, std::move(opArgs));
}

// Declares `let $prefix_N = rhsValue`; returns independent Identifier nodes
// for the declaration / write / return positions (no AST DAG).
EvalTempIdents ExpressionASTTransformer::DeclareEvalTemp(util::StringView prefix, ir::Expression *rhsValue,
                                                         ArenaVector<ir::Statement *> *stmts)
{
    auto resultName = MakeUniqueName(checker_, prefix);
    EvalTempIdents idents {MakeIdentifier(resultName), MakeIdentifier(resultName), MakeIdentifier(resultName)};
    auto *resultDecl =
        checker_->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, idents.decl, rhsValue);
    ArenaVector<ir::VariableDeclarator *> decls(checker_->Allocator()->Adapter());
    decls.push_back(resultDecl);
    stmts->push_back(checker_->AllocNode<ir::VariableDeclaration>(ir::VariableDeclaration::VariableDeclarationKind::LET,
                                                                  checker_->Allocator(), std::move(decls)));
    return idents;
}

ir::Statement *ExpressionASTTransformer::HandleSubscriptStringAssignment(ir::MemberExpression *me,
                                                                         ir::AssignmentExpression *node,
                                                                         ir::Expression *arrExpr,
                                                                         ArenaVector<ir::Statement *> &&stmts)
{
    auto *arrProxy = TransformExpression(arrExpr, &stmts);
    auto *propStr = TransformExpression(me->Property(), &stmts)->AsStringLiteral();
    ArenaVector<ir::Expression *> fargs(checker_->Allocator()->Adapter());
    fargs.push_back(propStr);
    auto *wrapped = WrapInWrap(arrProxy);
    auto *getFieldCall = MakeProxyInstanceCall(wrapped, "getField", std::move(fargs));

    ir::Expression *rhsValue = nullptr;
    auto op = node->OperatorType();
    if (op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        rhsValue = TransformExpression(node->Right(), &stmts);
    } else {
        // clone arrProxy; re-transforming would double-evaluate
        auto *arrProxy2 = static_cast<ir::Expression *>(arrProxy->Clone(checker_->Allocator(), nullptr));
        auto *propStr2 = checker_->AllocNode<ir::StringLiteral>(propStr->Str());
        auto *wrapped2 = WrapInWrap(arrProxy2);
        ArenaVector<ir::Expression *> readArgs(checker_->Allocator()->Adapter());
        readArgs.push_back(propStr2);
        auto *readFieldCall = MakeProxyInstanceCall(wrapped2, "getField", std::move(readArgs));
        auto *curVal = WrapInValueCall(readFieldCall);
        curVal =
            checker_->AllocNode<ir::TSAsExpression>(curVal, helpers::CreateETSTypeReference(checker_, "Object"), false);
        rhsValue = BuildCompoundRhs(node, curVal, &stmts);
    }

    auto idents = DeclareEvalTemp("$eval_result", rhsValue, &stmts);
    auto *wrappedRhs = WrapInWrap(idents.set);
    ArenaVector<ir::Expression *> svArgs(checker_->Allocator()->Adapter());
    svArgs.push_back(wrappedRhs);
    auto *setCall = MakeProxyInstanceCall(getFieldCall, "setValue", std::move(svArgs));
    stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(idents.ret));
    return BuildAssignReturnBlock(std::move(stmts));
}

ir::Expression *ExpressionASTTransformer::BuildSubscriptIndex(ir::Expression *idxExpr,
                                                              ArenaVector<ir::Statement *> *stmts)
{
    if (idxExpr->IsNumberLiteral()) {
        return checker_->AllocNode<ir::NumberLiteral>(idxExpr->AsNumberLiteral()->Number());
    }
    auto *idxRaw = TransformExpression(idxExpr, stmts);
    ArenaVector<ir::Expression *> toIntArgs(checker_->Allocator()->Adapter());
    toIntArgs.push_back(EnsureObject(checker_, idxRaw));
    return MakeDebuggerAPIStaticCall("toInt", std::move(toIntArgs));
}

// New element value: plain `=` transforms Right; compound ops read via $_get
// (*idxForGet feeds the write path's clone).
ir::Expression *ExpressionASTTransformer::BuildSubscriptNewValue(ir::Expression *arrProxy, ir::Expression *idxExpr,
                                                                 ir::AssignmentExpression *node, lexer::TokenType op,
                                                                 ir::Expression **idxForGet,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    if (op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        return TransformExpression(node->Right(), stmts);
    }
    auto *arrAsObj1 = MakeArrayOfObjectType(arrProxy);
    *idxForGet = BuildSubscriptIndex(idxExpr, stmts);
    ArenaVector<ir::Expression *> getArgs(checker_->Allocator()->Adapter());
    getArgs.push_back(*idxForGet);
    auto *curVal = MakeProxyInstanceCall(arrAsObj1, "$_get", std::move(getArgs));
    return BuildCompoundRhs(node, curVal, stmts);
}

// Element write: identifier receivers route through the type-aware
// DebuggerAPI.setElement; others keep $_set.
ir::Expression *ExpressionASTTransformer::BuildSubscriptSetCall(ir::Expression *arrExpr, ir::Expression *arrProxy,
                                                                ir::Expression *idxForGet, ir::Expression *idxExpr,
                                                                ir::Identifier *valueIdent,
                                                                ArenaVector<ir::Statement *> *stmts)
{
    if (arrExpr->IsIdentifier()) {
        ArenaVector<ir::Expression *> setElemArgs(checker_->Allocator()->Adapter());
        setElemArgs.push_back(MakeIdentifier("thread"));
        setElemArgs.push_back(MakeIdentifier("frame"));
        setElemArgs.push_back(MakeStringLiteral(arrExpr->AsIdentifier()->Name()));
        setElemArgs.push_back(idxForGet != nullptr
                                  ? static_cast<ir::Expression *>(idxForGet->Clone(checker_->Allocator(), nullptr))
                                  : BuildSubscriptIndex(idxExpr, stmts));
        setElemArgs.push_back(EnsureObject(checker_, valueIdent));
        return MakeDebuggerAPIStaticCall("setElement", std::move(setElemArgs));
    }
    auto *arrForSet = idxForGet != nullptr
                          ? static_cast<ir::Expression *>(arrProxy->Clone(checker_->Allocator(), nullptr))
                          : arrProxy;
    auto *arrAsObj2 = MakeArrayOfObjectType(arrForSet);
    auto *idxForSet = idxForGet != nullptr
                          ? static_cast<ir::Expression *>(idxForGet->Clone(checker_->Allocator(), nullptr))
                          : BuildSubscriptIndex(idxExpr, stmts);
    ArenaVector<ir::Expression *> setArgs(checker_->Allocator()->Adapter());
    setArgs.push_back(idxForSet);
    setArgs.push_back(EnsureObject(checker_, valueIdent));
    return MakeProxyInstanceCall(arrAsObj2, "$_set", std::move(setArgs));
}

ir::Statement *ExpressionASTTransformer::EmitSubscriptWriteback(ir::Expression *arrExpr, ir::Expression *idxExpr,
                                                                ir::AssignmentExpression *node, lexer::TokenType op,
                                                                ArenaVector<ir::Statement *> &&stmts)
{
    // transform once; $_get/$_set get independent clones
    auto *arrProxy = TransformExpression(arrExpr, &stmts);

    ir::Expression *idxForGet = nullptr;
    auto *rhsValue = BuildSubscriptNewValue(arrProxy, idxExpr, node, op, &idxForGet, &stmts);

    auto idents = DeclareEvalTemp("$eval_result", rhsValue, &stmts);

    auto *setCall = BuildSubscriptSetCall(arrExpr, arrProxy, idxForGet, idxExpr, idents.set, &stmts);
    stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(idents.ret));
    return BuildAssignReturnBlock(std::move(stmts));
}

ir::Statement *ExpressionASTTransformer::BuildAssignWriteback(ir::Expression *lhsProxy, ir::Expression *rawForArith,
                                                              ir::Expression *left, ir::AssignmentExpression *node,
                                                              lexer::TokenType op, ArenaVector<ir::Statement *> &&stmts)
{
    ir::Expression *rhsValue = nullptr;
    if (op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        rhsValue = TransformExpression(node->Right(), &stmts);
    } else {
        // WrapInValueCall returns Any; DebuggerAPI ops take Object
        if (left->IsMemberExpression() || left->IsIdentifier()) {
            rawForArith = checker_->AllocNode<ir::TSAsExpression>(
                rawForArith, helpers::CreateETSTypeReference(checker_, "Object"), false);
        }
        rhsValue = BuildCompoundRhs(node, rawForArith, &stmts);
    }

    auto idents = DeclareEvalTemp("$eval_result", rhsValue, &stmts);

    if (left->IsIdentifier()) {
        ArenaVector<ir::Expression *> setIdentArgs(checker_->Allocator()->Adapter());
        setIdentArgs.push_back(MakeIdentifier("thread"));
        setIdentArgs.push_back(MakeIdentifier("frame"));
        setIdentArgs.push_back(MakeStringLiteral(left->AsIdentifier()->Name()));
        setIdentArgs.push_back(EnsureObject(checker_, idents.set));
        auto *setCall = MakeDebuggerAPIStaticCall("set", std::move(setIdentArgs));
        stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));
    } else {
        auto *wrappedRhs = WrapInWrap(idents.set);
        ArenaVector<ir::Expression *> setArgs(checker_->Allocator()->Adapter());
        setArgs.push_back(wrappedRhs);
        auto *setCall = MakeProxyInstanceCall(lhsProxy, "setValue", std::move(setArgs));
        stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));
    }
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(idents.ret));

    return BuildAssignReturnBlock(std::move(stmts));
}

std::string_view ExpressionASTTransformer::GetCompoundOpMethod(ir::AssignmentExpression *node)
{
    auto op = node->OperatorType();
    if (op == lexer::TokenType::PUNCTUATOR_PLUS_EQUAL)
        return "add";
    if (op == lexer::TokenType::PUNCTUATOR_MINUS_EQUAL)
        return "sub";
    if (op == lexer::TokenType::PUNCTUATOR_MULTIPLY_EQUAL)
        return "mul";
    if (op == lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL)
        return "div";
    if (op == lexer::TokenType::PUNCTUATOR_MOD_EQUAL)
        return "mod";
    if (op == lexer::TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL)
        return "pow";
    if (op == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT_EQUAL)
        return "shl";
    if (op == lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL)
        return "shr";
    if (op == lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL)
        return "ushr";
    if (op == lexer::TokenType::PUNCTUATOR_BITWISE_AND_EQUAL)
        return "bitAnd";
    if (op == lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL)
        return "bitOr";
    if (op == lexer::TokenType::PUNCTUATOR_BITWISE_XOR_EQUAL)
        return "bitXor";
    return "";
}

ir::BlockStatement *ExpressionASTTransformer::BuildAssignReturnBlock(ArenaVector<ir::Statement *> &&stmts)
{
    return checker_->AllocNode<ir::BlockStatement>(checker_->Allocator(), std::move(stmts));
}

ir::Expression *ExpressionASTTransformer::BuildIdentifierLHS(ir::Identifier *ident)
{
    ArenaVector<ir::Expression *> getArgs(checker_->Allocator()->Adapter());
    getArgs.push_back(MakeIdentifier("thread"));
    getArgs.push_back(MakeIdentifier("frame"));
    getArgs.push_back(MakeStringLiteral(ident->Name()));

    auto *getCall = MakeDebuggerAPIStaticCall("get", std::move(getArgs));
    return WrapInWrap(getCall);
}

ir::Expression *ExpressionASTTransformer::BuildMemberLHS(ir::MemberExpression *memberExpr,
                                                         ArenaVector<ir::Statement *> *stmts)
{
    ES2PANDA_ASSERT(memberExpr->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS ||
                    memberExpr->Kind() == ir::MemberExpressionKind::NONE);

    auto *obj = memberExpr->Object();

    ir::Expression *base = nullptr;
    if (obj->IsIdentifier()) {
        base = BuildIdentifierLHS(obj->AsIdentifier());
    } else if (obj->IsMemberExpression() &&
               (obj->AsMemberExpression()->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS ||
                obj->AsMemberExpression()->Kind() == ir::MemberExpressionKind::NONE)) {
        base = BuildMemberLHS(obj->AsMemberExpression(), stmts);
    } else {
        // thread through stmts so settling statements (?? temps, lifted
        // conditionals) stay inside this assignment's sequence
        auto *trans = TransformExpression(obj, stmts);
        base = WrapInWrap(trans);
    }

    auto *propStr = MakeStringLiteral(memberExpr->Property()->AsIdentifier()->Name());
    ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
    args.push_back(propStr);
    return MakeProxyInstanceCall(base, "getField", std::move(args));
}

ir::CallExpression *ExpressionASTTransformer::MakeDebuggerAPIStaticCall(util::StringView methodName,
                                                                        ArenaVector<ir::Expression *> &&args)
{
    auto *callee = MakeStaticMemberAccess("DebuggerAPI", methodName);
    return checker_->AllocNode<ir::CallExpression>(callee, std::move(args), nullptr, false);
}

ir::CallExpression *ExpressionASTTransformer::MakeProxyInstanceCall(ir::Expression *receiver,
                                                                    util::StringView methodName,
                                                                    ArenaVector<ir::Expression *> &&args)
{
    auto *prop = MakeIdentifier(methodName);
    auto *callee = checker_->AllocNode<ir::MemberExpression>(receiver, prop, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                             false, false);
    return checker_->AllocNode<ir::CallExpression>(callee, std::move(args), nullptr, false);
}

ir::MemberExpression *ExpressionASTTransformer::MakeStaticMemberAccess(util::StringView className,
                                                                       util::StringView methodName)
{
    auto *classIdent = MakeIdentifier(className);
    auto *methodIdent = MakeIdentifier(methodName);
    return checker_->AllocNode<ir::MemberExpression>(classIdent, methodIdent, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                     false, false);
}

ir::Identifier *ExpressionASTTransformer::MakeIdentifier(util::StringView name)
{
    return checker_->AllocNode<ir::Identifier>(name, checker_->Allocator());
}

ir::StringLiteral *ExpressionASTTransformer::MakeStringLiteral(util::StringView value)
{
    return checker_->AllocNode<ir::StringLiteral>(value);
}

ir::TSAsExpression *ExpressionASTTransformer::MakeArrayOfObjectType(ir::Expression *arrProxy)
{
    auto *arrIdent = checker_->AllocNode<ir::Identifier>("Array", checker_->Allocator());
    auto *objectIdent = checker_->AllocNode<ir::Identifier>("Object", checker_->Allocator());
    auto *objTypePart = checker_->AllocNode<ir::ETSTypeReferencePart>(objectIdent, checker_->Allocator());
    ArenaVector<ir::TypeNode *> typeParamVec(1, objTypePart, checker_->Allocator()->Adapter());
    auto *typeParams = checker_->AllocNode<ir::TSTypeParameterInstantiation>(std::move(typeParamVec));
    auto *arrTypePart =
        checker_->AllocNode<ir::ETSTypeReferencePart>(arrIdent, typeParams, nullptr, checker_->Allocator());
    auto *arrType = checker_->AllocNode<ir::ETSTypeReference>(arrTypePart, checker_->Allocator());
    return checker_->AllocNode<ir::TSAsExpression>(arrProxy, arrType, false);
}

ir::Expression *ExpressionASTTransformer::WrapInValueCall(ir::Expression *expr)
{
    auto *prop = MakeIdentifier("value");
    auto *callee =
        checker_->AllocNode<ir::MemberExpression>(expr, prop, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    ArenaVector<ir::Expression *> empty(checker_->Allocator()->Adapter());
    return checker_->AllocNode<ir::CallExpression>(callee, std::move(empty), nullptr, false);
}

ir::Expression *ExpressionASTTransformer::WrapInWrap(ir::Expression *expr)
{
    ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
    args.push_back(EnsureObject(checker_, expr));
    return MakeDebuggerAPIStaticCall("wrap", std::move(args));
}

}  // namespace ark::es2panda::evaluate
