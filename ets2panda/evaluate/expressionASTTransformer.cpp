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
#include "ir/ets/etsDestructuring.h"
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

// Relational operators of the DebuggerAPI dispatch set: char comparisons are
// spec-legal (17_experimental "char Operations") and must stay on the
// dispatch; only arithmetic/bitwise/shift forms divert to the native checker.
static bool IsRelationalOp(lexer::TokenType op)
{
    return op == lexer::TokenType::PUNCTUATOR_LESS_THAN || op == lexer::TokenType::PUNCTUATOR_GREATER_THAN ||
           op == lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL || op == lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL;
}

// The `2n ** -1n` exponent shape: unary minus over a BigInt literal.
static bool IsNegatedBigIntLiteral(ir::Expression *expr)
{
    return expr->IsUnaryExpression() &&
           expr->AsUnaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_MINUS &&
           expr->AsUnaryExpression()->Argument()->IsBigIntLiteral();
}

// Whether a type expression carries explicit type arguments
// (`EvalBox<number>`); instantiated generics are rejected on the instanceof
// RHS by the mainline checker (INSTANCEOF_ERASED, ESY18871).
static bool HasTypeArguments(ir::Expression *typeExpr)
{
    if (typeExpr->IsETSTypeReference()) {
        auto *part = typeExpr->AsETSTypeReference()->Part();
        return part != nullptr && part->TypeParams() != nullptr;
    }
    if (typeExpr->IsTSTypeReference()) {
        return typeExpr->AsTSTypeReference()->TypeParams() != nullptr;
    }
    return false;
}

// Whether a new-class type reference or cast annotation is the builtin
// fixed/value array type syntax (`FixedArray<T>` / `ValueArray<T>`,
// single-segment): predefined type-level names (reserved, ESY0242) that the
// checker resolves to ETSArrayType via CheckPredefinedBuiltinTypes without
// any class resolution. Their constructors desugar through
// FixedArrayLowering (newarr intrinsic) and their casts compile to
// array-descriptor checkcasts -- no class with this name exists at runtime,
// so the newInstance/castAs string dispatch always fails ("Class not found").
// The bare-identifier form (no type arguments) matches too: the native node
// reaches the standard checker and is rejected like the main compiler
// (FIXED_ARRAY_PARAM_ERROR).
static bool IsBuiltinArrayTypeName(ir::Expression *typeExpr)
{
    if (typeExpr->IsIdentifier()) {
        auto name = typeExpr->AsIdentifier()->Name();
        return name.Is("FixedArray") || name.Is("ValueArray");
    }
    if (typeExpr->IsETSTypeReference()) {
        auto *part = typeExpr->AsETSTypeReference()->Part();
        if (part == nullptr || part->Previous() != nullptr || part->Name() == nullptr ||
            !part->Name()->IsIdentifier()) {
            return false;  // qualified / folded / broken forms are not the builtin syntax
        }
        auto name = part->Name()->AsIdentifier()->Name();
        return name.Is("FixedArray") || name.Is("ValueArray");
    }
    return false;
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

// Rejection message for unsupported destructuring element forms, aligned with
// the normal pipeline's diagnostics (REST_UNSUPPORTED_IN_DESTRUCTURING /
// DEFAULT_UNSUPPORTED_IN_DESTRUCTURING / NOT_IMPLEMENTED #275).
static util::StringView DestructuringElementRejectMessage(ir::Expression *elem)
{
    if (elem->IsRestElement()) {
        return "Rest element is not supported in destructuring assignment in debugger evaluation";
    }
    if (elem->IsAssignmentPattern()) {
        return "Default value is not supported in destructuring assignment in debugger evaluation";
    }
    if (elem->IsArrayPattern()) {
        return "Nested destructuring is not supported in debugger evaluation";
    }
    return "Unsupported destructuring element in debugger evaluation";
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

// Whether an `as` wrapper's annotation is the erased-value form: the
// single-segment Object or Any reference (the member / call / super /
// string-subscript / optional-chain / new-class-instance transform outputs).
// Object marks pre-nullability-fix wrappers; Any marks the nullable result
// wrappers (runtime may deliver null), which the spread bridge must treat
// identically.
bool IsErasedValueAnnotation(ir::Expression *arg)
{
    auto *ann = arg->AsTSAsExpression()->TypeAnnotation();
    if (ann == nullptr || !ann->IsETSTypeReference()) {
        return false;
    }
    auto *part = ann->AsETSTypeReference()->Part();
    return part != nullptr && part->Previous() == nullptr && part->Name() != nullptr && part->Name()->IsIdentifier() &&
           (part->Name()->AsIdentifier()->Name().Is("Object") || part->Name()->AsIdentifier()->Name().Is("Any"));
}

// Whether a transformed spread argument needs bridging to `as Array<Object>`
// (ESE0049: Object/Any are not iterable). Matches the erased-value outputs:
// bare get/getThis, castAs(..., "Array"), proxy $_get, `as Object`/`as Any`
// result wrappers.
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
    return arg->IsTSAsExpression() && IsErasedValueAnnotation(arg);
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

// Source-level name of a primitive type argument. Switch (not a positional
// array): immune to PrimitiveType enum reordering, same output strings.
static util::StringView GetPrimitiveTypeArgName(ir::PrimitiveType type)
{
    switch (type) {
        case ir::PrimitiveType::BYTE:
            return "byte";
        case ir::PrimitiveType::INT:
            return "int";
        case ir::PrimitiveType::LONG:
            return "long";
        case ir::PrimitiveType::SHORT:
            return "short";
        case ir::PrimitiveType::FLOAT:
            return "float";
        case ir::PrimitiveType::DOUBLE:
            return "double";
        case ir::PrimitiveType::BOOLEAN:
            return "boolean";
        case ir::PrimitiveType::CHAR:
            return "char";
        case ir::PrimitiveType::VOID:
            return "void";
    }
    return util::StringView();
}

// Formats the explicit generic instantiation of a call for the runtime:
// single-segment reference names and primitive names pass through, other
// forms degrade to "?" (the runtime treats it as unknown), and no type
// arguments yield the empty string. Comma-separated when multiple. This is
// the DebuggerAPI.callFunction typeArgs wire format.
static util::StringView FormatCallTypeArgs(checker::ETSChecker *checker, ir::TSTypeParameterInstantiation *typeParams)
{
    if (typeParams == nullptr || typeParams->Params().empty()) {
        // Valid empty view (not the default-constructed one): StringLiteral
        // emission needs a non-null data pointer.
        return util::StringView("");
    }
    std::string formatted;
    for (auto *tp : typeParams->Params()) {
        if (!formatted.empty()) {
            formatted += ",";
        }
        if (tp->IsETSTypeReference()) {
            auto *part = tp->AsETSTypeReference()->Part();
            if (part != nullptr && part->Name() != nullptr && part->Name()->IsIdentifier()) {
                formatted += std::string(part->Name()->AsIdentifier()->Name().Utf8());
                continue;
            }
        } else if (tp->IsETSPrimitiveType()) {
            formatted += std::string(GetPrimitiveTypeArgName(tp->AsETSPrimitiveType()->GetPrimitiveType()).Utf8());
            continue;
        }
        formatted += "?";
    }

    auto *buf = static_cast<char *>(checker->Allocator()->Alloc(formatted.size() + 1));
    [[maybe_unused]] auto err = memcpy_s(buf, formatted.size() + 1, formatted.c_str(), formatted.size() + 1);
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

// let/const declaration: per declarator (source order),
//   let $eval_decl_N = <initializer>
//     — evaluated once, shared by the registration and the return value
//   → DebuggerAPI.defineVariable(thread, frame, "a", $eval_decl_N, readonly)
//   → ...
//   → yields the last declarator's temp
// Only Identifier targets with initializers and LET/CONST kinds are
// accepted; other forms degrade to a null-returning body. The name only
// ever appears as a string literal, never as an eval-function local, so
// declarations like `let thread = 1` cannot collide with the wrapper
// parameters. The value argument passes the temp identifier raw
// (no EnsureObject): the runtime parameter is Any-typed and must accept a
// null initializer. A later declarator's initializer references an earlier
// declarator through the pendingDecls_ temp binding, preserving the
// statement's lexical semantics: a get()-based lookup would resolve a
// same-named frame variable first, which shadows the session variable in
// the runtime name resolution.
ir::Statement *ExpressionASTTransformer::TransformDeclaration(ir::VariableDeclaration *node)
{
    ArenaVector<ir::Statement *> stmts(checker_->Allocator()->Adapter());

    auto reject = [&stmts, this](util::StringView message) {
        stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(ReportUnsupported(message, checker_)));
        return BuildAssignReturnBlock(std::move(stmts));
    };

    auto kind = node->Kind();
    if (kind != ir::VariableDeclaration::VariableDeclarationKind::LET &&
        kind != ir::VariableDeclaration::VariableDeclarationKind::CONST) {
        return reject("Only 'let' and 'const' declarations are supported in debugger evaluation");
    }

    // Pre-validate every declarator before emitting anything: a rejected
    // form must not leave earlier registrations in the output.
    for (auto *declarator : node->Declarators()) {
        if (declarator->Id() == nullptr || !declarator->Id()->IsIdentifier()) {
            // the host parser accepts destructuring declarations, so they
            // must be rejected here rather than at parse time
            return reject("Destructuring declaration is not supported in debugger evaluation");
        }
        if (declarator->Init() == nullptr) {
            return reject("Declaration without initializer is not supported in debugger evaluation");
        }
    }

    const bool readonly = kind == ir::VariableDeclaration::VariableDeclarationKind::CONST;
    EvalTempIdents lastIdents {};

    for (auto *declarator : node->Declarators()) {
        auto name = declarator->Id()->AsIdentifier()->Name();
        auto *init = TransformExpression(declarator->Init(), &stmts);
        auto idents = DeclareEvalTemp("$eval_decl", init, &stmts);
        // register after this initializer's transform: `let a = a + 1` reads
        // the outer a, `let a = 1, b = a + 1` binds this temp
        pendingDecls_[name] = idents.decl->Name();

        ArenaVector<ir::Expression *> defArgs(checker_->Allocator()->Adapter());
        defArgs.push_back(MakeIdentifier("thread"));
        defArgs.push_back(MakeIdentifier("frame"));
        defArgs.push_back(MakeStringLiteral(name));
        defArgs.push_back(idents.set);
        defArgs.push_back(checker_->AllocNode<ir::BooleanLiteral>(readonly));
        stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(
            MakeDebuggerAPIStaticCall("defineVariable", std::move(defArgs))));
        lastIdents = idents;
    }
    pendingDecls_.clear();

    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(lastIdents.ret));
    return BuildAssignReturnBlock(std::move(stmts));
}

