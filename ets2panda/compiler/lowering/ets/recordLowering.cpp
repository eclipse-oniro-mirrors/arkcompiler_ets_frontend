/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "recordLowering.h"

#include <cmath>
#include <limits>

#include "checker/ETSchecker.h"

#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

using KeyType = std::variant<int32_t, int64_t, float, double, util::StringView>;
using KeySetType = std::unordered_set<KeyType>;

static bool IsIntegralKey(KeyType const &key)
{
    return std::holds_alternative<int32_t>(key) || std::holds_alternative<int64_t>(key);
}

static bool IsFloatingKey(KeyType const &key)
{
    return std::holds_alternative<float>(key) || std::holds_alternative<double>(key);
}

static int64_t ToInt64(KeyType const &key)
{
    if (std::holds_alternative<int32_t>(key)) {
        return std::get<int32_t>(key);
    }
    return std::get<int64_t>(key);
}

static double ToDouble(KeyType const &key)
{
    if (std::holds_alternative<float>(key)) {
        return std::get<float>(key);
    }
    if (std::holds_alternative<double>(key)) {
        return std::get<double>(key);
    }
    if (std::holds_alternative<int32_t>(key)) {
        return static_cast<double>(std::get<int32_t>(key));
    }
    return static_cast<double>(std::get<int64_t>(key));
}

static bool NumericKeysEqual(KeyType const &lhs, KeyType const &rhs)
{
    if (IsIntegralKey(lhs) && IsIntegralKey(rhs)) {
        return ToInt64(lhs) == ToInt64(rhs);
    }

    if (IsFloatingKey(lhs) && IsFloatingKey(rhs)) {
        return ToDouble(lhs) == ToDouble(rhs);
    }

    auto const intVal = IsIntegralKey(lhs) ? ToInt64(lhs) : ToInt64(rhs);
    auto const floatVal = IsFloatingKey(lhs) ? ToDouble(lhs) : ToDouble(rhs);
    if (!std::isfinite(floatVal) || std::trunc(floatVal) != floatVal) {
        return false;
    }

    constexpr auto minInt64 = static_cast<double>(std::numeric_limits<int64_t>::min());
    constexpr auto maxInt64 = static_cast<double>(std::numeric_limits<int64_t>::max());
    if (floatVal < minInt64 || floatVal > maxInt64) {
        return false;
    }

    return static_cast<int64_t>(floatVal) == intVal;
}

static bool AreEquivalentKeys(KeyType const &lhs, KeyType const &rhs)
{
    if (std::holds_alternative<util::StringView>(lhs) || std::holds_alternative<util::StringView>(rhs)) {
        return lhs == rhs;
    }

    return NumericKeysEqual(lhs, rhs);
}

static bool ContainsEquivalentKey(KeySetType const &keySet, KeyType const &key)
{
    return std::any_of(keySet.cbegin(), keySet.cend(),
                       [&key](auto const &existing) { return AreEquivalentKeys(existing, key); });
}

static void AddKeyOrReportCollision(KeySetType &keySet, KeyType const &key, ir::ObjectExpression *expr,
                                    public_lib::Context *ctx)
{
    if (ContainsEquivalentKey(keySet, key)) {
        ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::OBJ_LIT_PROP_NAME_COLLISION, {}, expr->Start());
        return;
    }

    keySet.insert(key);
}

static KeyType TypeToKey(checker::Type *type)
{
    if (type->IsByteType()) {
        return static_cast<double>(type->AsByteType()->GetValue());
    }
    if (type->IsShortType()) {
        return static_cast<double>(type->AsShortType()->GetValue());
    }
    if (type->IsIntType()) {
        return static_cast<double>(type->AsIntType()->GetValue());
    }
    if (type->IsLongType()) {
        return static_cast<double>(type->AsLongType()->GetValue());
    }
    if (type->IsFloatType()) {
        return static_cast<double>(type->AsFloatType()->GetValue());
    }
    if (type->IsDoubleType()) {
        return type->AsDoubleType()->GetValue();
    }
    if (type->IsETSStringType()) {
        return type->AsETSStringType()->GetValue();
    }
    ES2PANDA_UNREACHABLE();
    return {};
}

static std::optional<util::StringView> ComputeMemberExpressionKey(ir::Expression *key, public_lib::Context *ctx)
{
    auto *memberExpr = key->AsMemberExpression();
    util::UString stableKey(key->TsType()->AsETSObjectType()->Name(), ctx->Allocator());
    stableKey.Append('.');
    stableKey.Append(memberExpr->Property()->AsIdentifier()->Name());
    return stableKey.View();
}

