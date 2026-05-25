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

#include "iterableSpreadLowering.h"

#include <sstream>
#include <vector>

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/util.h"
#include "generated/signatures.h"
#include "ir/astNode.h"
#include "ir/base/spreadElement.h"
#include "ir/expression.h"
#include "ir/expressions/identifier.h"
#include "ir/opaqueTypeNode.h"
#include "ir/ts/tsAsExpression.h"

namespace ark::es2panda::compiler {

static ir::OpaqueTypeNode *CreateOpaqueTypeNode(public_lib::Context *ctx, checker::Type *type)
{
    return ctx->AllocNode<ir::OpaqueTypeNode>(type, ctx->allocator);
}

ir::Identifier *CreateSpreadTempResizableArray(public_lib::Context *ctx, checker::Type *elementType,
                                               ArenaVector<ir::Statement *> &statements)
{
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const tempArrayIdent = Gensym(allocator);
    auto *const tempArrayType = checker->CreateETSResizableArrayType(checker->GetNonConstantType(elementType));
    tempArrayIdent->SetTsType(tempArrayType);
    statements.emplace_back(parser->CreateFormattedStatement(
        "let @@I1: Array<@@T2> = @@E3;", tempArrayIdent, CreateOpaqueTypeNode(ctx, elementType),
        CreateUninitializedResizableArray(ctx, parser->CreateFormattedExpression("0"), tempArrayType)));
    return tempArrayIdent;
}

static ir::Statement *CreateCopyResizableArrayToFixedArrayStatement(public_lib::Context *ctx,
                                                                    ir::Identifier *sourceIdent,
                                                                    ir::Identifier *targetIdent)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const indexIdent = Gensym(allocator);
    return parser->CreateFormattedStatement(
        "for (let @@I1: int = 0; @@I2 < @@I3.length; @@I4 = @@I5 + 1) {"
        "@@I6[@@I7] = @@I8[@@I9];"
        "}",
        indexIdent, indexIdent->CloneReference(allocator, nullptr), sourceIdent->CloneReference(allocator, nullptr),
        indexIdent->CloneReference(allocator, nullptr), indexIdent->CloneReference(allocator, nullptr),
        targetIdent->CloneReference(allocator, nullptr), indexIdent->CloneReference(allocator, nullptr),
        sourceIdent->CloneReference(allocator, nullptr), indexIdent->CloneReference(allocator, nullptr));
}

ir::Identifier *FinalizeSpreadTempArray(public_lib::Context *ctx, checker::Type *arrayType,
                                        ir::Identifier *tempArrayIdent, ArenaVector<ir::Statement *> &statements)
{
    if (!arrayType->IsETSArrayType()) {
        return tempArrayIdent;
    }

    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();

    auto *const fixedArrayLengthIdent = Gensym(allocator);
    fixedArrayLengthIdent->SetTsType(checker->GlobalIntType());
    statements.emplace_back(parser->CreateFormattedStatement("let @@I1: int = @@I2.length;", fixedArrayLengthIdent,
                                                             tempArrayIdent->CloneReference(allocator, nullptr)));

    auto *const fixedArrayIdent = Gensym(allocator);
    fixedArrayIdent->SetTsType(arrayType);
    statements.emplace_back(parser->CreateFormattedStatement(
        "let @@I1 = @@E2;", fixedArrayIdent,
        CreateUninitializedFixedArray(ctx, fixedArrayLengthIdent->CloneReference(allocator, nullptr), arrayType)));
    statements.emplace_back(CreateCopyResizableArrayToFixedArrayStatement(ctx, tempArrayIdent, fixedArrayIdent));
    return fixedArrayIdent;
}

static checker::Type *GetVariableType(ir::Expression *expression)
{
    if (!expression->IsIdentifier() || expression->AsIdentifier()->Variable() == nullptr) {
        return nullptr;
    }

    return expression->AsIdentifier()->Variable()->TsType();
}

static checker::Type *GetSpreadType(ir::SpreadElement *spreadElement)
{
    if (spreadElement->GetResolvedSpreadSourceType() != nullptr) {
        return spreadElement->GetResolvedSpreadSourceType();
    }

    return spreadElement->TsType() != nullptr ? spreadElement->TsType() : spreadElement->Argument()->TsType();
}