// Dispatch entry: values and operators are handled here; accesses / calls
// and type operations delegate to TransformCompoundExpression, aggregates
// and construction to TransformAggregateExpression.
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
    auto *arg = node->Argument();

    // Member / subscript targets route through the single-snapshot handlers:
    // one receiver (and index) evaluation serves the old-value read and the
    // writeback.
    if (arg->IsMemberExpression()) {
        auto *me = arg->AsMemberExpression();
        if (me->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS || me->Kind() == ir::MemberExpressionKind::NONE ||
            me->Property()->IsStringLiteral()) {
            return HandleFieldTargetUpdate(me, node, stmts);
        }
        return HandleSubscriptTargetUpdate(me, node, stmts);
    }

    auto *assign = checker_->AllocNode<ir::AssignmentExpression>(arg, one, compoundOp);
    auto *block = TransformAssignment(assign);
    if (node->IsPrefix()) {
        return HarvestBlock(block, stmts);
    }
    // postfix on an identifier: frame reads are idempotent, keep the
    // historical shape
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

// Shared ++/-- shape for getField-style targets (member access `r.p` and
// string-literal subscript `r["p"]`): the receiver value is snapshotted
// once; the old-value read and the writeback each construct getField on top
// of the snapshot (getField itself is a pure proxy construction, so the
// only receiver evaluation happens at the snapshot).
//   r.p++ → let $base = <r>
//           → let $old = wrap($base).getField(p).value() as Object
//           → let $new = add($old, 1)
//           → wrap($base).getField(p).setValue(wrap($new))
//           → yields $old (prefix: $new)
ir::Expression *ExpressionASTTransformer::HandleFieldTargetUpdate(ir::MemberExpression *me, ir::UpdateExpression *node,
                                                                  ArenaVector<ir::Statement *> *stmts)
{
    auto *ownerValue = TransformExpression(me->Object(), stmts);
    auto ownerIdents = DeclareEvalTemp("$eval_base", ownerValue, stmts);

    util::StringView propName = me->Property()->IsIdentifier() ? me->Property()->AsIdentifier()->Name()
                                                               : me->Property()->AsStringLiteral()->Str();

    ArenaVector<ir::Expression *> readArgs(checker_->Allocator()->Adapter());
    readArgs.push_back(MakeStringLiteral(propName));
    auto *readField = MakeProxyInstanceCall(WrapInWrap(ownerIdents.ret), "getField", std::move(readArgs));
    auto *oldAsObject = checker_->AllocNode<ir::TSAsExpression>(
        WrapInValueCall(readField), helpers::CreateETSTypeReference(checker_, "Object"), false);
    auto oldIdents = DeclareEvalTemp("$eval_old", oldAsObject, stmts);

    auto *one = checker_->AllocNode<ir::NumberLiteral>(lexer::Number(1));
    ArenaVector<ir::Expression *> opArgs(checker_->Allocator()->Adapter());
    opArgs.push_back(oldIdents.set);
    opArgs.push_back(EnsureObject(checker_, one));
    auto opMethod = (node->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS_PLUS) ? std::string_view("add") : "sub";
    auto newIdents = DeclareEvalTemp("$eval_new", MakeDebuggerAPIStaticCall(opMethod, std::move(opArgs)), stmts);

    ArenaVector<ir::Expression *> writeArgs(checker_->Allocator()->Adapter());
    writeArgs.push_back(MakeStringLiteral(propName));
    auto *writeField = MakeProxyInstanceCall(WrapInWrap(ownerIdents.set), "getField", std::move(writeArgs));
    ArenaVector<ir::Expression *> svArgs(checker_->Allocator()->Adapter());
    svArgs.push_back(WrapInWrap(newIdents.set));
    auto *setCall = MakeProxyInstanceCall(writeField, "setValue", std::move(svArgs));
    stmts->push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));

    return node->IsPrefix() ? newIdents.ret : oldIdents.ret;
}