static ir::Expression *UpdateObjectExpression(ir::ObjectExpression *expr, public_lib::Context *ctx);

bool RecordLowering::PerformForProgram(parser::Program *program)
{
    // Replace Record Object Expressions with Block Expressions
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *ast) -> ir::AstNode * {
            if (ast->IsObjectExpression()) {
                return UpdateObjectExpression(ast->AsObjectExpression(), ctx);
            }

            return ast;
        },
        Name());

    return true;
}

static void CheckDuplicateKey(KeySetType &keySet, ir::ObjectExpression *expr, public_lib::Context *ctx)
{
    for (auto *it : expr->Properties()) {
        if (it->IsSpreadElement()) {
            // Skip spread elements - they are handled separately
            continue;
        }

        auto *prop = it->AsProperty();
        switch (prop->Key()->Type()) {
            case ir::AstNodeType::NUMBER_LITERAL: {
                auto number = prop->Key()->AsNumberLiteral()->Number();
                if (number.IsInt()) {
                    AddKeyOrReportCollision(keySet, number.GetInt(), expr, ctx);
                } else if (number.IsLong()) {
                    AddKeyOrReportCollision(keySet, number.GetLong(), expr, ctx);
                } else if (number.IsFloat()) {
                    AddKeyOrReportCollision(keySet, number.GetFloat(), expr, ctx);
                } else if (number.IsDouble()) {
                    AddKeyOrReportCollision(keySet, number.GetDouble(), expr, ctx);
                }
                break;
            }
            case ir::AstNodeType::STRING_LITERAL: {
                AddKeyOrReportCollision(keySet, prop->Key()->AsStringLiteral()->Str(), expr, ctx);
                break;
            }
            case ir::AstNodeType::MEMBER_EXPRESSION: {
                auto enumKey = ComputeMemberExpressionKey(prop->Key(), ctx);
                if (enumKey.has_value() && keySet.insert(enumKey.value()).second) {
                    continue;
                }
                ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::OBJ_LIT_UNKNOWN_PROP, {}, expr->Start());
                break;
            }
            default: {
                ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::OBJ_LIT_UNKNOWN_PROP, {}, expr->Start());
                break;
            }
        }
    }
}

static void CheckLiteralsCompleteness(KeySetType &keySet, ir::ObjectExpression *expr, public_lib::Context *ctx)
{
    auto *keyType = expr->TsType()->AsETSObjectType()->TypeArguments().front();
    if (!keyType->IsETSUnionType()) {
        return;
    }
    for (auto &ct : keyType->AsETSUnionType()->ConstituentTypes()) {
        if (ct->IsConstantType() && !ContainsEquivalentKey(keySet, TypeToKey(ct))) {
            ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::OBJ_LIT_NOT_COVERING_UNION, {}, expr->Start());
        }
    }
}

static void CheckKeyType(checker::ETSChecker *checker, checker::Type const *const keyType,
                         ir::ObjectExpression const *const expr, public_lib::Context *ctx)
{
    if (keyType->IsETSObjectType()) {
        if (keyType->IsETSStringType() || keyType->IsBuiltinNumeric() ||
            checker->Relation()->IsIdenticalTo(keyType, checker->GetGlobalTypesHolder()->GlobalNumericBuiltinType()) ||
            checker->Relation()->IsIdenticalTo(keyType, checker->GetGlobalTypesHolder()->GlobalIntegralBuiltinType()) ||
            keyType->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::ENUM_OBJECT)) {
            return;
        }
    }
    ctx->GetChecker()->AsETSChecker()->LogError(diagnostic::OBJ_LIT_UNKNOWN_PROP, {}, expr->Start());
}

static ir::Expression *CreateBlockExpression(ir::ObjectExpression *expr, checker::Type *keyType,
                                             checker::Type *valueType, public_lib::Context *ctx);