ir::Expression *CloneSpreadArgumentWithSmartType(public_lib::Context *ctx, ir::SpreadElement *spreadElement)
{
    auto *const allocator = ctx->allocator;
    auto *const spreadArgument = spreadElement->Argument();
    auto *const initExpr = spreadArgument->Clone(allocator, nullptr)->AsExpression();
    auto *const smartType = GetSpreadType(spreadElement);
    if (smartType == nullptr || !spreadArgument->IsIdentifier() ||
        spreadArgument->AsIdentifier()->Variable() == nullptr ||
        spreadArgument->AsIdentifier()->Variable()->TsType() == smartType) {
        return initExpr;
    }

    auto *const typeNode = ctx->AllocNode<ir::OpaqueTypeNode>(smartType, allocator);
    return ctx->AllocNode<ir::TSAsExpression>(initExpr, typeNode, false);
}

class IterableSpreadAppendBuilder final {
public:
    IterableSpreadAppendBuilder(public_lib::Context *ctx, ir::SpreadElement *spreadElement,
                                ir::Identifier *targetArrayIdent, checker::Type *targetElementType)
        : checker_(ctx->GetChecker()->AsETSChecker()),
          allocator_(ctx->allocator),
          parser_(ctx->parser->AsETSParser()),
          spreadArgument_(spreadElement->Argument()),
          targetArrayIdent_(targetArrayIdent),
          spreadType_(GetSpreadType(spreadElement)),
          normalizedSpreadType_(checker_->NormalizeSpreadType(spreadType_)),
          spreadElementType_(spreadElement->GetResolvedSpreadElementType()),
          targetElementType_(targetElementType),
          spreadArgIdent_(Gensym(allocator_))
    {
    }

    void AppendTo(ArenaVector<ir::Statement *> &statements)
    {
        ES2PANDA_ASSERT(spreadElementType_ != nullptr);
        ES2PANDA_ASSERT(!spreadElementType_->IsTypeError());
        ES2PANDA_ASSERT(targetElementType_ != nullptr);
        ES2PANDA_ASSERT(!targetElementType_->IsTypeError());

        spreadArgIdent_->SetTsType(spreadType_);

        AppendArgumentEvaluation();
        if (normalizedSpreadType_->IsETSUnionType()) {
            AppendUnionSpread();
        } else {
            AppendIteratorLoop();
        }

        auto generatedStatements = parser_->CreateFormattedStatements(ss_.str(), nodesWaitingInsert_);
        statements.insert(statements.cend(), generatedStatements.cbegin(), generatedStatements.cend());
    }

private:
    auto AddIdentDeclaration(ir::Identifier *ident) -> std::string
    {
        nodesWaitingInsert_.emplace_back(ident);
        return "@@I" + std::to_string(nodesWaitingInsert_.size());
    }

    auto AddIdentReference(ir::Identifier *ident) -> std::string
    {
        nodesWaitingInsert_.emplace_back(ident->CloneReference(allocator_, nullptr));
        return "@@I" + std::to_string(nodesWaitingInsert_.size());
    }

    auto AddExpr(ir::Expression *expr) -> std::string
    {
        nodesWaitingInsert_.emplace_back(expr);
        return "@@E" + std::to_string(nodesWaitingInsert_.size());
    }

    auto AddType(checker::Type *type) -> std::string
    {
        nodesWaitingInsert_.emplace_back(checker_->AllocNode<ir::OpaqueTypeNode>(type, allocator_));
        return "@@T" + std::to_string(nodesWaitingInsert_.size());
    }

    void AppendArgumentEvaluation()
    {
        ss_ << "let " << AddIdentDeclaration(spreadArgIdent_) << ": " << AddType(spreadType_) << " = "
            << AddExpr(spreadArgument_);
        auto *const variableType = GetVariableType(spreadArgument_);
        if (variableType != nullptr && variableType != spreadType_) {
            ss_ << " as " << AddType(spreadType_);
        }
        ss_ << ";";
    }

    void AppendIteratorLoop()
    {
        AppendIteratorLoop(AddIdentReference(spreadArgIdent_));
    }

