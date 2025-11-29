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
#include "plugin_conversion_rule_unit_test.h"

#include <gtest/gtest.h>

namespace ark::es2panda {
// apiName: CreateETSModule
TEST_F(PluginConversionRuleUnitTest, ParserProgramPtrInputParameter)
{
    std::string targetCAPI {R"(
    /* explicit ETSModule(ArenaAllocator *allocator, ArenaVector<Statement *> &&statementList, Identifier *ident,
                       ModuleFlag flag, Language lang, parser::Program *program) */
    extern "C" es2panda_AstNode *CreateETSModule([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode **statementList, size_t statementListLen,
    [[maybe_unused]] es2panda_AstNode *ident, [[maybe_unused]] Es2pandaModuleFlag flag,
    [[maybe_unused]] es2panda_Program *program)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        ArenaVector<ir::Statement *> statementListArenaVector
        {reinterpret_cast<Context *>(context)->allocator->Adapter()};
        for (size_t i = 0; i < statementListLen; ++i) {
            auto *statementListElement1 = statementList[i];
            auto *statementListElement1E2p = reinterpret_cast<ir::Statement *>(statementListElement1);

            statementListArenaVector.push_back(statementListElement1E2p);
        }
        auto *identE2p = reinterpret_cast<ir::Identifier *>(ident);
        auto flagE2p = E2pToIrModuleFlag(flag);
        ark::es2panda::Language langE2p {Language::Id::ETS};
        auto *programE2p =  reinterpret_cast<parser::Program *>(program);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::ETSModule>(allocatorE2p, std::move(statementListArenaVector),
            identE2p, flagE2p, langE2p, programE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateETSFunctionTypeIr
TEST_F(PluginConversionRuleUnitTest, FunctionSignatureInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateETSFunctionTypeIr([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_FunctionSignature *signature, [[maybe_unused]] Es2pandaScriptFunctionFlags funcFlags)
    {
        auto funcFlagsE2p = E2pToIrScriptFunctionFlags(funcFlags);
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::ETSFunctionType>(
            std::forward<ir::FunctionSignature>(*reinterpret_cast<ir::FunctionSignature *>(signature)),
            funcFlagsE2p, allocatorE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateFunctionSignature
TEST_F(PluginConversionRuleUnitTest, FunctionSignaturePtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_FunctionSignature *CreateFunctionSignature([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *typeParams, [[maybe_unused]] es2panda_AstNode **params, size_t paramsLen,
    [[maybe_unused]] es2panda_AstNode *returnTypeAnnotation, [[maybe_unused]] bool hasReceiver)
    {
        auto *typeParamsE2p = reinterpret_cast<ir::TSTypeParameterDeclaration *>(typeParams);
        ArenaVector<ir::Expression *> paramsArenaVector {reinterpret_cast<Context *>(context)->allocator->Adapter()};
     	for (size_t i = 0; i < paramsLen; ++i) {
     		auto *paramsElement1 = params[i];
     		auto *paramsElement1E2p = reinterpret_cast<ir::Expression *>(paramsElement1);

     		paramsArenaVector.push_back(paramsElement1E2p);
     	}
        auto *returnTypeAnnotationE2p = reinterpret_cast<ir::TypeNode *>(returnTypeAnnotation);

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_FunctionSignature *>(ctxAllocator->New<ir::FunctionSignature>(typeParamsE2p,
               std::move(paramsArenaVector), returnTypeAnnotationE2p, hasReceiver));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: FunctionSignatureParamsConst
TEST_F(PluginConversionRuleUnitTest, FunctionSignaturePtrCall)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode **FunctionSignatureParamsConst([[maybe_unused]] es2panda_Context *context,
    es2panda_FunctionSignature *classInstance/*return_args:*/, size_t *returnTypeLen)
    {
     	auto *ctx = reinterpret_cast<Context *>(context);
     	auto *ctxAllocator = ctx->allocator;
     	auto resultVector = ((reinterpret_cast<const ir::FunctionSignature *>(classInstance))->Params());
     	*returnTypeLen = resultVector.size();
     	auto apiRes = ctxAllocator->New< es2panda_AstNode *[]>(*returnTypeLen);
     	for (size_t i = 0; i < *returnTypeLen; ++i) {
     	auto toPush = reinterpret_cast< es2panda_AstNode *>(resultVector.at(i));
     		apiRes[i] = reinterpret_cast< es2panda_AstNode *>(toPush);
     	};
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateFunctionSignature
TEST_F(PluginConversionRuleUnitTest, FunctionSignaturePtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_FunctionSignature *CreateFunctionSignature([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *typeParams, [[maybe_unused]] es2panda_AstNode **params, size_t paramsLen,
    [[maybe_unused]] es2panda_AstNode *returnTypeAnnotation, [[maybe_unused]] bool hasReceiver)
    {
        auto *typeParamsE2p = reinterpret_cast<ir::TSTypeParameterDeclaration *>(typeParams);
        ArenaVector<ir::Expression *> paramsArenaVector {reinterpret_cast<Context *>(context)->allocator->Adapter()};
     	for (size_t i = 0; i < paramsLen; ++i) {
     		auto *paramsElement1 = params[i];
     		auto *paramsElement1E2p = reinterpret_cast<ir::Expression *>(paramsElement1);

     		paramsArenaVector.push_back(paramsElement1E2p);
 	}
        auto *returnTypeAnnotationE2p = reinterpret_cast<ir::TypeNode *>(returnTypeAnnotation);

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_FunctionSignature *>(ctxAllocator->New<ir::FunctionSignature>(typeParamsE2p,
               std::move(paramsArenaVector), returnTypeAnnotationE2p, hasReceiver));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateImportSource
TEST_F(PluginConversionRuleUnitTest, DISABLED_LanguageInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ImportSource *CreateImportSource([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *source, [[maybe_unused]] es2panda_AstNode *resolvedSource,
    [[maybe_unused]] bool hasDecl)
    {
        auto *sourceE2p = reinterpret_cast<ir::StringLiteral *>(source);
        auto *resolvedSourceE2p = reinterpret_cast<ir::StringLiteral *>(resolvedSource);
        ark::es2panda::Language langE2p {Language::Id::ETS};

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ImportSource *>(ctxAllocator->New<ir::ImportSource>(sourceE2p,
               resolvedSourceE2p, langE2p, hasDecl));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: IdentifierIsReferenceConst
TEST_F(PluginConversionRuleUnitTest, ScriptionExtensionInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" bool IdentifierIsReferenceConst([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance/*return_args:*/)
    {
        ark::es2panda::ScriptExtension extE2p {ScriptExtension::ETS};
        auto apiRes = ((reinterpret_cast<const ir::Identifier *>(classInstance))->IsReference(extE2p));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: AstNodeSetStart
TEST_F(PluginConversionRuleUnitTest, SourcePositionInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeSetStart([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_SourcePosition *start/*return_args:*/)
    {
        auto &startE2p = *reinterpret_cast<lexer::SourcePosition *>(start);
        ((reinterpret_cast< ir::AstNode *>(classInstance))->SetStart(startE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: MemberExpressionCompileToRegConst
TEST_F(PluginConversionRuleUnitTest, VRegInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void MemberExpressionCompileToRegConst([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_CodeGen *pg,
    [[maybe_unused]] es2panda_VReg *objReg/*return_args:*/)
    {
        auto *pgE2p = reinterpret_cast<compiler::PandaGen *>(pg);
        auto &objRegE2p = *reinterpret_cast<compiler::VReg *>(objReg);
        ((reinterpret_cast<const ir::MemberExpression *>(classInstance))->CompileToReg(pgE2p, objRegE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: AstNodeDumpConst1
TEST_F(PluginConversionRuleUnitTest, SrcDumperPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeDumpConst1([[maybe_unused]] es2panda_Context *context, es2panda_AstNode *classInstance,
    [[maybe_unused]] es2panda_SrcDumper *dumper/*return_args:*/)
    {
        auto *dumperE2p = reinterpret_cast<ir::SrcDumper *>(dumper);
        ((reinterpret_cast<const ir::AstNode *>(classInstance))->Dump(dumperE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateSrcDumper
TEST_F(PluginConversionRuleUnitTest, SrcDumperPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_SrcDumper *CreateSrcDumper([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *node)
    {
        auto *nodeE2p = reinterpret_cast<ir::AstNode *>(node);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_SrcDumper *>(ctxAllocator->New<ir::SrcDumper>(nodeE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: SrcDumperStrConst
TEST_F(PluginConversionRuleUnitTest, SrcDumperPtrCall)
{
    std::string targetCAPI {R"(
    extern "C"  char *SrcDumperStrConst([[maybe_unused]] es2panda_Context *context,
    es2panda_SrcDumper *classInstance/*return_args:*/)
    {
        auto apiRes = StdStringToCString(reinterpret_cast<Context *>(context)->allocator,
                   ((reinterpret_cast<const ir::SrcDumper *>(classInstance))->Str()));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateSrcDumper
TEST_F(PluginConversionRuleUnitTest, SrcDumperPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_SrcDumper *CreateSrcDumper([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *node)
    {
        auto *nodeE2p = reinterpret_cast<ir::AstNode *>(node);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_SrcDumper *>(ctxAllocator->New<ir::SrcDumper>(nodeE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: AstNodeDumpConst
TEST_F(PluginConversionRuleUnitTest, AstDumperPtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeDumpConst([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_AstDumper *dumper/*return_args:*/)
    {
        auto *dumperE2p = reinterpret_cast<ir::AstDumper *>(dumper);
        ((reinterpret_cast<const ir::AstNode *>(classInstance))->Dump(dumperE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateAstDumper
TEST_F(PluginConversionRuleUnitTest, AstDumperPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstDumper *CreateAstDumper([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *node, [[maybe_unused]] char *sourceCode)
    {
        auto *nodeE2p = reinterpret_cast<ir::AstNode *>(node);
        util::StringView sourceCodeE2p {sourceCode};
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_AstDumper *>(ctxAllocator->New<ir::AstDumper>(nodeE2p, sourceCodeE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: AstDumperStrConst
TEST_F(PluginConversionRuleUnitTest, AstDumperPtrCall)
{
    std::string targetCAPI {R"(
    extern "C"  char *AstDumperStrConst([[maybe_unused]] es2panda_Context *context,
    es2panda_AstDumper *classInstance/*return_args:*/)
    {
        auto apiRes = StdStringToCString(reinterpret_cast<Context *>(context)->allocator, ((reinterpret_cast<
                   const ir::AstDumper *>(classInstance))->Str()));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateAstDumper
TEST_F(PluginConversionRuleUnitTest, AstDumperPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstDumper *CreateAstDumper([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *node, [[maybe_unused]] char *sourceCode)
    {
        auto *nodeE2p = reinterpret_cast<ir::AstNode *>(node);
        util::StringView sourceCodeE2p {sourceCode};
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_AstDumper *>(ctxAllocator->New<ir::AstDumper>(nodeE2p, sourceCodeE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateTryStatement
TEST_F(PluginConversionRuleUnitTest, LabelPairInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateTryStatement([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *block, [[maybe_unused]] es2panda_AstNode **catchClauses,
    size_t catchClausesLen, [[maybe_unused]] es2panda_AstNode *finalizer,
    [[maybe_unused]] es2panda_LabelPair **finalizerInsertionsLabelPair, size_t finalizerInsertionsLabelPairLen,
    [[maybe_unused]] es2panda_AstNode **finalizerInsertionsStatement, size_t finalizerInsertionsStatementLen)
    {
        auto *blockE2p = reinterpret_cast<ir::BlockStatement *>(block);
        ArenaVector<ir::CatchClause *> catchClausesArenaVector
        {reinterpret_cast<Context *>(context)->allocator->Adapter()};
     	for (size_t i = 0; i < catchClausesLen; ++i) {
     		auto *catchClausesElement1 = catchClauses[i];
 	    	auto *catchClausesElement1E2p = reinterpret_cast<ir::CatchClause *>(catchClausesElement1);

 	    	catchClausesArenaVector.push_back(catchClausesElement1E2p);
     	}
        auto *finalizerE2p = reinterpret_cast<ir::BlockStatement *>(finalizer);
        ArenaVector<compiler::LabelPair > finalizerInsertionsLabelPairArenaVector
        {reinterpret_cast<Context *>(context)->allocator->Adapter()};
     	for (size_t i = 0; i < finalizerInsertionsLabelPairLen; ++i) {
     		auto finalizerInsertionsLabelPairElement1 = finalizerInsertionsLabelPair[i];

     		finalizerInsertionsLabelPairArenaVector.push_back(std::forward<compiler::LabelPair>
            (*reinterpret_cast<compiler::LabelPair *>(finalizerInsertionsLabelPairElement1)));
     	}
        ArenaVector<ir::Statement *> finalizerInsertionsStatementArenaVector {
        reinterpret_cast<Context *>(context)->allocator->Adapter()};
     	for (size_t i = 0; i < finalizerInsertionsStatementLen; ++i) {
 	    	auto *finalizerInsertionsStatementElement1 = finalizerInsertionsStatement[i];
 	    	auto *finalizerInsertionsStatementElement1E2p = reinterpret_cast<ir::Statement *>
                                                            (finalizerInsertionsStatementElement1);

 	    	finalizerInsertionsStatementArenaVector.push_back(finalizerInsertionsStatementElement1E2p);
     	}
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::TryStatement>(blockE2p, std::move(catchClausesArenaVector),
            finalizerE2p, finalizerInsertionsLabelPairArenaVector, finalizerInsertionsStatementArenaVector));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: TryStatementAddFinalizerInsertion
TEST_F(PluginConversionRuleUnitTest, LabelPairReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_LabelPair *TryStatementAddFinalizerInsertion([[maybe_unused]]
    es2panda_Context *context, es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_LabelPair *insertion,
    [[maybe_unused]] es2panda_AstNode *insertionStmt/*return_args:*/, const es2panda_AstNode **returnTypeSecond)
    {
    {
        auto *prog = const_cast<parser::Program *>(reinterpret_cast<ir::AstNode *>(classInstance)->Program());
        if (prog != nullptr) {
            prog->SetProgramModified(true);
        }
    }

        auto *insertionStmtE2p = reinterpret_cast<ir::Statement *>(insertionStmt);

        auto *ctx = reinterpret_cast<Context *>(context);
        [[maybe_unused]] auto *ctxAllocator = ctx->allocator;
        auto resultPair = ((reinterpret_cast< ir::TryStatement *>(classInstance))->
        AddFinalizerInsertion(std::forward<compiler::LabelPair>
        (*reinterpret_cast<compiler::LabelPair *>(insertion)), insertionStmtE2p));
        *returnTypeSecond = reinterpret_cast<const es2panda_AstNode *>
        (const_cast<ir::Statement *>(resultPair.second));
        auto apiRes = reinterpret_cast< es2panda_LabelPair *>
        (ctxAllocator->New<compiler::LabelPair>(resultPair.first));
    ;
        return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: TryStatementAddFinalizerInsertion
TEST_F(PluginConversionRuleUnitTest, LabelPairPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_LabelPair *TryStatementAddFinalizerInsertion([[maybe_unused]]
    es2panda_Context *context, es2panda_AstNode *classInstance, [[maybe_unused]]
    es2panda_LabelPair *insertion, [[maybe_unused]] es2panda_AstNode
    *insertionStmt/*return_args:*/, const es2panda_AstNode **returnTypeSecond)
    {
    {
        auto *prog = const_cast<parser::Program *>(reinterpret_cast<ir::AstNode *>(classInstance)->Program());
        if (prog != nullptr) {
            prog->SetProgramModified(true);
        }
    }
        auto *insertionStmtE2p = reinterpret_cast<ir::Statement *>(insertionStmt);

        auto *ctx = reinterpret_cast<Context *>(context);
        [[maybe_unused]] auto *ctxAllocator = ctx->allocator;
        auto resultPair = ((reinterpret_cast< ir::TryStatement *>(classInstance))->
        AddFinalizerInsertion(std::forward<compiler::
        LabelPair>(*reinterpret_cast<compiler::LabelPair *>(insertion)), insertionStmtE2p));
        *returnTypeSecond = reinterpret_cast<const es2panda_AstNode *>(const_cast<ir::Statement *>(resultPair.second));
        auto apiRes = reinterpret_cast< es2panda_LabelPair *>(ctxAllocator->New<compiler::LabelPair>(resultPair.first));
    ;
        return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateETSImportDeclaration
TEST_F(PluginConversionRuleUnitTest, DISABLED_ImportSourcePtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateETSImportDeclaration([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_ImportSource *source, [[maybe_unused]] es2panda_AstNode **specifiers,
    size_t specifiersLen, [[maybe_unused]] Es2pandaImportKinds importKind)
    {
        auto *sourceE2p = reinterpret_cast<ir::ImportSource *>(source);
        ArenaVector<ir::AstNode *> specifiersArenaVector {reinterpret_cast<Context *>(context)->allocator->Adapter()};
     	for (size_t i = 0; i < specifiersLen; ++i) {
     		auto *specifiersElement1 = specifiers[i];
     		auto *specifiersElement1E2p = reinterpret_cast<ir::AstNode *>(specifiersElement1);

     		specifiersArenaVector.push_back(specifiersElement1E2p);
     	}
        auto importKindE2p = E2pToIrImportKinds(importKind);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::ETSImportDeclaration>(sourceE2p, specifiersArenaVector, importKindE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateImportSource
TEST_F(PluginConversionRuleUnitTest, DISABLED_ImportSourcePtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ImportSource *CreateImportSource([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *source, [[maybe_unused]] es2panda_AstNode *resolvedSource,
    [[maybe_unused]] bool hasDecl)
    {
        auto *sourceE2p = reinterpret_cast<ir::StringLiteral *>(source);
        auto *resolvedSourceE2p = reinterpret_cast<ir::StringLiteral *>(resolvedSource);
        ark::es2panda::Language langE2p {Language::Id::ETS};

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ImportSource *>(ctxAllocator->New<ir::ImportSource>(sourceE2p,
               resolvedSourceE2p, langE2p, hasDecl));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ImportSourceSourceConst
TEST_F(PluginConversionRuleUnitTest, DISABLED_ImportSourcePtrCall)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_AstNode *ImportSourceSourceConst([[maybe_unused]] es2panda_Context *context,
    es2panda_ImportSource *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_AstNode *>((reinterpret_cast<const ir::ImportSource *>
                   (classInstance))->Source());
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateImportSource
TEST_F(PluginConversionRuleUnitTest, DISABLED_ImportSourcePtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ImportSource *CreateImportSource([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *source, [[maybe_unused]] es2panda_AstNode *resolvedSource,
    [[maybe_unused]] bool hasDecl)
    {
        auto *sourceE2p = reinterpret_cast<ir::StringLiteral *>(source);
        auto *resolvedSourceE2p = reinterpret_cast<ir::StringLiteral *>(resolvedSource);
        ark::es2panda::Language langE2p {Language::Id::ETS};

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ImportSource *>(ctxAllocator->New<ir::ImportSource>(
               sourceE2p, resolvedSourceE2p, langE2p, hasDecl));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateValidationInfo
TEST_F(PluginConversionRuleUnitTest, ValidationInfoPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ValidationInfo *CreateValidationInfo([[maybe_unused]] es2panda_Context *context)
    {
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ValidationInfo *>(ctxAllocator->New<ir::ValidationInfo>());
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ValidationInfoFailConst
TEST_F(PluginConversionRuleUnitTest, ValidationInfoPtrCall)
{
    std::string targetCAPI {R"(
    extern "C" bool ValidationInfoFailConst([[maybe_unused]] es2panda_Context *context,
    es2panda_ValidationInfo *classInstance/*return_args:*/)
    {
        auto apiRes = ((reinterpret_cast<const ir::ValidationInfo *>(classInstance))->Fail());
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateValidationInfo
TEST_F(PluginConversionRuleUnitTest, ValidationInfoPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ValidationInfo *CreateValidationInfo([[maybe_unused]] es2panda_Context *context)
    {
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ValidationInfo *>(ctxAllocator->New<ir::ValidationInfo>());
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ScriptFunctionSetSignature
TEST_F(PluginConversionRuleUnitTest, IRSignaturePtrInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void ScriptFunctionSetSignature([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] es2panda_Signature *signature/*return_args:*/)
    {
    {
        auto *prog = const_cast<parser::Program *>(reinterpret_cast<ir::AstNode *>(classInstance)->Program());
        if (prog != nullptr) {
            prog->SetProgramModified(true);
        }
    }
        auto *signatureE2p = reinterpret_cast<checker::Signature *>(signature);
        ((reinterpret_cast< ir::ScriptFunction *>(classInstance))->SetSignature(signatureE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ScriptFunctionSignatureConst
TEST_F(PluginConversionRuleUnitTest, IRSignaturePtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_Signature *ScriptFunctionSignatureConst([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_Signature *>((reinterpret_cast<
                   const ir::ScriptFunction *>(classInstance))->Signature());
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: SignatureGetSignatureInfoConst
TEST_F(PluginConversionRuleUnitTest, IRSignaturePtrCall)
{
    std::string targetCAPI {R"(
    extern "C" const es2panda_SignatureInfo *SignatureGetSignatureInfoConst([[maybe_unused]] es2panda_Context *context,
    es2panda_Signature *classInstance/*return_args:*/)
    {
        auto apiRes = reinterpret_cast<const es2panda_SignatureInfo *>((reinterpret_cast<
                   const checker::Signature *>(classInstance))->GetSignatureInfo());
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// #22952
// apiName: CreateSignature
TEST_F(PluginConversionRuleUnitTest, DISABLED_IRSignaturerPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_Signature *CreateSignature([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_SignatureInfo *signatureInfo, [[maybe_unused]]  es2panda_Type *returnType)
    {
        auto *signatureInfoE2p = reinterpret_cast<checker::SignatureInfo *>(signatureInfo);
        auto *returnTypeE2p = reinterpret_cast<checker::Type *>(returnType);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_Signature *>(ctxAllocator->New<checker::Signature>
               (signatureInfoE2p, returnTypeE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}
}  // namespace ark::es2panda