static ir::Expression *UpdateObjectExpression(ir::ObjectExpression *expr, public_lib::Context *ctx)
{
    auto checker = ctx->GetChecker()->AsETSChecker();

    if (!expr->TsType()->IsETSObjectType()) {
        // Unexpected preferred type
        return expr;
    }

    // Check if this is actually a Record or Map type using proper type identity checking
    auto *objType = expr->TsType()->AsETSObjectType();
    auto *originalBaseType = objType->GetOriginalBaseType();
    auto *globalTypes = checker->GetGlobalTypesHolder();

    if (!checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalMapBuiltinType()) &&
        !checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalRecordBuiltinType())) {
        // Only update object expressions for Map/Record types
        return expr;
    }

    // Access type arguments
    [[maybe_unused]] size_t constexpr numArguments = 2;
    auto const &typeArguments = expr->TsType()->AsETSObjectType()->TypeArguments();
    ES2PANDA_ASSERT(typeArguments.size() == numArguments);

    auto const *keyType = typeArguments[0];
    if (keyType->IsETSTypeParameter()) {
        keyType = keyType->AsETSTypeParameter()->GetConstraintType();
    }

    // check keys correctness
    if (keyType->IsETSUnionType()) {
        for (auto const *const ct : keyType->AsETSUnionType()->ConstituentTypes()) {
            CheckKeyType(checker, ct, expr, ctx);
        }
    } else {
        CheckKeyType(checker, keyType, expr, ctx);
    }

    KeySetType keySet;
    CheckDuplicateKey(keySet, expr, ctx);
    CheckLiteralsCompleteness(keySet, expr, ctx);

    auto *const scope = NearestScope(expr);
    checker::SavedCheckerContext scc {checker, checker::CheckerStatus::IGNORE_VISIBILITY};
    auto expressionCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), scope);

    // Create Block Expression
    auto block = CreateBlockExpression(expr, typeArguments[0], typeArguments[1], ctx);
    ES2PANDA_ASSERT(block != nullptr);
    block->SetParent(expr->Parent());

    // Run checks
    InitScopesPhaseETS::RunExternalNode(block, ctx->GetChecker()->VarBinder());
    checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(block, NearestScope(block));
    block->Check(checker);

    // Replace Object Expression with Block Expression
    return block;
}

static std::string TypeToString(checker::Type *type)
{
    std::stringstream ss;
    type->ToString(ss);
    return ss.str();
}

static ir::Expression *CreateBlockExpression(ir::ObjectExpression *expr, checker::Type *keyType,
                                             checker::Type *valueType, public_lib::Context *ctx)
{
    /* This function will create block expression in the following format
     *
     * let map = new Map<key_type, value_type>();
     * map.set(k1, v1)
     * map.set(k2, v2)
     * ...
     * // For spread elements:
     * let spread_src_ = spread_expr;
     * spread_src_.forEach((value, key) => { map.set(key, value); });
     * map
     */

    auto *allocator = ctx->Allocator();
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    // Initialize map with provided type arguments
    auto *ident = Gensym(ctx->Allocator());

    // Determine container type using proper type checking
    auto *objType = expr->TsType()->AsETSObjectType();
    auto *originalBaseType = objType->GetOriginalBaseType();
    auto *globalTypes = checker->GetGlobalTypesHolder();

    std::string containerType;
    if (checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalMapBuiltinType())) {
        containerType = "Map";
    } else {
        containerType = "Record";
    }

    ArenaVector<ir::Statement *> statements(ctx->allocator->Adapter());
    auto &properties = expr->Properties();

    const std::string createSrc =
        "let @@I1 = new " + containerType + "<" + TypeToString(keyType) + "," + "@@T2" + ">()";
    statements.push_back(ctx->parser->AsETSParser()->CreateFormattedStatements(createSrc, ident, valueType).front());

    // Build statements from properties
    for (const auto &property : properties) {
        if (property->IsSpreadElement()) {
            auto *spreadArg = property->AsSpreadElement()->Argument();
            const auto tempSource = Gensym(allocator);

            statements.push_back(parser->CreateFormattedStatement("let @@I1 = @@E2;", tempSource, spreadArg));

            std::vector<ir::AstNode *> forEachArgs;
            std::stringstream forEachStream;

            forEachStream << "@@I1.forEach((value, key) => { @@I2.set(key, value); });";
            forEachArgs.push_back(tempSource->Clone(allocator, nullptr));
            forEachArgs.push_back(ident->Clone(allocator, nullptr));

            statements.push_back(parser->CreateFormattedStatement(forEachStream.str(), forEachArgs));
        } else {
            // Handle regular properties
            statements.push_back(
                parser->CreateFormattedStatement("@@I1.set(@@E2, @@E3)", ident->Clone(ctx->allocator, nullptr),
                                                 property->AsProperty()->Key(), property->AsProperty()->Value()));
        }
    }
    statements.push_back(parser->CreateFormattedStatement("@@I1", ident->Clone(ctx->allocator, nullptr)));

    // Create Block Expression
    auto block = ctx->AllocNode<ir::BlockExpression>(std::move(statements));
    return block;
}

}  // namespace ark::es2panda::compiler