// ++/-- on an element-access target: receiver and index snapshotted once;
// the $_get read, the arithmetic and the setElement/$_set write share the
// snapshots.
ir::Expression *ExpressionASTTransformer::HandleSubscriptTargetUpdate(ir::MemberExpression *me,
                                                                      ir::UpdateExpression *node,
                                                                      ArenaVector<ir::Statement *> *stmts)
{
    auto *arrExpr = me->Object();
    auto *arrProxy = TransformExpression(arrExpr, stmts);
    auto arrIdents = DeclareEvalTemp("$eval_base", arrProxy, stmts);

    ir::Expression *idxRead = nullptr;
    ir::Expression *idxWrite = nullptr;
    if (me->Property()->IsNumberLiteral()) {
        idxRead = BuildSubscriptIndex(me->Property(), stmts);
        idxWrite = BuildSubscriptIndex(me->Property(), stmts);
    } else {
        auto idxIdents = DeclareEvalTemp("$eval_index", BuildSubscriptIndex(me->Property(), stmts), stmts);
        idxRead = idxIdents.ret;
        idxWrite = idxIdents.set;
    }

    auto *getCall = BuildSubscriptReflectionRead(arrIdents.ret, idxRead);
    auto oldIdents = DeclareEvalTemp("$eval_old", getCall, stmts);

    auto *one = checker_->AllocNode<ir::NumberLiteral>(lexer::Number(1));
    ArenaVector<ir::Expression *> opArgs(checker_->Allocator()->Adapter());
    // reflection reads are Any-typed (nullable): bridge to the Object the
    // DebuggerAPI operators take
    opArgs.push_back(EnsureObject(checker_, oldIdents.set));
    opArgs.push_back(EnsureObject(checker_, one));
    auto opMethod = (node->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS_PLUS) ? std::string_view("add") : "sub";
    auto newIdents = DeclareEvalTemp("$eval_new", MakeDebuggerAPIStaticCall(opMethod, std::move(opArgs)), stmts);

    ir::Expression *setCall = nullptr;
    if (arrExpr->IsIdentifier()) {
        ArenaVector<ir::Expression *> setElemArgs(checker_->Allocator()->Adapter());
        setElemArgs.push_back(MakeIdentifier("thread"));
        setElemArgs.push_back(MakeIdentifier("frame"));
        setElemArgs.push_back(MakeStringLiteral(arrExpr->AsIdentifier()->Name()));
        setElemArgs.push_back(idxWrite);
        setElemArgs.push_back(newIdents.set);
        setCall = MakeDebuggerAPIStaticCall("setElement", std::move(setElemArgs));
    } else {
        setCall = BuildSubscriptReflectionWrite(arrIdents.set, idxWrite, newIdents.set);
    }
    stmts->push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));

    return node->IsPrefix() ? newIdents.ret : oldIdents.ret;
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
            // unknown-typed values convert to a resizable Array copy for the
            // checker; builtin-indexable receivers (native arrays, tuples)
            // only work through the runtime conversion — a static
            // `as Array<Object>` cast fails for every non-resizable value
            if (NeedsSpreadArrayBridge(arg)) {
                arg = MakeToResizableArrayCall(arg);
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
        // Constant literal casts keep the native `as` node: the standard
        // checker then applies the mainline rules (ESE1050320 range check
        // for `256 as byte`, ESE0326 for `65 as char` / `1 as boolean`,
        // ESE123811 for `3.99 as int`) instead of the value-converting
        // toXxx dispatch, which silently truncates. Legal in-range forms
        // (`5 as int`) compile and evaluate natively -- identical result.
        if (transformedExpr->IsNumberLiteral()) {
            auto *clonedType =
                static_cast<ir::TypeNode *>(node->TypeAnnotation()->Clone(checker_->Allocator(), nullptr));
            return checker_->AllocNode<ir::TSAsExpression>(transformedExpr, clonedType, node->IsConst());
        }
        ArenaVector<ir::Expression *> castArgs(checker_->Allocator()->Adapter());
        castArgs.push_back(EnsureObject(checker_, transformedExpr));
        return MakeDebuggerAPIStaticCall(methodName, std::move(castArgs));
    }

    if (auto *erased = EraseArrayCast(node, transformedExpr); erased != nullptr) {
        return erased;
    }

    // Builtin fixed/value array casts (`x as FixedArray<T>` /
    // `x as ValueArray<T>`): predefined type-level syntax (reserved,
    // ESY0242) compiled to an array-descriptor checkcast by the standard
    // pipeline. The castAs string dispatch cannot resolve these names at
    // runtime ("Class not found"), and the mainline cast is legal -- so the
    // native path is the only correct lowering here.
    if (IsBuiltinArrayTypeName(node->TypeAnnotation())) {
        auto *clonedType = static_cast<ir::TypeNode *>(node->TypeAnnotation()->Clone(checker_->Allocator(), nullptr));
        return checker_->AllocNode<ir::TSAsExpression>(transformedExpr, clonedType, node->IsConst());
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
    // single-segment names only: the `as T` bridge requires T bindable here.
    // Object is bridged too: the transformed initializer is usually Any-typed
    // (get / .value() as Any / newInstance results), and Any is not
    // assignable to the non-nullable Object element type (ESE0127/ESE0046).
    // The cast is a no-op for already-Object-typed values (a W670214
    // warning) and boxes primitives, matching the spec's cast expression.
    auto elemName = ExtractTypeRefName(checker_, elemType);
    if (!elemName.Empty() && elemName.Utf8().find('.') == std::string_view::npos) {
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
        // Erase to Any, not Object: a null/undefined initializer must stay
        // assignable (Object is non-nullable; Array.create is generic, so
        // the erased element bytecode is identical either way).
        elemTypeRef = helpers::CreateETSTypeReference(checker_, "Any");
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
    // Builtin fixed/value array creation: the names are predefined
    // type-level syntax (reserved, ESY0242), never classes -- keep the node
    // native so the standard checker/FixedArrayLowering pipeline handles it
    // exactly like the main compiler (newarr intrinsic).
    if (IsBuiltinArrayTypeName(node->GetTypeRef())) {
        return TransformBuiltinArrayConstructor(node, stmts);
    }
    ArenaVector<ir::Expression *> newArgs(checker_->Allocator()->Adapter());
    newArgs.push_back(MakeIdentifier("thread"));
    newArgs.push_back(MakeIdentifier("frame"));
    newArgs.push_back(MakeStringLiteral(className));
    for (auto *arg : node->GetArguments()) {
        newArgs.push_back(TransformCallArgument(arg, stmts));
    }
    auto *newCall = MakeDebuggerAPIStaticCall("newInstance", std::move(newArgs));
    return checker_->AllocNode<ir::TSAsExpression>(newCall, helpers::CreateETSTypeReference(checker_, "Any"), false);
}

// `new FixedArray<T>(len[, elem])` / `new ValueArray<T>(len[, elem])` kept
// native: the type reference is a predefined name that binds without any
// scope lookup (CheckPredefinedBuiltinTypes), so the isolated eval module
// resolves it; FixedArrayLowering then desugars the node to a newarr
// intrinsic -- byte-identical to the main compiler's output for this syntax.
// The transformed arguments are Any/Object-typed and not assignable to the
// builtin constructor's signature, so they are bridged:
//   len  -- literals pass through, other forms via toInt (int dimension).
//   elem -- literals pass through, other forms via the element-type bridge
//           (as T / toXxx, same primitives as TransformNewArrayInstance).
// Malformed forms (no type arguments, non-primitive ValueArray elements,
// arity outside the constructor) reach the standard checker and are rejected
// with the main compiler's own diagnostics.
ir::Expression *ExpressionASTTransformer::TransformBuiltinArrayConstructor(ir::ETSNewClassInstanceExpression *node,
                                                                           ArenaVector<ir::Statement *> *stmts)
{
    auto *typeRef = node->GetTypeRef();
    // element type node: well-formed single type argument only; otherwise
    // null (the checker rejects the malformed reference before any bridge
    // would matter)
    ir::TypeNode *elemType = nullptr;
    if (typeRef->IsETSTypeReference()) {
        auto *part = typeRef->AsETSTypeReference()->Part();
        if (part != nullptr && part->TypeParams() != nullptr && part->TypeParams()->Params().size() == 1U) {
            elemType = part->TypeParams()->Params()[0];
        }
    }

    auto &args = node->GetArguments();
    ArenaVector<ir::Expression *> newArgs(checker_->Allocator()->Adapter());

    // len: literals pass through; other forms bridge through toInt (the
    // builtin constructor requires an int dimension)
    if (!args.empty()) {
        auto *len = TransformExpression(args[0], stmts);
        if (!len->IsNumberLiteral()) {
            ArenaVector<ir::Expression *> toIntArgs(checker_->Allocator()->Adapter());
            toIntArgs.push_back(EnsureObject(checker_, len));
            len = MakeDebuggerAPIStaticCall("toInt", std::move(toIntArgs));
        }
        newArgs.push_back(len);
    }
    // elem: literals pass through; other forms bridge to the element type
    if (args.size() > 1U) {
        auto *elem = TransformExpression(args[1], stmts);
        if (!IsLiteralNode(elem) && elemType != nullptr) {
            elem = BridgeValueByElementType(UnparenthesizeType(elemType), elem);
        }
        newArgs.push_back(elem);
    }
    // extra arguments (arity over the constructor form): transformed raw --
    // the standard checker applies the builtin signature rules
    for (size_t i = 2U; i < args.size(); i++) {
        newArgs.push_back(TransformExpression(args[i], stmts));
    }

    auto *clonedTypeRef = static_cast<ir::Expression *>(typeRef->Clone(checker_->Allocator(), nullptr));
    return checker_->AllocNode<ir::ETSNewClassInstanceExpression>(clonedTypeRef, std::move(newArgs));
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
    // Declaration statement being transformed (let a = 1, b = a + 1): a
    // reference to an earlier declarator binds its temp directly -- lexical
    // statement semantics -- instead of the runtime frame lookup, which a
    // same-named frame variable would shadow. Empty outside
    // TransformDeclaration.
    if (auto it = pendingDecls_.find(node->Name()); it != pendingDecls_.end()) {
        return MakeIdentifier(it->second);
    }

    ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
    args.push_back(MakeIdentifier("thread"));
    args.push_back(MakeIdentifier("frame"));
    args.push_back(MakeStringLiteral(node->Name()));

    return MakeDebuggerAPIStaticCall("get", std::move(args));
}

ir::Expression *ExpressionASTTransformer::TransformNullishCoalescing(ir::BinaryExpression *node,
                                                                     ArenaVector<ir::Statement *> *stmts)
{
    // a ?? b → let $t = <a>; ($t as Any != null) ? $t : <b>
    auto *leftExpr = TransformExpression(node->Left(), stmts);

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

    // The $t snapshot is declared BEFORE the right operand transforms, so
    // the right-hand settling statements cannot run ahead of the left-value
    // capture. A pure right operand keeps the exact historical conditional
    // shape; a statement-producing one is lifted into the null branch so its
    // writebacks run only when the left value is nullish (the historical
    // shared vector emitted them unconditionally).
    ArenaVector<ir::Statement *> rightStmts(checker_->Allocator()->Adapter());
    auto *right = TransformExpression(node->Right(), &rightStmts);
    if (rightStmts.empty()) {
        return checker_->AllocNode<ir::ConditionalExpression>(nullCheck, consequentLeft, right);
    }
    ArenaVector<ir::Statement *> emptyStmts(checker_->Allocator()->Adapter());
    return LiftConditionalBranches(nullCheck, consequentLeft, std::move(emptyStmts), right, std::move(rightStmts),
                                   stmts);
}

ir::Expression *ExpressionASTTransformer::TransformInstanceof(ir::BinaryExpression *node,
                                                              ArenaVector<ir::Statement *> *stmts)
{
    auto *left = TransformExpression(node->Left(), stmts);
    // An instantiated generic on the instanceof RHS is a mainline compile
    // error (checker INSTANCEOF_ERASED, ESY18871); erasing the type
    // arguments into a runtime instanceofOp call would silently succeed.
    // Keep the native node -- the eval pipeline rejects it (the unresolvable
    // user class name fails BIND even before the checker's rule fires).
    if (HasTypeArguments(node->Right())) {
        auto *clonedRight = static_cast<ir::Expression *>(node->Right()->Clone(checker_->Allocator(), nullptr));
        return checker_->AllocNode<ir::BinaryExpression>(left, clonedRight, node->OperatorType());
    }
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
        // Pure-literal shapes that the mainline checker rejects keep the
        // native node, so the standard checker applies the same rules:
        // - char in arithmetic/bitwise/shift (spec: char allows equality and
        //   relational only; mainline ESE0107/ESE0108/ESE4201);
        // - '>>>' over a bigint literal ('<<'/'>>' are spec-legal for bigint
        //   and stay on the dispatch; mainline ESE0107).
        // Relational operators are never diverted: char comparisons
        // (`c'A' < c'B'`) are spec-legal and evaluate through lt/gt/le/ge.
        if (IsLiteralNode(left) && IsLiteralNode(right) && !IsRelationalOp(op) &&
            ((left->IsCharLiteral() || right->IsCharLiteral()) ||
             (op == lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT &&
              (left->IsBigIntLiteral() || right->IsBigIntLiteral())))) {
            return checker_->AllocNode<ir::BinaryExpression>(left, right, op);
        }
        ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
        // DebuggerAPI operators take Object: bridge Any-typed operands.
        args.push_back(EnsureObject(checker_, left));
        args.push_back(EnsureObject(checker_, right));
        return MakeDebuggerAPIStaticCall(it->second, std::move(args));
    }
    return checker_->AllocNode<ir::BinaryExpression>(left, right, op);
}