    void AppendIteratorLoop(const std::string &iterableExpr)
    {
        auto *const iteratorIdent = Gensym(allocator_);
        auto *const iteratorResultIdent = Gensym(allocator_);
        ss_ << "let " << AddIdentDeclaration(iteratorIdent) << " = " << iterableExpr << "."
            << compiler::Signatures::ITERATOR_METHOD << "();";
        ss_ << "while (true) {";
        ss_ << "let " << AddIdentDeclaration(iteratorResultIdent) << " = " << AddIdentReference(iteratorIdent)
            << ".next();";
        ss_ << "if (" << AddIdentReference(iteratorResultIdent) << ".done) {break;}";
        ss_ << AddIdentReference(targetArrayIdent_) << ".push(" << AddIdentReference(iteratorResultIdent)
            << ".value as " << AddType(targetElementType_) << ");";
        ss_ << "}";
    }

    void AppendIteratorFallback(bool hasConditionalBranch)
    {
        if (hasConditionalBranch) {
            ss_ << " else {";
        }

        AppendIteratorLoop("(" + AddIdentReference(spreadArgIdent_) + " as Iterable<" + AddType(spreadElementType_) +
                           ">)");

        if (hasConditionalBranch) {
            ss_ << "}";
        }
    }

    void AppendArrayLoop(checker::Type *arrayType)
    {
        auto *const sourceIdent = Gensym(allocator_);
        auto *const indexIdent = Gensym(allocator_);
        sourceIdent->SetTsType(arrayType);
        indexIdent->SetTsType(checker_->GlobalIntType());

        ss_ << "let " << AddIdentDeclaration(sourceIdent) << ": " << AddType(arrayType) << " = "
            << AddIdentReference(spreadArgIdent_) << " as " << AddType(arrayType) << ";";
        ss_ << "for (let " << AddIdentDeclaration(indexIdent) << ": int = 0; " << AddIdentReference(indexIdent) << " < "
            << AddIdentReference(sourceIdent) << ".length; " << AddIdentReference(indexIdent) << " = "
            << AddIdentReference(indexIdent) << " + 1) {";
        ss_ << AddIdentReference(targetArrayIdent_) << ".push(" << AddIdentReference(sourceIdent) << "["
            << AddIdentReference(indexIdent) << "] as " << AddType(targetElementType_) << ");";
        ss_ << "}";
    }

    void AppendArrayBranch(checker::Type *arrayType, bool &hasConditionalBranch)
    {
        ss_ << (hasConditionalBranch ? " else if (" : "if (") << AddIdentReference(spreadArgIdent_) << " instanceof "
            << AddType(arrayType) << ") {";
        AppendArrayLoop(arrayType);
        ss_ << "}";
        hasConditionalBranch = true;
    }

    void AppendFinalArrayBranch(checker::Type *arrayType, bool hasConditionalBranch)
    {
        if (hasConditionalBranch) {
            ss_ << " else {";
        }

        AppendArrayLoop(arrayType);

        if (hasConditionalBranch) {
            ss_ << "}";
        }
    }

    checker::Type *CreateFixedArrayViewType(const std::vector<checker::Type *> &arrayTypes)
    {
        ES2PANDA_ASSERT(!arrayTypes.empty());

        std::vector<checker::Type *> elementTypes;
        elementTypes.reserve(arrayTypes.size());
        for (auto *const arrayType : arrayTypes) {
            elementTypes.emplace_back(checker_->GetNonConstantType(checker_->GetElementTypeOfArray(arrayType)));
        }

        checker::Type *elementType = elementTypes.front();
        if (elementTypes.size() > 1U) {
            auto *const unionType = checker_->CreateETSUnionType(std::move(elementTypes));
            elementType = unionType->IsETSUnionType() ? unionType->AsETSUnionType()->NormalizedType() : unionType;
        }

        return checker_->CreateETSArrayType(checker_->GetNonConstantType(elementType), false);
    }

