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
#include <gtest/gtest.h>

#include "plugin_conversion_rule_unit_test.h"

namespace ark::es2panda {
/* virtual void Identical(TypeRelation *relation, Type *other) */
TEST_F(PluginConversionRuleUnitTest, CheckerTypeRelationInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void TypeIdentical([[maybe_unused]] es2panda_Context *context, es2panda_Type *classInstance,
[[maybe_unused]] es2panda_TypeRelation *relation, [[maybe_unused]] es2panda_Type *other/*return_args:*/)
    {
        auto *relationE2p = reinterpret_cast<checker::TypeRelation *>(relation);
        auto *otherE2p = reinterpret_cast<checker::Type *>(other);
        ((reinterpret_cast<checker::Type *>(classInstance))->Identical(relationE2p, otherE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* bool IsIdenticalTo(Type const *source, Type const *target) */
TEST_F(PluginConversionRuleUnitTest, CheckerTypeRelationMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" bool TypeRelationIsIdenticalTo([[maybe_unused]] es2panda_Context *context,
es2panda_TypeRelation *classInstance, [[maybe_unused]] es2panda_Type *source,
[[maybe_unused]] es2panda_Type *target/*return_args:*/)
    {
        auto *sourceE2p = reinterpret_cast<checker::Type *>(source);
        auto *targetE2p = reinterpret_cast<checker::Type *>(target);
        auto apiRes = ((reinterpret_cast<checker::TypeRelation *>(classInstance))
            ->IsIdenticalTo(sourceE2p, targetE2p));
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit TypeRelation(Checker *checker) */
TEST_F(PluginConversionRuleUnitTest, CheckerTypeRelationConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_TypeRelation *CreateTypeRelation([[maybe_unused]] es2panda_Context *context)
    {
        auto *checkerE2p = reinterpret_cast<Context *>(context)->GetChecker();
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_TypeRelation *>(ctxAllocator->New<checker::TypeRelation>(checkerE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* bool IsIdenticalTo(IndexInfo *source, IndexInfo *target) */
TEST_F(PluginConversionRuleUnitTest, CheckerIndexInfoPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" bool TypeRelationIsIdenticalTo2([[maybe_unused]] es2panda_Context *context,
es2panda_TypeRelation *classInstance, [[maybe_unused]] es2panda_IndexInfo *source,
[[maybe_unused]] es2panda_IndexInfo *target/*return_args:*/)
    {
        auto *sourceE2p = reinterpret_cast<checker::IndexInfo *>(source);
        auto *targetE2p = reinterpret_cast<checker::IndexInfo *>(target);
        auto apiRes = ((reinterpret_cast<checker::TypeRelation *>(classInstance))
                ->IsIdenticalTo(sourceE2p, targetE2p));
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* virtual const IndexInfo *NumberIndexInfo() const */
TEST_F(PluginConversionRuleUnitTest, CheckerIndexInfoPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_IndexInfo *ObjectTypeNumberIndexInfoConst([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_IndexInfo *>
((reinterpret_cast<const checker::ObjectType *>(classInstance))->NumberIndexInfo());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* Type *GlobalNullType() */
TEST_F(PluginConversionRuleUnitTest, CheckerGlobalTypesHolderPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_Type *GlobalTypesHolderGlobalNullType([[maybe_unused]] es2panda_Context *context,
es2panda_GlobalTypesHolder *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<es2panda_Type *>
    ((reinterpret_cast<checker::GlobalTypesHolder *>(classInstance))->GlobalNullType());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] Type *GlobalClassCastErrorBuiltinType() const noexcept */
TEST_F(PluginConversionRuleUnitTest, CheckerGlobalTypesHolderPtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_Type *GlobalTypesHolderGlobalClassCastErrorBuiltinTypeConst([[maybe_unused]]
es2panda_Context *context, es2panda_GlobalTypesHolder *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast< es2panda_Type *>((
    reinterpret_cast<const checker::GlobalTypesHolder *>(classInstance))->GlobalClassCastErrorBuiltinType());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit GlobalTypesHolder(ArenaAllocator *allocator) */
TEST_F(PluginConversionRuleUnitTest, CheckerGlobalTypesHolderConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_GlobalTypesHolder *CreateGlobalTypesHolder([[maybe_unused]] es2panda_Context *context)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_GlobalTypesHolder *>
    (ctxAllocator->New<checker::GlobalTypesHolder>(allocatorE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ObjectDescriptor *Desc() */
TEST_F(PluginConversionRuleUnitTest, CheckerObjectDescriptorPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_ObjectDescriptor *ObjectTypeDesc([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast< es2panda_ObjectDescriptor *>
    ((reinterpret_cast<checker::ObjectType *>(classInstance))->Desc());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit IntType(UType value) */
TEST_F(PluginConversionRuleUnitTest, CheckerUTypeInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_Type *CreateIntType1([[maybe_unused]] es2panda_Context *context,
[[maybe_unused]] int32_t value)
    {
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_Type *>(ctxAllocator->New<checker::IntType>(value));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* UType GetValue() const */
TEST_F(PluginConversionRuleUnitTest, CheckerUTypeReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" int32_t IntTypeGetValueConst([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance/*return_args:*/)
    {
        auto apiRes = ((reinterpret_cast<const checker::IntType *>(classInstance))->GetValue());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit ETSTuple(ArenaAllocator *const allocator, const TupleSizeType size) */
TEST_F(PluginConversionRuleUnitTest, CheckerTupleSizeTypeInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateETSTuple1([[maybe_unused]] es2panda_Context *context,
[[maybe_unused]] size_t size)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::ETSTuple>(allocatorE2p, size));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] TupleSizeType GetTupleSize() const */
TEST_F(PluginConversionRuleUnitTest, CheckerTupleSizeTypeReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" size_t ETSTupleGetTupleSizeConst([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/)
    {
        auto apiRes = ((reinterpret_cast<const ir::ETSTuple *>(classInstance))->GetTupleSize());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* void SetVariable(varbinder::Variable *variable) noexcept */
TEST_F(PluginConversionRuleUnitTest, VariablePtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeSetVariable([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_Variable *variable/*return_args:*/)
    {
    {
        auto *prog = const_cast<parser::Program *>(reinterpret_cast<ir::AstNode *>(classInstance)->Program());
        if (prog != nullptr) {
            prog->SetProgramModified(true);
        }
    }
        auto *variableE2p = reinterpret_cast<varbinder::Variable *>(variable);
        ((reinterpret_cast< ir::AstNode *>(classInstance))->SetVariable(variableE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* const varbinder::LocalVariable *RestVar() const */
TEST_F(PluginConversionRuleUnitTest, VariablePtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_Variable *SignatureRestVarConst([[maybe_unused]] es2panda_Context *context,
es2panda_Signature *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_Variable *>
    ((reinterpret_cast<const checker::Signature *>(classInstance))->RestVar());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] const Decl *Declaration() const noexcept */
TEST_F(PluginConversionRuleUnitTest, VariablePtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_Declaration *VariableDeclarationConst([[maybe_unused]] es2panda_Context *context,
es2panda_Variable *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_Declaration *>
    ((reinterpret_cast<const varbinder::Variable *>(classInstance))->Declaration());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* bool IsVariableScope() const */
TEST_F(PluginConversionRuleUnitTest, ScopePtrInputParameterAndMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" bool ScopeIsVariableScopeConst([[maybe_unused]] es2panda_Context *context,
es2panda_Scope *classInstance/*return_args:*/)
    {
        auto apiRes = ((reinterpret_cast<const varbinder::Scope *>(classInstance))->IsVariableScope());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* const VariableScope *AsVariableScope() const */
TEST_F(PluginConversionRuleUnitTest, ScopePtrAsReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_Scope *ScopeAsVariableScopeConst([[maybe_unused]] es2panda_Context *context,
es2panda_Scope *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_Scope *>
    ((reinterpret_cast<const varbinder::Scope *>(classInstance))->AsVariableScope());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* void Reset(Decl *decl, VariableFlags flags) noexcept */
TEST_F(PluginConversionRuleUnitTest, DeclarationPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void VariableReset([[maybe_unused]] es2panda_Context *context, es2panda_Variable *classInstance,
[[maybe_unused]] es2panda_Declaration *decl, [[maybe_unused]] Es2pandaVariableFlags flags/*return_args:*/)
    {
        auto *declE2p = reinterpret_cast<varbinder::Decl *>(decl);
        auto flagsE2p = E2pToIrVariableFlags(flags);
        ((reinterpret_cast<varbinder::Variable *>(classInstance))->Reset(declE2p, flagsE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] const Decl *Declaration() const noexcept */
TEST_F(PluginConversionRuleUnitTest, DeclarationPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_Declaration *VariableDeclarationConst([[maybe_unused]] es2panda_Context *context,
es2panda_Variable *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_Declaration *>
    ((reinterpret_cast<const varbinder::Variable *>(classInstance))->Declaration());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* bool IsExternal() const */
TEST_F(PluginConversionRuleUnitTest, RecordTablePtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" bool RecordTableIsExternalConst([[maybe_unused]] es2panda_Context *context,
es2panda_RecordTable *classInstance/*return_args:*/)
    {
        auto apiRes = ((reinterpret_cast<const varbinder::RecordTable *>(classInstance))->IsExternal());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit RecordTable(ArenaAllocator *allocator, parser::Program *program, RecordTableFlags flags) */
TEST_F(PluginConversionRuleUnitTest, RecordTableConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_RecordTable *CreateRecordTable([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_Program *program, [[maybe_unused]] Es2pandaRecordTableFlags flags)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *programE2p =  reinterpret_cast<parser::Program *>(program);
        auto flagsE2p = E2pToIrRecordTableFlags(flags);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_RecordTable *>
        (ctxAllocator->New<varbinder::RecordTable>(allocatorE2p, programE2p, flagsE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* util::StringView FormRecordName() const */
TEST_F(PluginConversionRuleUnitTest, BoundContextPtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C"  char *BoundContextFormRecordNameConst([[maybe_unused]] es2panda_Context *context,
es2panda_BoundContext *classInstance/*return_args:*/)
    {
        auto apiRes = StringViewToCString(reinterpret_cast<Context *>(context)->allocator,
    ((reinterpret_cast<const varbinder::BoundContext *>(classInstance))->FormRecordName()));
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ConstScopeFindResult Find(const util::StringView &name,
                             ResolveBindingOptions options = ResolveBindingOptions::BINDINGS) const */
/* ConstScopeFindResulPtrMethodCall */
TEST_F(PluginConversionRuleUnitTest, ConstScopeFindResultReturnValue)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_ScopeFindResult *ScopeFindConst([[maybe_unused]] es2panda_Context *context,
es2panda_Scope *classInstance, [[maybe_unused]] char *name,
[[maybe_unused]] Es2pandaResolveBindingOptions options/*return_args:*/)
    {
        util::StringView nameE2p {name};
        auto optionsE2p = E2pToIrResolveBindingOptions(options);
        auto apiRes = reinterpret_cast<es2panda_ScopeFindResult *>
    (reinterpret_cast<Context *>(context)->allocator->New<varbinder::ConstScopeFindResult>
    (((reinterpret_cast<const varbinder::Scope *>(classInstance))->Find(nameE2p, optionsE2p))));
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* void SetBindingProps(Decl *newDecl, BindingProps *props, bool isStatic) */
TEST_F(PluginConversionRuleUnitTest, BindingPropsPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void ClassScopeSetBindingProps([[maybe_unused]] es2panda_Context *context, es2panda_Scope *classInstance,
[[maybe_unused]] es2panda_Declaration *newDecl, [[maybe_unused]] es2panda_BindingProps *props,
[[maybe_unused]] bool isStatic/*return_args:*/)
    {
        auto *newDeclE2p = reinterpret_cast<varbinder::Decl *>(newDecl);
        auto *propsE2p = reinterpret_cast<varbinder::ClassScope::BindingProps *>(props);
        ((reinterpret_cast<varbinder::ClassScope *>(classInstance))->SetBindingProps(newDeclE2p, propsE2p, isStatic));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit Identifier(ArenaAllocator *const allocator) */
/* ArenaAllocatorPtrParameterConversion */
TEST_F(PluginConversionRuleUnitTest, ContextPtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateIdentifier([[maybe_unused]] es2panda_Context *context)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::Identifier>(allocatorE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ir::Expression *CreateExpression(std::string_view sourceCode,
                                    ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) */
TEST_F(PluginConversionRuleUnitTest, ETSCheckerParamterConversion)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_AstNode *ETSParserCreateExpression([[maybe_unused]] es2panda_Context *context,
[[maybe_unused]] char *sourceCode, [[maybe_unused]] Es2pandaExpressionParseFlags flags/*return_args:*/)
    {
        std::string_view sourceCodeE2p {sourceCode};
        auto flagsE2p = E2pToIrExpressionParseFlags(flags);
        auto apiRes = reinterpret_cast< es2panda_AstNode *>((reinterpret_cast<Context *>
    (context)->parser->AsETSParser())->CreateExpression(sourceCodeE2p, flagsE2p));
        return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* void ResolveReferenceForScope(ir::AstNode *node, Scope *scope) */
TEST_F(PluginConversionRuleUnitTest, ETSBinderPtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" void ETSBinderResolveReferenceForScope([[maybe_unused]] es2panda_Context *context,
[[maybe_unused]] es2panda_AstNode *node, [[maybe_unused]] es2panda_Scope *scope/*return_args:*/)
    {
        auto *nodeE2p = reinterpret_cast<ir::AstNode *>(node);
        auto *scopeE2p = reinterpret_cast<varbinder::Scope *>(scope);
        ((reinterpret_cast<Context *>(context)->parserProgram->VarBinder()->AsETSBinder())
    ->ResolveReferenceForScope(nodeE2p, scopeE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}
}  // namespace ark::es2panda