// Short-circuit && / || over the already-transformed operands. A pure right
// operand (empty vector) keeps the native operator — with the nullish
// as-Any fixup, which keeps a real runtime comparison against a
// null/undefined literal (W65001 folding, see the flags comment in
// TransformBinary). A statement-producing right operand cannot ride the
// native operator, so it lifts into an if-shape with the left value
// captured once. A falsy `&&` / truthy `||` left keeps the left value as
// the result, matching the operators' value semantics.
//   a && b → let $t = <a>; if ($t) {…; r = <b>} else {r = $t}
//   a || b → let $t = <a>; if ($t) {r = $t} else {…; r = <b>}
ir::Expression *ExpressionASTTransformer::TransformLogicalBinary(lexer::TokenType op, ir::Expression *left,
                                                                 ir::Expression *right, bool rightIsNullish,
                                                                 bool leftIsNullish,
                                                                 ArenaVector<ir::Statement *> &&rightStmts,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    if (rightStmts.empty()) {
        if (rightIsNullish) {
            left =
                checker_->AllocNode<ir::TSAsExpression>(left, helpers::CreateETSTypeReference(checker_, "Any"), false);
        } else if (leftIsNullish) {
            right =
                checker_->AllocNode<ir::TSAsExpression>(right, helpers::CreateETSTypeReference(checker_, "Any"), false);
        }
        return checker_->AllocNode<ir::BinaryExpression>(left, right, op);
    }
    auto idents = DeclareEvalTemp("$eval_logic", left, stmts);
    ArenaVector<ir::Statement *> emptyStmts(checker_->Allocator()->Adapter());
    if (op == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) {
        return LiftConditionalBranches(idents.ret, right, std::move(rightStmts), idents.set, std::move(emptyStmts),
                                       stmts);
    }
    return LiftConditionalBranches(idents.ret, idents.set, std::move(emptyStmts), right, std::move(rightStmts), stmts);
}