    void AddUnionConstituentToBuckets(std::vector<checker::Type *> &fixedArrayTypes,
                                      std::vector<checker::Type *> &valueArrayTypes, bool &needsIteratorFallback,
                                      checker::Type *constituentType)
    {
        auto *const normalizedConstituent = checker_->NormalizeSpreadType(constituentType);
        if (normalizedConstituent->IsETSArrayType()) {
            if (normalizedConstituent->AsETSArrayType()->IsValueArray()) {
                valueArrayTypes.emplace_back(normalizedConstituent);
            } else {
                fixedArrayTypes.emplace_back(normalizedConstituent);
            }
            return;
        }
        if (normalizedConstituent->IsETSStringType() || normalizedConstituent->IsETSResizableArrayType() ||
            normalizedConstituent->IsETSReadonlyArrayType()) {
            needsIteratorFallback = true;
            return;
        }
        if (normalizedConstituent->IsETSObjectType() && !normalizedConstituent->IsETSStringType()) {
            ES2PANDA_ASSERT(checker_->HasStandardLibraryIterableInterface(normalizedConstituent->AsETSObjectType()));
            needsIteratorFallback = true;
            return;
        }

        needsIteratorFallback = true;
    }

    void CollectUnionSpreadBuckets(std::vector<checker::Type *> &fixedArrayTypes,
                                   std::vector<checker::Type *> &valueArrayTypes, bool &needsIteratorFallback)
    {
        for (auto *const constituentType : normalizedSpreadType_->AsETSUnionType()->ConstituentTypes()) {
            AddUnionConstituentToBuckets(fixedArrayTypes, valueArrayTypes, needsIteratorFallback, constituentType);
        }

        for (auto *&arrayType : fixedArrayTypes) {
            // FixedArray<T> type is not preserved up to undefined - we have to use 'instanceof FixedArray<T|undefined>`
            // instead
            auto *elementType = arrayType->AsETSArrayType()->ElementType();
            if (!elementType->PossiblyETSUndefined()) {
                elementType = checker_->CreateETSUnionType({elementType, checker_->GlobalETSUndefinedType()});
                arrayType = checker_->CreateETSArrayType(elementType);
            }
        }
    }

    bool AppendFixedArrayUnionBranch(const std::vector<checker::Type *> &fixedArrayTypes, bool needsIteratorFallback,
                                     bool hasValueArrayTypes, bool &hasConditionalBranch)
    {
        if (fixedArrayTypes.empty()) {
            return false;
        }

        auto *const fixedArrayType = CreateFixedArrayViewType(fixedArrayTypes);
        if (!needsIteratorFallback && !hasValueArrayTypes) {
            AppendArrayLoop(fixedArrayType);
            return true;
        }
        AppendArrayBranch(fixedArrayType, hasConditionalBranch);
        return false;
    }

    void AppendValueArrayUnionBranches(const std::vector<checker::Type *> &valueArrayTypes, bool needsIteratorFallback,
                                       bool &hasConditionalBranch)
    {
        for (size_t index = 0; index < valueArrayTypes.size(); ++index) {
            if (!needsIteratorFallback && index + 1U == valueArrayTypes.size()) {
                AppendFinalArrayBranch(valueArrayTypes[index], hasConditionalBranch);
                continue;
            }
            AppendArrayBranch(valueArrayTypes[index], hasConditionalBranch);
        }
    }

    void AppendUnionSpread()
    {
        std::vector<checker::Type *> fixedArrayTypes;
        std::vector<checker::Type *> valueArrayTypes;
        bool needsIteratorFallback = false;
        CollectUnionSpreadBuckets(fixedArrayTypes, valueArrayTypes, needsIteratorFallback);

        bool hasConditionalBranch = false;
        if (AppendFixedArrayUnionBranch(fixedArrayTypes, needsIteratorFallback, !valueArrayTypes.empty(),
                                        hasConditionalBranch)) {
            return;
        }
        AppendValueArrayUnionBranches(valueArrayTypes, needsIteratorFallback, hasConditionalBranch);
        if (needsIteratorFallback) {
            AppendIteratorFallback(hasConditionalBranch);
        }
    }

    checker::ETSChecker *checker_;
    ArenaAllocator *allocator_;
    parser::ETSParser *parser_;
    ir::Expression *spreadArgument_;
    ir::Identifier *targetArrayIdent_;
    checker::Type *spreadType_;
    checker::Type *normalizedSpreadType_;
    checker::Type *spreadElementType_;
    checker::Type *targetElementType_;
    ir::Identifier *spreadArgIdent_;
    std::vector<ir::AstNode *> nodesWaitingInsert_;
    std::stringstream ss_;
};

