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
/* explicit ETSUnionType(ArenaVector<TypeNode *> &&types) */
TEST_F(PluginConversionRuleUnitTest, ArenaVectorInputParameterBadCase)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateETSUnionTypeIr(es2panda_Context *context,
es2panda_AstNode **types, size_t typesLen)
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_FALSE(HasMatched(targetAPIWithNoSpace));
}

/* explicit ETSUnionType(ArenaVector<TypeNode *> &&types, ArenaAllocator *const allocator) */
TEST_F(PluginConversionRuleUnitTest, ArenaVectorInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateETSUnionTypeIr([[maybe_unused]] es2panda_Context *context,
        [[maybe_unused]] es2panda_AstNode **types, size_t typesLen)
    {
        ArenaVector<ir::TypeNode *> typesArenaVector {reinterpret_cast<Context *>(context)->allocator->Adapter()};
        for (size_t i = 0; i < typesLen; ++i) {
            auto *typesElement1 = types[i];
            auto *typesElement1E2p = reinterpret_cast<ir::TypeNode *>(typesElement1);

            typesArenaVector.push_back(typesElement1E2p);
        }
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::ETSUnionType>(std::move(typesArenaVector), allocatorE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] ArenaVector<ir::AnnotationUsage *> &Annotations() noexcept */
/* ir::AnnotationAllowedPtrMethodCall */
TEST_F(PluginConversionRuleUnitTest, ArenaVectorReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode **AnnotationAllowedAnnotations([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/, size_t *returnTypeLen)
    {
 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    auto *ctxAllocator = ctx->allocator;
 	    auto resultVector = ((reinterpret_cast< ir::AnnotationAllowed<ir::AstNode> *>(classInstance))->Annotations());
 	    *returnTypeLen = resultVector.size();
 	    auto apiRes = ctxAllocator->New<es2panda_AstNode *[]>(*returnTypeLen);
 	    for (size_t i = 0; i < *returnTypeLen; ++i) {
 	        auto toPush = reinterpret_cast< es2panda_AstNode *>(resultVector.at(i));
 		    apiRes[i] = reinterpret_cast<es2panda_AstNode *>(toPush);
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* static bool SignatureRelatedToSomeSignature(TypeRelation *relation, Signature *sourceSignature,
                                               ArenaVector<Signature *> *targetSignatures) */
TEST_F(PluginConversionRuleUnitTest, ArenaVectorPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" bool ObjectTypeSignatureRelatedToSomeSignature([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance, [[maybe_unused]] es2panda_TypeRelation *relation,
[[maybe_unused]] es2panda_Signature *sourceSignature, [[maybe_unused]] es2panda_Signature **targetSignatures,
size_t targetSignaturesLen/*return_args:*/)
    {
        auto *relationE2p = reinterpret_cast<checker::TypeRelation *>(relation);
        auto *sourceSignatureE2p = reinterpret_cast<checker::Signature *>(sourceSignature);
        auto *targetSignaturesArenaVector = reinterpret_cast<Context *>(context)->allocator->
    New<ArenaVector< checker::Signature *>>(reinterpret_cast<Context *>(context)->allocator->Adapter());
        for (size_t i = 0; i < targetSignaturesLen; ++i) {
            auto *targetSignaturesElement1 = targetSignatures[i];
            auto *targetSignaturesElement1E2p = reinterpret_cast<checker::Signature *>(targetSignaturesElement1);
            targetSignaturesArenaVector->push_back(targetSignaturesElement1E2p);
        }
        auto apiRes = ((reinterpret_cast< checker::ObjectType *>(classInstance))->
    SignatureRelatedToSomeSignature(relationE2p, sourceSignatureE2p, targetSignaturesArenaVector));
        return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* const ArenaVector<Statement *> &Statements() const */
TEST_F(PluginConversionRuleUnitTest, ArenaVectorPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode **BlockStatementStatementsConst([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/, size_t *returnTypeLen)
    {
 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    auto *ctxAllocator = ctx->allocator;
 	    auto resultVector = ((reinterpret_cast<const ir::BlockStatement *>(classInstance))->Statements());
 	    *returnTypeLen = resultVector.size();
 	    auto apiRes = ctxAllocator->New<es2panda_AstNode *[]>(*returnTypeLen);
 	    for (size_t i = 0; i < *returnTypeLen; ++i) {
 	        auto toPush = reinterpret_cast<es2panda_AstNode *>(resultVector.at(i));
 		    apiRes[i] = reinterpret_cast<es2panda_AstNode *>(toPush);
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* explicit ETSReExportDeclaration(ETSImportDeclaration *etsImportDeclarations,
                                   const std::vector<std::string> &userPaths, util::StringView programPath,
                                   ArenaAllocator *allocator) */
TEST_F(PluginConversionRuleUnitTest, VectorInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateETSReExportDeclaration([[maybe_unused]] es2panda_Context *context
, [[maybe_unused]] es2panda_AstNode *etsImportDeclarations, [[maybe_unused]] char **userPaths,
size_t userPathsLen, [[maybe_unused]] char *programPath)
    {
        auto *etsImportDeclarationsE2p = reinterpret_cast<ir::ETSImportDeclaration *>(etsImportDeclarations);
        std::vector<std::string > userPathsVector;
        for (size_t i = 0; i < userPathsLen; ++i) {
            auto userPathsElement1 = userPaths[i];
            std::string userPathsElement1E2p{userPathsElement1};
            userPathsVector.push_back(userPathsElement1E2p);
        }
        util::StringView programPathE2p{programPath};
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::ETSReExportDeclaration>(etsImportDeclarationsE2p, userPathsVector,
            programPathE2p, allocatorE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* std::vector<varbinder::LocalVariable *> Fields() const */
TEST_F(PluginConversionRuleUnitTest, VectorReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_Variable **ETSObjectTypeFieldsConst([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance/*return_args:*/, size_t *returnTypeLen)
    {
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto resultVector = ((reinterpret_cast<const checker::ETSObjectType *>(classInstance))->Fields());
 	    /* WARNING! This section has not been tested! */
 	    *returnTypeLen = resultVector.size();
 	    auto apiRes = ctxAllocator->New<es2panda_Variable *[]>(*returnTypeLen);
 	    for (size_t i = 0; i < *returnTypeLen; ++i) {
            auto toPush = reinterpret_cast<es2panda_Variable *>(resultVector.at(i));
 		    apiRes[i] = reinterpret_cast<es2panda_Variable *>(toPush);
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ir::Statement *CreateFormattedTopLevelStatement(std::string_view sourceCode,
                                                   std::vector<ir::AstNode *> &insertingNodes) */
TEST_F(PluginConversionRuleUnitTest, VectorPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_AstNode *ETSParserCreateFormattedTopLevelStatement([[maybe_unused]] es2panda_Context *context,
[[maybe_unused]] char *sourceCode, [[maybe_unused]] es2panda_AstNode **insertingNodes,
size_t insertingNodesLen/*return_args:*/)
    {
        std::string_view sourceCodeE2p {sourceCode};
        std::vector<ir::AstNode *> insertingNodesVector;
        for (size_t i = 0; i < insertingNodesLen; ++i) {
            auto *insertingNodesElement1 = insertingNodes[i];
            auto *insertingNodesElement1E2p = reinterpret_cast<ir::AstNode *>(insertingNodesElement1);
            insertingNodesVector.push_back(insertingNodesElement1E2p);
        }
        auto apiRes = reinterpret_cast< es2panda_AstNode *>((reinterpret_cast<Context *>(context)->parser
            ->AsETSParser())->CreateFormattedTopLevelStatement(sourceCodeE2p, insertingNodesVector));
        return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* const ArenaVector<util::StringView> &GetUserPaths() const */
TEST_F(PluginConversionRuleUnitTest, VectorPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" char **ETSReExportDeclarationGetUserPathsConst([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/, size_t *returnTypeLen)
    {
 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    auto *ctxAllocator = ctx->allocator;
 	    auto resultVector = ((reinterpret_cast<const ir::ETSReExportDeclaration *>(classInstance))->GetUserPaths());
 	    *returnTypeLen = resultVector.size();
 	    auto apiRes = ctxAllocator->New< char *[]>(*returnTypeLen);
 	    for (size_t i = 0; i < *returnTypeLen; ++i) {
 	        auto toPush = StringViewToCString(reinterpret_cast<Context *>(context)->allocator, (resultVector.at(i)));
 		    apiRes[i] = reinterpret_cast< char *>(toPush);
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ETSObjectType *GetInstantiatedType(util::StringView hash) */
TEST_F(PluginConversionRuleUnitTest, ArenaSetInputParameter)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_Type *ETSObjectTypeGetInstantiatedType([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance, [[maybe_unused]] char *hash/*return_args:*/)
    {
        util::StringView hashE2p{hash};
        auto apiRes = reinterpret_cast<es2panda_Type *>
    ((reinterpret_cast<checker::ETSObjectType *>(classInstance))->GetInstantiatedType(hashE2p));
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* const ArenaSet<ir::ClassDefinition *> &ClassDefinitions() const */
TEST_F(PluginConversionRuleUnitTest, ArenaSetPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode **RecordTableClassDefinitionsConst([[maybe_unused]] es2panda_Context *context,
es2panda_RecordTable *classInstance/*return_args:*/, size_t *returnTypeLen)
    {
 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    auto *ctxAllocator = ctx->allocator;
 	    auto resultSet = ((reinterpret_cast<const varbinder::RecordTable *>(classInstance))->ClassDefinitions());
 	    *returnTypeLen = resultSet.size();
 	    auto apiRes = ctxAllocator->New< es2panda_AstNode *[]>(*returnTypeLen);
 	    size_t i = 0;
 	    for (auto elem : resultSet) {
 		    auto toPush = reinterpret_cast< es2panda_AstNode *>(elem);
 		    apiRes[i++] = reinterpret_cast< es2panda_AstNode *>(toPush);
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* virtual void MergeBindings(VariableMap const &bindings) */
TEST_F(PluginConversionRuleUnitTest, ArenaUnorderedMapPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void ScopeMergeBindings([[maybe_unused]] es2panda_Context *context, es2panda_Scope *classInstance,
[[maybe_unused]] char **bindingsKeyArray, es2panda_Variable **bindingsValueArray, size_t bindingsLen/*return_args:*/)
    {
    	ArenaUnorderedMap<util::StringView, varbinder::Variable *> bindingsArenaMap
    {reinterpret_cast<Context *>(context)->allocator->Adapter()};
 	    for (size_t i = 0; i < bindingsLen; ++i) {
 		    auto bindingsElement1 = bindingsKeyArray[i];
 		    auto *bindingsElement2 = bindingsValueArray[i];
 		    util::StringView bindingsElement1E2p{bindingsElement1};
 		    auto *bindingsElement2E2p = reinterpret_cast<varbinder::Variable *>(bindingsElement2);
 		    bindingsArenaMap[bindingsElement1E2p] = bindingsElement2E2p;
 	    }
        ((reinterpret_cast< varbinder::Scope *>(classInstance))->MergeBindings(bindingsArenaMap));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ArenaUnorderedMap<util::StringView, varbinder::Variable *> &CachedSyntheticProperties() */
TEST_F(PluginConversionRuleUnitTest, ArenaUnorderedMapPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" char **UnionTypeCachedSyntheticProperties([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance/*return_args:*/, es2panda_Variable ***arenaMapValueArray, size_t *returnTypeLen)
    {
 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    auto *ctxAllocator = ctx->allocator;
 	    auto resultMap = ((reinterpret_cast<checker::UnionType *>(classInstance))->CachedSyntheticProperties());
 	    *returnTypeLen = resultMap.size();
 	    auto apiRes = ctxAllocator->New<char *[]>(*returnTypeLen);
 	    *arenaMapValueArray = ctxAllocator->New<es2panda_Variable *[]>(*returnTypeLen);
 	    size_t i = 0;
 	    for (auto [key, value] : resultMap) {
 		    apiRes[i] = StringViewToCString(reinterpret_cast<Context *>(context)->allocator, (key));
 		    (*arenaMapValueArray)[i] = reinterpret_cast< es2panda_Variable *>(value);
 		    ++i;
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] std::pair<checker::Type *, checker::Type *> GetComplimentaryType(ETSChecker *checker,
                                                                                  checker::Type *sourceType) */
TEST_F(PluginConversionRuleUnitTest, PairReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_Type *ETSUnionTypeCheckerGetComplimentaryType([[maybe_unused]] es2panda_Context *context,
es2panda_Type *classInstance, [[maybe_unused]] es2panda_Type *sourceType/*return_args:*/,
es2panda_Type **returnTypeSecond)
    {
        auto *checkerE2p = reinterpret_cast<Context *>(context)->GetChecker()->AsETSChecker();
        auto *sourceTypeE2p = reinterpret_cast<checker::Type *>(sourceType);
 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    [[maybe_unused]] auto *ctxAllocator = ctx->allocator;
 	    auto resultPair = ((reinterpret_cast<checker::ETSUnionType *>(classInstance))
    ->GetComplimentaryType(checkerE2p, sourceTypeE2p));
 	    *returnTypeSecond = reinterpret_cast<es2panda_Type *>(const_cast<checker::Type *>(resultPair.second));
 	    auto apiRes = reinterpret_cast<es2panda_Type *>(resultPair.first);
        ;
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* ArenaMap<util::StringView, Variable *> OrderedBindings(ArenaAllocator *allocator) const */
TEST_F(PluginConversionRuleUnitTest, ArenaMapReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" char **ScopeOrderedBindingsConst([[maybe_unused]] es2panda_Context *context,
es2panda_Scope *classInstance/*return_args:*/, es2panda_Variable ***arenaMapValueArray, size_t *returnTypeLen)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;

 	    auto *ctx = reinterpret_cast<Context *>(context);
 	    auto *ctxAllocator = ctx->allocator;
 	    auto resultMap = ((reinterpret_cast<const varbinder::Scope *>(classInstance))->OrderedBindings(allocatorE2p));
 	    *returnTypeLen = resultMap.size();
 	    auto apiRes = ctxAllocator->New<char *[]>(*returnTypeLen);
 	    *arenaMapValueArray = ctxAllocator->New<es2panda_Variable *[]>(*returnTypeLen);
 	    size_t i = 0;
 	    for (auto [key, value] : resultMap) {
 		    apiRes[i] = StringViewToCString(reinterpret_cast<Context *>(context)->allocator, (key));
 		    (*arenaMapValueArray)[i] = reinterpret_cast<es2panda_Variable *>(value);
 		    ++i;
 	    };
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* Typed<AstNode> *AsTyped() */
TEST_F(PluginConversionRuleUnitTest, IrTypedPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_AstNode *AstNodeAsTyped([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<es2panda_AstNode *>((reinterpret_cast<ir::AstNode *>(classInstance))
            ->AsTyped());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] checker::Type const *TsType() const */
TEST_F(PluginConversionRuleUnitTest, IrTypedPtrMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_Type *TypedTsTypeConst([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_Type *>
    ((reinterpret_cast<const ir::Typed<ir::AstNode> *>(classInstance))->TsType());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* void SetParent(AstNode *const parent) noexcept */
TEST_F(PluginConversionRuleUnitTest, AstNodePtrInputParameterAndMethodCall)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeSetParent([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_AstNode *parent/*return_args:*/)
    {
        auto *parentE2p = reinterpret_cast<ir::AstNode *>(parent);
        ((reinterpret_cast< ir::AstNode *>(classInstance))->SetParent(parentE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* [[nodiscard]] const AstNode *Parent() const noexcept */
TEST_F(PluginConversionRuleUnitTest, AstNodePtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_AstNode *AstNodeParentConst([[maybe_unused]] es2panda_Context *context,
es2panda_AstNode *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_AstNode *>
((reinterpret_cast<const ir::AstNode *>(classInstance))->Parent());
	    return apiRes;
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

/* virtual void Compile([[maybe_unused]] compiler::PandaGen *pg) const = 0 */
TEST_F(PluginConversionRuleUnitTest, CodeGenPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeCompileConst([[maybe_unused]] es2panda_Context *context, es2panda_AstNode *classInstance,
[[maybe_unused]] es2panda_CodeGen *pg/*return_args:*/)
    {
        auto *pgE2p = reinterpret_cast<compiler::PandaGen *>(pg);
        ((reinterpret_cast<const ir::AstNode *>(classInstance))->Compile(pgE2p));
    }
    )"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}
}  // namespace ark::es2panda