// Settle-left shape for the non-short-circuit operators with a
// statement-producing right operand: capture the left value into a temp,
// run the right settling statements, then the right value, then the
// operation — restoring the spec left→right evaluation order (a shared
// statement vector would emit the right writebacks ahead of the left read).
ir::Expression *ExpressionASTTransformer::TransformSettleLeftBinary(lexer::TokenType op, ir::Expression *left,
                                                                    ir::Expression *right, bool leftIsNullish,
                                                                    ArenaVector<ir::Statement *> &&rightStmts,
                                                                    ArenaVector<ir::Statement *> *stmts)
{
    auto leftIdents = DeclareEvalTemp("$eval_operand", left, stmts);
    stmts->insert(stmts->end(), rightStmts.begin(), rightStmts.end());
    auto rightIdents = DeclareEvalTemp("$eval_operand", right, stmts);
    if (IsEqualityOp(op)) {
        ir::Expression *rightRead = rightIdents.ret;
        if (leftIsNullish) {
            rightRead = checker_->AllocNode<ir::TSAsExpression>(
                rightRead, helpers::CreateETSTypeReference(checker_, "Any"), false);
        }
        return checker_->AllocNode<ir::BinaryExpression>(leftIdents.ret, rightRead, op);
    }
    return TransformArithmeticBinary(op, leftIdents.ret, rightIdents.ret);
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

    // `2n ** -1n` (literal base, negative bigint literal exponent): the
    // mainline checker rejects a negative bigint exponent at compile time
    // (ESE655064). Pure-literal shape: clone the original operands (no
    // identifiers, binds natively) and keep the native node, so the standard
    // checker rejects it instead of the runtime RangeError from the
    // DebuggerAPI pow dispatch. Variable-base forms keep the dispatch.
    if (op == lexer::TokenType::PUNCTUATOR_EXPONENTIATION && IsNegatedBigIntLiteral(node->Right()) &&
        IsLiteralNode(node->Left())) {
        auto *leftClone = static_cast<ir::Expression *>(node->Left()->Clone(checker_->Allocator(), nullptr));
        auto *rightClone = static_cast<ir::Expression *>(node->Right()->Clone(checker_->Allocator(), nullptr));
        return checker_->AllocNode<ir::BinaryExpression>(leftClone, rightClone, op);
    }

    auto *left = TransformExpression(node->Left(), stmts);
    // The right operand settles into an isolated statement vector: the left
    // value must be captured before its settling statements run (spec
    // left→right order), and for the short-circuit operators the statements
    // may only execute in the taken branch. A pure right operand (empty
    // vector) keeps the exact historical output.
    ArenaVector<ir::Statement *> rightStmts(checker_->Allocator()->Adapter());
    auto *right = TransformExpression(node->Right(), &rightStmts);

    // Null/undefined comparison: cast the other operand to Any. The
    // Object-typed producers (bare get/getThis, proxy $_get) can deliver
    // null at runtime, but Object is non-nullable in ETS — the checker
    // folds `x == null` to constant false (W65001). `as Any` keeps a
    // real runtime comparison (same mechanism as the ?? transform above).
    bool rightIsNullish = node->Right()->IsNullLiteral() || node->Right()->IsUndefinedLiteral();
    bool leftIsNullish = node->Left()->IsNullLiteral() || node->Left()->IsUndefinedLiteral();

    if (IsLogicalOp(op)) {
        return TransformLogicalBinary(op, left, right, rightIsNullish, leftIsNullish, std::move(rightStmts), stmts);
    }

    if (!rightStmts.empty()) {
        return TransformSettleLeftBinary(op, left, right, leftIsNullish, std::move(rightStmts), stmts);
    }

    if (IsEqualityOp(op)) {
        if (rightIsNullish) {
            left =
                checker_->AllocNode<ir::TSAsExpression>(left, helpers::CreateETSTypeReference(checker_, "Any"), false);
        } else if (leftIsNullish) {
            right =
                checker_->AllocNode<ir::TSAsExpression>(right, helpers::CreateETSTypeReference(checker_, "Any"), false);
        }
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
        // here would double-evaluate it.
        // Result wrappers use `as Any` (not `as Object`): member reads may
        // deliver null at runtime and Object is non-nullable in ETS — the
        // checker would fold `obj.x == null` to false (W65001). Consumers
        // needing Object bridge back via EnsureObject.
        auto *chain = BuildMemberChain(node, stmts);
        auto *valueCall = WrapInValueCall(chain);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"),
                                                       false);
    }
    return TransformElementAccess(node, stmts);
}

ir::Expression *ExpressionASTTransformer::TransformElementAccess(ir::MemberExpression *node,
                                                                 ArenaVector<ir::Statement *> *stmts)
{
    auto *obj = TransformExpression(node->Object(), stmts);

    auto *idxExpr = node->Property();

    // string subscript == field access: getField via ValueProxy reflection
    if (idxExpr->IsStringLiteral()) {
        auto *propStr = TransformExpression(idxExpr, stmts)->AsStringLiteral();
        ArenaVector<ir::Expression *> fargs(checker_->Allocator()->Adapter());
        fargs.push_back(propStr);
        auto *wrapped = WrapInWrap(obj);
        auto *getFieldCall = MakeProxyInstanceCall(wrapped, "getField", std::move(fargs));
        auto *valueCall = WrapInValueCall(getFieldCall);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"),
                                                       false);
    }

    // String literal receiver: keep the native String.$_get lowering (spec
    // "String Indexing Expression"). The checker resolves $_get on
    // std.core.String (a boot class) and objectIndexAccess lowers the call,
    // exactly as for a normally compiled program.
    if (node->Object()->IsStringLiteral()) {
        auto *nativeIdx = BuildSubscriptIndex(idxExpr, stmts);
        return checker_->AllocNode<ir::MemberExpression>(obj, nativeIdx, ir::MemberExpressionKind::ELEMENT_ACCESS, true,
                                                         false);
    }

    // Any-typed receiver (get/getThis/proxy results): dispatch $_get at
    // runtime through DebugProxy.call reflection — arrays, strings and custom
    // indexer classes alike. A static `as Array<Object>` cast compiles, but
    // the emitted checkcast fails at runtime for every non-array receiver.
    auto *idx = BuildSubscriptIndex(idxExpr, stmts);
    auto *wrapped = WrapInWrap(obj);
    ArenaVector<ir::Expression *> callArgs(checker_->Allocator()->Adapter());
    callArgs.push_back(MakeStringLiteral("$_get"));
    callArgs.push_back(idx);
    auto *getCall = MakeProxyInstanceCall(wrapped, "call", std::move(callArgs));
    auto *valueCall = WrapInValueCall(getCall);
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"), false);
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
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"), false);
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
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"),
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
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"), false);
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
        // Runtime contract: the 4th argument carries the explicit generic
        // instantiation ("" when absent) so the runtime can reproduce the
        // compiler's instantiation-boundary conversions (argument boxing
        // per T) that bytecode erasure otherwise drops.
        fnArgs.push_back(MakeStringLiteral(FormatCallTypeArgs(checker_, node->TypeParams())));
        for (auto *arg : node->Arguments()) {
            fnArgs.push_back(TransformCallArgument(arg, stmts));
        }
        auto *fnCall = MakeDebuggerAPIStaticCall("callFunction", std::move(fnArgs));
        auto *valueCall = WrapInValueCall(fnCall);
        return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"),
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
    return checker_->AllocNode<ir::TSAsExpression>(valueCall, helpers::CreateETSTypeReference(checker_, "Any"), false);
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
        return checker_->AllocNode<ir::TSAsExpression>(val, helpers::CreateETSTypeReference(checker_, "Any"), false);
    }
    // ELEMENT_ACCESS inside a chain: the chain value is Any-typed, so use the
    // same runtime $_get dispatch as TransformElementAccess — a static
    // Array<Object> cast would break string and custom-indexer receivers.
    auto *idx = BuildSubscriptIndex(me->Property(), stmts);
    auto *wrapped = WrapInWrap(target);
    ArenaVector<ir::Expression *> callArgs(checker_->Allocator()->Adapter());
    callArgs.push_back(MakeStringLiteral("$_get"));
    callArgs.push_back(idx);
    auto *getCall = MakeProxyInstanceCall(wrapped, "call", std::move(callArgs));
    auto *val = WrapInValueCall(getCall);
    return checker_->AllocNode<ir::TSAsExpression>(val, helpers::CreateETSTypeReference(checker_, "Any"), false);
}