void AppendIterableSpreadToArray(public_lib::Context *ctx, ir::SpreadElement *spreadElement,
                                 ir::Identifier *targetArrayIdent, checker::Type *targetElementType,
                                 ArenaVector<ir::Statement *> &statements)
{
    IterableSpreadAppendBuilder(ctx, spreadElement, targetArrayIdent, targetElementType).AppendTo(statements);
}

static void AppendTupleSpreadToArray(public_lib::Context *ctx, ir::SpreadElement *spreadElement,
                                     ir::Identifier *targetArrayIdent, checker::Type *elementType,
                                     ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const tupleIdent = Gensym(allocator);
    auto *const tupleType =
        ctx->GetChecker()->AsETSChecker()->NormalizeSpreadType(GetSpreadType(spreadElement))->AsETSTupleType();
    tupleIdent->SetTsType(tupleType);
    statements.emplace_back(parser->CreateFormattedStatement("let @@I1 = @@E2;", tupleIdent,
                                                             CloneSpreadArgumentWithSmartType(ctx, spreadElement)));
    for (size_t index = 0; index < tupleType->GetTupleSize(); ++index) {
        statements.emplace_back(parser->CreateFormattedStatement(
            "@@I1.push(@@I2[" + std::to_string(index) + "] as @@T3);",
            targetArrayIdent->CloneReference(allocator, nullptr), tupleIdent->CloneReference(allocator, nullptr),
            CreateOpaqueTypeNode(ctx, elementType)));
    }
}

static void AppendIndexableSpreadToArray(public_lib::Context *ctx, ir::SpreadElement *spreadElement,
                                         ir::Identifier *targetArrayIdent, checker::Type *elementType,
                                         ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const arrayIdent = Gensym(allocator);
    auto *const arrayIndexIdent = Gensym(allocator);
    arrayIdent->SetTsType(ctx->GetChecker()->AsETSChecker()->NormalizeSpreadType(GetSpreadType(spreadElement)));
    statements.emplace_back(parser->CreateFormattedStatement("let @@I1 = @@E2;", arrayIdent,
                                                             CloneSpreadArgumentWithSmartType(ctx, spreadElement)));
    statements.emplace_back(parser->CreateFormattedStatement(
        "for (let @@I1: int = 0; @@I2 < @@I3.length; @@I4 = @@I5 + 1) {"
        "@@I6.push(@@I7[@@I8] as @@T9);"
        "}",
        arrayIndexIdent, arrayIndexIdent->CloneReference(allocator, nullptr),
        arrayIdent->CloneReference(allocator, nullptr), arrayIndexIdent->CloneReference(allocator, nullptr),
        arrayIndexIdent->CloneReference(allocator, nullptr), targetArrayIdent->CloneReference(allocator, nullptr),
        arrayIdent->CloneReference(allocator, nullptr), arrayIndexIdent->CloneReference(allocator, nullptr),
        CreateOpaqueTypeNode(ctx, elementType)));
}

void AppendSpreadToArray(public_lib::Context *ctx, ir::SpreadElement *spreadElement, ir::Identifier *targetArrayIdent,
                         checker::Type *elementType, ArenaVector<ir::Statement *> &statements)
{
    switch (spreadElement->GetResolvedSpreadKind()) {
        case ir::SpreadElement::ResolvedSpreadKind::ITERABLE: {
            AppendIterableSpreadToArray(ctx, spreadElement, targetArrayIdent, elementType, statements);
            return;
        }
        case ir::SpreadElement::ResolvedSpreadKind::TUPLE: {
            AppendTupleSpreadToArray(ctx, spreadElement, targetArrayIdent, elementType, statements);
            return;
        }
        case ir::SpreadElement::ResolvedSpreadKind::INDEXABLE: {
            AppendIndexableSpreadToArray(ctx, spreadElement, targetArrayIdent, elementType, statements);
            return;
        }
        case ir::SpreadElement::ResolvedSpreadKind::INVALID: {
            ES2PANDA_UNREACHABLE();
        }
    }

    ES2PANDA_UNREACHABLE();
}

}  // namespace ark::es2panda::compiler