ir::Expression *ExpressionASTTransformer::BuildChainInvokeValue(ir::Expression *receiver,
                                                                ArenaVector<ir::Expression *> &&args)
{
    auto *wrapped = WrapInWrap(receiver);
    auto *invokeResult = MakeProxyInstanceCall(wrapped, "invoke", std::move(args));
    auto *accessValue = WrapInValueCall(invokeResult);
    return checker_->AllocNode<ir::TSAsExpression>(accessValue, helpers::CreateETSTypeReference(checker_, "Any"),
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
            // The element-access index settles into an isolated vector: its
            // writebacks must only run in the taken branch (the chain may
            // short-circuit ahead of this access). Property access produces
            // no statements, so the fast path is unchanged for it.
            ArenaVector<ir::Statement *> idxStmts(checker_->Allocator()->Adapter());
            auto *accessValue = BuildChainMemberAccess(me, cond->Consequent(), &idxStmts);
            if (idxStmts.empty()) {
                return checker_->AllocNode<ir::ConditionalExpression>(cond->Test(), accessValue, cond->Alternate());
            }
            ArenaVector<ir::Statement *> emptyStmts(checker_->Allocator()->Adapter());
            return LiftConditionalBranches(cond->Test(), accessValue, std::move(idxStmts), cond->Alternate(),
                                           std::move(emptyStmts), stmts);
        }
        return BuildChainMemberAccess(me, base, stmts);
    }

    // save base to temp + null check + conditional
    ir::Identifier *tempRef = nullptr;
    auto *nullCheck = DeclareNullCheckedTemp(base, "$eval_opt", &tempRef, stmts);
    ArenaVector<ir::Statement *> idxStmts(checker_->Allocator()->Adapter());
    auto *accessValue = BuildChainMemberAccess(me, tempRef, &idxStmts);
    if (idxStmts.empty()) {
        return checker_->AllocNode<ir::ConditionalExpression>(nullCheck, accessValue,
                                                              checker_->AllocNode<ir::UndefinedLiteral>());
    }
    ArenaVector<ir::Statement *> emptyStmts(checker_->Allocator()->Adapter());
    return LiftConditionalBranches(nullCheck, accessValue, std::move(idxStmts),
                                   checker_->AllocNode<ir::UndefinedLiteral>(), std::move(emptyStmts), stmts);
}

ir::Expression *ExpressionASTTransformer::TransformOptionalCall(ir::CallExpression *call,
                                                                ArenaVector<ir::Statement *> *stmts)
{
    auto *callee = TransformOptionalChain(call->Callee(), stmts);

    // Arguments settle into an isolated vector: a pure argument list keeps
    // the historical shape, while settling statements (assignment
    // writebacks) may only run when the chain is not short-circuited (the
    // historical shared vector leaked them ahead of the null check).
    auto buildArgs = [this, call](ArenaVector<ir::Statement *> *argStmts) {
        ArenaVector<ir::Expression *> invokeArgs(checker_->Allocator()->Adapter());
        for (auto *arg : call->Arguments()) {
            invokeArgs.push_back(TransformCallArgument(arg, argStmts));
        }
        return invokeArgs;
    };

    if (!call->IsOptional()) {
        // nest inside the base conditional's consequent so the outer
        // short-circuit extends through the call
        if (callee->IsConditionalExpression()) {
            auto *cond = callee->AsConditionalExpression();
            ArenaVector<ir::Statement *> argStmts(checker_->Allocator()->Adapter());
            auto *accessValue = BuildChainInvokeValue(cond->Consequent(), buildArgs(&argStmts));
            if (argStmts.empty()) {
                return checker_->AllocNode<ir::ConditionalExpression>(cond->Test(), accessValue, cond->Alternate());
            }
            ArenaVector<ir::Statement *> emptyStmts(checker_->Allocator()->Adapter());
            return LiftConditionalBranches(cond->Test(), accessValue, std::move(argStmts), cond->Alternate(),
                                           std::move(emptyStmts), stmts);
        }
        return BuildChainInvokeValue(callee, buildArgs(stmts));
    }

    ir::Identifier *tempRef = nullptr;
    auto *nullCheck = DeclareNullCheckedTemp(callee, "$eval_opt", &tempRef, stmts);
    ArenaVector<ir::Statement *> argStmts(checker_->Allocator()->Adapter());
    auto *accessValue = BuildChainInvokeValue(tempRef, buildArgs(&argStmts));
    if (argStmts.empty()) {
        return checker_->AllocNode<ir::ConditionalExpression>(nullCheck, accessValue,
                                                              checker_->AllocNode<ir::UndefinedLiteral>());
    }
    ArenaVector<ir::Statement *> emptyStmts(checker_->Allocator()->Adapter());
    return LiftConditionalBranches(nullCheck, accessValue, std::move(argStmts),
                                   checker_->AllocNode<ir::UndefinedLiteral>(), std::move(emptyStmts), stmts);
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
        // the runtime conversion provides an iterable for every receiver kind
        // (a static Array<Object> cast would fail for non-resizable values)
        auto *transformed = TransformExpression(arg->AsSpreadElement()->Argument(), stmts);
        auto *asArray = MakeToResizableArrayCall(transformed);
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

    if (left->IsETSDestructuring()) {
        return TransformDestructuringAssignment(node);
    }

    ArenaVector<ir::Statement *> stmts(checker_->Allocator()->Adapter());

    ir::Expression *lhsProxy = nullptr;
    ir::Expression *rawForArith = nullptr;  // Object for compound arithmetic

    if (left->IsIdentifier()) {
        // Identifier: BuildAssignWriteback emits the DebuggerAPI.set() writeback.
        rawForArith = TransformExpression(left, &stmts);  // copy for compound arithmetic
    } else if (left->IsMemberExpression()) {
        auto *me = left->AsMemberExpression();
        if (me->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS || me->Kind() == ir::MemberExpressionKind::NONE) {
            if (op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
                // Simple `=`: one evaluation of the receiver chain; the
                // historical single-eval path is kept unchanged.
                lhsProxy = BuildMemberLHS(me, &stmts);
            } else {
                // Compound: the cloned-read shape would re-evaluate the whole
                // receiver chain (side-effecting bases and getters twice) —
                // route through the single-snapshot compound path.
                return HandleMemberCompoundAssignment(me, node, std::move(stmts));
            }
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

// Destructuring assignment [a, , b] = rhs over the DebuggerAPI primitives:
//   let $eval_dstr_N = <rhs>
//   → set(thread, frame, "a", wrap($base).call("$_get", 0).value() as Any as Object)
//   → set(thread, frame, "b", wrap($base).call("$_get", 2).value() as Any as Object)
//   → yields $base
// Mirrors DestructuringPhase::ProcessAssignmentExpression semantics: the RHS
// is evaluated once into a snapshot, elements are read by index (holes skip
// the index forward) and the value of the whole expression is the RHS. Only
// identifier elements are supported — matching the compiler's own
// destructuring surface (rest/default/nested elements are compile errors in
// the normal pipeline, see DestructuringElementRejectMessage).
ir::Statement *ExpressionASTTransformer::TransformDestructuringAssignment(ir::AssignmentExpression *node)
{
    ArenaVector<ir::Statement *> stmts(checker_->Allocator()->Adapter());

    // Defensive: the compound-operator form ([a, b] += x) parses the LHS as an
    // ArrayExpression and fails earlier (INVALID_LEFT_SIDE_IN_ASSIGNMENT); the
    // plain '=' form reaches here as ETSDestructuring.
    if (node->OperatorType() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(
            ReportUnsupported("Compound destructuring assignment is not supported in debugger evaluation", checker_)));
        return BuildAssignReturnBlock(std::move(stmts));
    }

    auto *dstr = node->Left()->AsETSDestructuring();
    for (auto *elem : dstr->Elements()) {
        if (elem->IsIdentifier() || elem->IsOmittedExpression()) {
            continue;
        }
        stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(
            ReportUnsupported(DestructuringElementRejectMessage(elem), checker_)));
        return BuildAssignReturnBlock(std::move(stmts));
    }

    // The eval RHS is always a call expression (get/...) rather than a plain
    // identifier, so it is snapshotted unconditionally: the per-element reads
    // must not re-evaluate it (DestructuringPhase skips the temp only for
    // plain identifier RHS, which cannot occur here).
    auto *rhsValue = TransformExpression(node->Right(), &stmts);
    auto baseName = MakeUniqueName(checker_, "$eval_dstr");
    auto *baseDeclId = MakeIdentifier(baseName);
    auto *baseDecl = checker_->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::LET, baseDeclId, rhsValue);
    ArenaVector<ir::VariableDeclarator *> baseDecls(checker_->Allocator()->Adapter());
    baseDecls.push_back(baseDecl);
    stmts.push_back(checker_->AllocNode<ir::VariableDeclaration>(ir::VariableDeclaration::VariableDeclarationKind::LET,
                                                                 checker_->Allocator(), std::move(baseDecls)));

    // Per-element: read $base[i] through the $_get reflection dispatch, write
    // back through the type-aware DebuggerAPI.set (same writeback primitive
    // as the identifier assignment path in BuildAssignWriteback).
    uint32_t idx = 0;
    for (auto *elem : dstr->Elements()) {
        if (elem->IsOmittedExpression()) {
            idx++;
            continue;
        }
        auto *read = BuildDestructuringElementRead(baseName, idx);
        ArenaVector<ir::Expression *> setArgs(checker_->Allocator()->Adapter());
        setArgs.push_back(MakeIdentifier("thread"));
        setArgs.push_back(MakeIdentifier("frame"));
        setArgs.push_back(MakeStringLiteral(elem->AsIdentifier()->Name()));
        setArgs.push_back(EnsureObject(checker_, read));
        stmts.push_back(
            checker_->AllocNode<ir::ExpressionStatement>(MakeDebuggerAPIStaticCall("set", std::move(setArgs))));
        idx++;
    }

    // The value of a destructuring assignment is the RHS.
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(MakeIdentifier(baseName)));
    return BuildAssignReturnBlock(std::move(stmts));
}

// Reads $base[index] through the DebugProxy $_get reflection dispatch — the
// same shape as TransformElementAccess's Any-receiver branch. The snapshot
// reference is built on an independent identifier per read: temp references
// never pass through TransformExpression (TransformIdentifier would emit a
// get() against the user frame instead of reading the local temp).
ir::Expression *ExpressionASTTransformer::BuildDestructuringElementRead(util::StringView baseName, uint32_t index)
{
    auto *idx = checker_->AllocNode<ir::NumberLiteral>(lexer::Number(index));
    auto *wrapped = WrapInWrap(MakeIdentifier(baseName));
    ArenaVector<ir::Expression *> callArgs(checker_->Allocator()->Adapter());
    callArgs.push_back(MakeStringLiteral("$_get"));
    callArgs.push_back(idx);
    auto *getCall = MakeProxyInstanceCall(wrapped, "call", std::move(callArgs));
    return checker_->AllocNode<ir::TSAsExpression>(WrapInValueCall(getCall),
                                                   helpers::CreateETSTypeReference(checker_, "Any"), false);
}

// wrap(base).call("$_get", idx).value(): the runtime dispatches over the
// actual receiver kind — resizable arrays and custom indexable classes hit
// their real '$_get' method, native arrays (FixedArray/ValueArray) and
// tuples fall into the builtin miss-path of callInstanceMember.
ir::Expression *ExpressionASTTransformer::BuildSubscriptReflectionRead(ir::Expression *base, ir::Expression *idx)
{
    auto *wrapped = WrapInWrap(base);
    ArenaVector<ir::Expression *> callArgs(checker_->Allocator()->Adapter());
    callArgs.push_back(MakeStringLiteral("$_get"));
    callArgs.push_back(idx);
    auto *getCall = MakeProxyInstanceCall(wrapped, "call", std::move(callArgs));
    return WrapInValueCall(getCall);
}

// wrap(base).call("$_set", idx, value): same runtime dispatch on the write
// side; '$_set' returns void and the write expression's value is the RHS,
// which the caller already holds separately.
ir::Expression *ExpressionASTTransformer::BuildSubscriptReflectionWrite(ir::Expression *base, ir::Expression *idx,
                                                                        ir::Expression *value)
{
    auto *wrapped = WrapInWrap(base);
    ArenaVector<ir::Expression *> callArgs(checker_->Allocator()->Adapter());
    callArgs.push_back(MakeStringLiteral("$_set"));
    callArgs.push_back(idx);
    callArgs.push_back(EnsureObject(checker_, value));
    return MakeProxyInstanceCall(wrapped, "call", std::move(callArgs));
}

// DebuggerAPI.toResizableArray(arg): provides the iterable that the spread
// lowering requires, for every builtin-indexable receiver kind — replacing
// the old `as Array<Object>` bridge cast, which only ever holds for
// resizable arrays (native arrays, tuples fail at runtime).
ir::Expression *ExpressionASTTransformer::MakeToResizableArrayCall(ir::Expression *arg)
{
    ArenaVector<ir::Expression *> args(checker_->Allocator()->Adapter());
    args.push_back(EnsureObject(checker_, arg));
    return MakeDebuggerAPIStaticCall("toResizableArray", std::move(args));
}

// Compound member assignment over a single receiver evaluation:
//   receiver.prop op= rhs
//     → let $base = <receiver value>
//     → let $cur  = wrap($base).getField(prop).value() as Object
//     → let $res  = <op>($cur, <rhs>)
//     → wrap($base).getField(prop).setValue(wrap($res))
//     → yields $res
// The receiver is snapshotted once (its value — the field owner); the read
// and the write construct getField on top of the snapshot (a pure proxy
// construction, see HandleFieldTargetUpdate). The read must not clone the
// getField chain from the raw receiver, which would re-invoke
// side-effecting receivers and getters. The read is settled before the RHS
// transforms, restoring the spec receiver→read→RHS→write order.
ir::Statement *ExpressionASTTransformer::HandleMemberCompoundAssignment(ir::MemberExpression *me,
                                                                        ir::AssignmentExpression *node,
                                                                        ArenaVector<ir::Statement *> &&stmts)
{
    auto *ownerValue = TransformExpression(me->Object(), &stmts);
    auto ownerIdents = DeclareEvalTemp("$eval_base", ownerValue, &stmts);

    auto propName = me->Property()->AsIdentifier()->Name();

    ArenaVector<ir::Expression *> readArgs(checker_->Allocator()->Adapter());
    readArgs.push_back(MakeStringLiteral(propName));
    auto *readField = MakeProxyInstanceCall(WrapInWrap(ownerIdents.ret), "getField", std::move(readArgs));
    auto *curAsObject = checker_->AllocNode<ir::TSAsExpression>(
        WrapInValueCall(readField), helpers::CreateETSTypeReference(checker_, "Object"), false);
    auto curIdents = DeclareEvalTemp("$eval_cur", curAsObject, &stmts);

    auto *rhsValue = BuildCompoundRhs(node, curIdents.ret, &stmts);
    auto resultIdents = DeclareEvalTemp("$eval_result", rhsValue, &stmts);

    ArenaVector<ir::Expression *> writeArgs(checker_->Allocator()->Adapter());
    writeArgs.push_back(MakeStringLiteral(propName));
    auto *writeField = MakeProxyInstanceCall(WrapInWrap(ownerIdents.set), "getField", std::move(writeArgs));
    ArenaVector<ir::Expression *> svArgs(checker_->Allocator()->Adapter());
    svArgs.push_back(WrapInWrap(resultIdents.set));
    auto *setCall = MakeProxyInstanceCall(writeField, "setValue", std::move(svArgs));
    stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(resultIdents.ret));
    return BuildAssignReturnBlock(std::move(stmts));
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
    // the subscript read may come from the reflection form (Any-typed,
    // nullable): bridge to the Object the DebuggerAPI operators take
    opArgs.push_back(EnsureObject(checker_, curValue));
    // Result wrappers are Any-typed (nullable); the DebuggerAPI operators
    // take Object — bridge back explicitly (null passes the cast at runtime
    // and the operator throws a TypeError on null arithmetic, as designed).
    opArgs.push_back(EnsureObject(checker_, rightRaw));
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
    // Receiver value snapshotted once; the read and the write construct
    // getField on top of the snapshot (the historical compound path cloned
    // arrProxy for the read, re-invoking side-effecting receivers).
    auto ownerIdents = DeclareEvalTemp("$eval_base", arrProxy, &stmts);

    ir::Expression *rhsValue = nullptr;
    auto op = node->OperatorType();
    if (op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        rhsValue = TransformExpression(node->Right(), &stmts);
    } else {
        // read settled before the RHS: spec order receiver→read→RHS→write
        auto *propStr1 = checker_->AllocNode<ir::StringLiteral>(propStr->Str());
        ArenaVector<ir::Expression *> readArgs(checker_->Allocator()->Adapter());
        readArgs.push_back(propStr1);
        auto *readField = MakeProxyInstanceCall(WrapInWrap(ownerIdents.ret), "getField", std::move(readArgs));
        auto *curVal = checker_->AllocNode<ir::TSAsExpression>(
            WrapInValueCall(readField), helpers::CreateETSTypeReference(checker_, "Object"), false);
        auto curIdents = DeclareEvalTemp("$eval_cur", curVal, &stmts);
        rhsValue = BuildCompoundRhs(node, curIdents.ret, &stmts);
    }

    auto idents = DeclareEvalTemp("$eval_result", rhsValue, &stmts);
    auto *propStr2 = checker_->AllocNode<ir::StringLiteral>(propStr->Str());
    ArenaVector<ir::Expression *> writeArgs(checker_->Allocator()->Adapter());
    writeArgs.push_back(propStr2);
    auto *writeField = MakeProxyInstanceCall(WrapInWrap(ownerIdents.set), "getField", std::move(writeArgs));
    ArenaVector<ir::Expression *> svArgs(checker_->Allocator()->Adapter());
    svArgs.push_back(WrapInWrap(idents.set));
    auto *setCall = MakeProxyInstanceCall(writeField, "setValue", std::move(svArgs));
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

ir::Statement *ExpressionASTTransformer::EmitSubscriptWriteback(ir::Expression *arrExpr, ir::Expression *idxExpr,
                                                                ir::AssignmentExpression *node, lexer::TokenType op,
                                                                ArenaVector<ir::Statement *> &&stmts)
{
    // Receiver and index are evaluated once into temps; the read (compound
    // only), the RHS and the write share the snapshots. The historical shape
    // cloned arrProxy / idxForGet for the write path, re-invoking
    // side-effecting receivers and index expressions. An identifier receiver
    // on a plain `=` needs no temp at all: setElement resolves the array by
    // name at runtime (a hoisted get would be a redundant frame read).
    const bool needsReceiverTemp = !(arrExpr->IsIdentifier() && op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    auto arrIdents = needsReceiverTemp ? DeclareEvalTemp("$eval_base", TransformExpression(arrExpr, &stmts), &stmts)
                                       : EvalTempIdents {nullptr, nullptr, nullptr};

    // index: literals pass through as independent nodes; other forms are
    // bridged through toInt and hoisted once
    ir::Expression *idxRead = nullptr;
    ir::Expression *idxWrite = nullptr;
    if (idxExpr->IsNumberLiteral()) {
        idxRead = BuildSubscriptIndex(idxExpr, &stmts);
        idxWrite = BuildSubscriptIndex(idxExpr, &stmts);
    } else {
        auto idxIdents = DeclareEvalTemp("$eval_index", BuildSubscriptIndex(idxExpr, &stmts), &stmts);
        idxRead = idxIdents.ret;
        idxWrite = idxIdents.set;
    }

    ir::Expression *rhsValue = nullptr;
    if (op == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        rhsValue = TransformExpression(node->Right(), &stmts);
    } else {
        // settle the read before the RHS: spec order receiver→index→read→RHS
        auto *curVal = BuildSubscriptReflectionRead(arrIdents.ret, idxRead);
        auto curIdents = DeclareEvalTemp("$eval_cur", curVal, &stmts);
        rhsValue = BuildCompoundRhs(node, curIdents.ret, &stmts);
    }

    auto resultIdents = DeclareEvalTemp("$eval_result", rhsValue, &stmts);

    // write: identifier receivers route through the type-aware
    // DebuggerAPI.setElement; others dispatch at runtime via the proxy
    // reflection call (a call-site Array<Object> cast would fail for every
    // non-resizable receiver)
    ir::Expression *setCall = nullptr;
    if (arrExpr->IsIdentifier()) {
        ArenaVector<ir::Expression *> setElemArgs(checker_->Allocator()->Adapter());
        setElemArgs.push_back(MakeIdentifier("thread"));
        setElemArgs.push_back(MakeIdentifier("frame"));
        setElemArgs.push_back(MakeStringLiteral(arrExpr->AsIdentifier()->Name()));
        setElemArgs.push_back(idxWrite);
        setElemArgs.push_back(EnsureObject(checker_, resultIdents.set));
        setCall = MakeDebuggerAPIStaticCall("setElement", std::move(setElemArgs));
    } else {
        setCall = BuildSubscriptReflectionWrite(arrIdents.set, idxWrite, resultIdents.set);
    }
    stmts.push_back(checker_->AllocNode<ir::ExpressionStatement>(setCall));
    stmts.push_back(checker_->AllocNode<ir::ReturnStatement>(resultIdents.ret));
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
