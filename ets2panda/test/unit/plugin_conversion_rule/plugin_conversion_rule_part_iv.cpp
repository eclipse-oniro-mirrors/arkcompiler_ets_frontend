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

// #22952
// apiName: CreateSignature
TEST_F(PluginConversionRuleUnitTest, DISABLED_SignaturerInfoPtrInputParameter)
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

// apiName: SignatureGetSignatureInfoConst
TEST_F(PluginConversionRuleUnitTest, SignaturerInfoPtrReturnValue)
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

// apiName: CreateCheckerContext
TEST_F(PluginConversionRuleUnitTest, CheckerContextPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_CheckerContext *CreateCheckerContext([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] Es2pandaCheckerStatus newStatus)
    {
        auto *checkerE2p = reinterpret_cast<Context *>(context)->GetChecker();
        auto newStatusE2p = E2pToIrCheckerStatus(newStatus);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_CheckerContext *>(ctxAllocator->New<checker::CheckerContext>
               (checkerE2p, newStatusE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CheckerContextStatusConst
TEST_F(PluginConversionRuleUnitTest, CheckerContextPtrCall)
{
    std::string targetCAPI {R"(
    extern "C" Es2pandaCheckerStatus CheckerContextStatusConst([[maybe_unused]] es2panda_Context *context,
    es2panda_CheckerContext *classInstance/*return_args:*/)
    {
        auto apiRes = IrToE2pCheckerStatus((reinterpret_cast<const checker::CheckerContext *>
                   (classInstance))->Status());
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateCheckerContext
TEST_F(PluginConversionRuleUnitTest, CheckerContextPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_CheckerContext *CreateCheckerContext([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] Es2pandaCheckerStatus newStatus)
    {
        auto *checkerE2p = reinterpret_cast<Context *>(context)->GetChecker();
        auto newStatusE2p = E2pToIrCheckerStatus(newStatus);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_CheckerContext *>(ctxAllocator->
               New<checker::CheckerContext>(checkerE2p, newStatusE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateResolveResult
TEST_F(PluginConversionRuleUnitTest, CheckerResolveResultPtrReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ResolveResult *CreateResolveResult([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_Variable *v, [[maybe_unused]] Es2pandaResolvedKind kind)
    {
        auto *vE2p = reinterpret_cast<varbinder::Variable *>(v);
        auto kindE2p = E2pToIrResolvedKind(kind);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ResolveResult *>(ctxAllocator->New<checker::ResolveResult>(vE2p, kindE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateResolveResult
TEST_F(PluginConversionRuleUnitTest, CheckerResolveResultPtrConstructor)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_ResolveResult *CreateResolveResult([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_Variable *v, [[maybe_unused]] Es2pandaResolvedKind kind)
    {
        auto *vE2p = reinterpret_cast<varbinder::Variable *>(v);
        auto kindE2p = E2pToIrResolvedKind(kind);
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        return reinterpret_cast<es2panda_ResolveResult *>(ctxAllocator->New<checker::ResolveResult>(vE2p, kindE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: FunctionScopeBindName
TEST_F(PluginConversionRuleUnitTest, UtilStringViewInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void FunctionScopeBindName([[maybe_unused]] es2panda_Context *context, es2panda_Scope *classInstance,
    [[maybe_unused]] char *name/*return_args:*/)
    {
        util::StringView nameE2p {name};
        ((reinterpret_cast< varbinder::FunctionScope *>(classInstance))->BindName(nameE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: FunctionScopeNameConst
TEST_F(PluginConversionRuleUnitTest, UtilStringViewReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const char *FunctionScopeNameConst([[maybe_unused]] es2panda_Context *context,
    es2panda_Scope *classInstance/*return_args:*/)
    {
        auto apiRes = StringViewToCString(reinterpret_cast<Context *>(context)->allocator,
                   ((reinterpret_cast<const varbinder::FunctionScope *>(classInstance))->Name()));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ETSParserCreateExpression
TEST_F(PluginConversionRuleUnitTest, StringViewInputParameter)
{
    std::string targetCAPI {R"(
    extern "C"  es2panda_AstNode *ETSParserCreateExpression([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] char *sourceCode, [[maybe_unused]] Es2pandaExpressionParseFlags flags/*return_args:*/)
    {
        std::string_view sourceCodeE2p {sourceCode};
        auto flagsE2p = E2pToIrExpressionParseFlags(flags);
        auto apiRes = reinterpret_cast< es2panda_AstNode *>((reinterpret_cast<Context *>(context)->parser->
                   AsETSParser())->CreateExpression(sourceCodeE2p, flagsE2p));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: IdentifierNameConst
TEST_F(PluginConversionRuleUnitTest, StringViewReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" const char *IdentifierNameConst([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance/*return_args:*/)
    {
        auto apiRes = StringViewToCString(reinterpret_cast<Context *>(context)->allocator,
                   ((reinterpret_cast<const ir::Identifier *>(classInstance))->Name()));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: SrcDumperAdd
TEST_F(PluginConversionRuleUnitTest, StringInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void SrcDumperAdd([[maybe_unused]] es2panda_Context *context, es2panda_SrcDumper *classInstance,
    [[maybe_unused]] char *str/*return_args:*/)
    {
        std::string strE2p {str};
        ((reinterpret_cast< ir::SrcDumper *>(classInstance))->Add(strE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: SrcDumperStrConst
TEST_F(PluginConversionRuleUnitTest, StringReturnValue)
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

// apiName: EnumVariableSetValue
TEST_F(PluginConversionRuleUnitTest, VarbinderEnumMemberResultInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void EnumVariableSetValue([[maybe_unused]] es2panda_Context *context, es2panda_Variable *classInstance,
    [[maybe_unused]] es2panda_variantDoubleCharArrayBool value/*return_args:*/)
    {
        varbinder::EnumMemberResult valueE2p;
        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
        switch  (value.index) {
          case 0:
            valueE2p = std::variant<double, util::StringView, bool>(value.variant.d);
            break;
          case 1:
            {
              util::StringView valueE2pStringView {value.variant.c};
              valueE2p = std::variant<double, util::StringView, bool>(valueE2pStringView);
              break;
            }
          case 2:
            valueE2p = std::variant<double, util::StringView, bool>(value.variant.b);
            break;
          default:
            break;
        }
        // NOLINTEND(cppcoreguidelines-pro-type-union-access)

        ((reinterpret_cast< varbinder::EnumVariable *>(classInstance))->SetValue(valueE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: EnumVariableValueConst
TEST_F(PluginConversionRuleUnitTest, VarbinderEnumMemberResultReturnValue)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_variantDoubleCharArrayBool EnumVariableValueConst([[maybe_unused]] es2panda_Context *context,
    es2panda_Variable *classInstance/*return_args:*/)
    {
        auto apiRes = EnumMemberResultToEs2pandaVariant(reinterpret_cast<Context *>(context)->allocator,
                   ((reinterpret_cast<const varbinder::EnumVariable *>(classInstance))->Value()));
    	return apiRes;
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: AstNodeIterateConst
TEST_F(PluginConversionRuleUnitTest, NodeTraverserInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeIterateConst([[maybe_unused]] es2panda_Context *context, es2panda_AstNode *classInstance,
    [[maybe_unused]] NodeTraverser cb/*return_args:*/)
    {
        std::function<void(ir::AstNode *)> cbE2p = [cb](ir::AstNode *traverserLambdaNode)
                                                   {cb(reinterpret_cast<es2panda_AstNode *>(traverserLambdaNode));};
        ((reinterpret_cast<const ir::AstNode *>(classInstance))->Iterate(cbE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: AstNodeTransformChildren
TEST_F(PluginConversionRuleUnitTest, NodeTransformerInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void AstNodeTransformChildren([[maybe_unused]] es2panda_Context *context,
    es2panda_AstNode *classInstance, [[maybe_unused]] NodeTransformer cb,
    [[maybe_unused]] char *transformationName/*return_args:*/)
    {
        std::function<ir::AstNode *(ir::AstNode *)> cbE2p = [cb](ir::AstNode *traverserLambdaNode)
        {return reinterpret_cast<ir::AstNode *>(cb(reinterpret_cast<es2panda_AstNode *>(traverserLambdaNode)));};
        std::string_view transformationNameE2p {transformationName};
        ((reinterpret_cast< ir::AstNode *>(classInstance))->TransformChildren(cbE2p, transformationNameE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ETSObjectTypeUpdateTypeProperties
TEST_F(PluginConversionRuleUnitTest, PropertyProcessorInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void ETSObjectTypeUpdateTypeProperties([[maybe_unused]] es2panda_Context *context,
    es2panda_Type *classInstance, [[maybe_unused]] PropertyProcessor func/*return_args:*/)
    {
        std::function<varbinder::LocalVariable *(varbinder::LocalVariable *, checker::Type *)> funcE2p =
        [func](varbinder::LocalVariable *propertyProcessorLambdaVariable, checker::Type *propertyProcessorLambdaType) {
        return reinterpret_cast<varbinder::LocalVariable *>(func(reinterpret_cast<es2panda_Variable *>
        (propertyProcessorLambdaVariable), reinterpret_cast<es2panda_Type *>(propertyProcessorLambdaType)));};
        ((reinterpret_cast< checker::ETSObjectType *>(classInstance))->UpdateTypeProperties(funcE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: ETSObjectTypeIterateConst
TEST_F(PluginConversionRuleUnitTest, PropertyTraverserInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" void ETSObjectTypeIterateConst([[maybe_unused]] es2panda_Context *context,
    es2panda_Type *classInstance, [[maybe_unused]] PropertyTraverser cb/*return_args:*/)
    {
        std::function<void(const varbinder::LocalVariable *)> cbE2p =
        [cb](const varbinder::LocalVariable *propertyTraverserLambdaVariable)
        {cb(reinterpret_cast<const es2panda_Variable *>(propertyTraverserLambdaVariable));};
        ((reinterpret_cast<const checker::ETSObjectType *>(classInstance))->Iterate(cbE2p));
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateTSModuleDeclaration
TEST_F(PluginConversionRuleUnitTest, TSModuleDeclarationConstructorFlagsInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateTSModuleDeclaration([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *name, [[maybe_unused]] es2panda_AstNode *body,
    [[maybe_unused]] bool declare, bool global)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *nameE2p = reinterpret_cast<ir::Expression *>(name);
        auto *bodyE2p = reinterpret_cast<ir::Statement *>(body);

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::TSModuleDeclaration>(allocatorE2p, nameE2p,
            bodyE2p, ir::TSModuleDeclaration::ConstructorFlags {declare, global}));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateTSEnumDeclaration
TEST_F(PluginConversionRuleUnitTest, TSEnumDeclarationConstructorFlagsInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateTSEnumDeclaration([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode *key, [[maybe_unused]] es2panda_AstNode **members, size_t membersLen,
    [[maybe_unused]] bool isConst, bool isStatic, bool isDeclare)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        auto *keyE2p = reinterpret_cast<ir::Identifier *>(key);
        ArenaVector<ir::AstNode *> membersArenaVector {reinterpret_cast<Context *>(context)->allocator->Adapter()};
        for (size_t i = 0; i < membersLen; ++i) {
            auto *membersElement1 = members[i];
            auto *membersElement1E2p = reinterpret_cast<ir::AstNode *>(membersElement1);

            membersArenaVector.push_back(membersElement1E2p);
        }
        
        ark::es2panda::Language langE2p {Language::Id::ETS};
        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::TSEnumDeclaration>(allocatorE2p, keyE2p,
            std::move(membersArenaVector), ir::TSEnumDeclaration::ConstructorFlags {isConst, isStatic, isDeclare},
            langE2p));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};
    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}

// apiName: CreateTSInterfaceDeclaration
TEST_F(PluginConversionRuleUnitTest, TSInterfaceDeclarationConstructorDataInputParameter)
{
    std::string targetCAPI {R"(
    extern "C" es2panda_AstNode *CreateTSInterfaceDeclaration([[maybe_unused]] es2panda_Context *context,
    [[maybe_unused]] es2panda_AstNode **extends, size_t extendsLen, [[maybe_unused]] es2panda_AstNode *id,
    es2panda_AstNode *typeParams, es2panda_AstNode *body, bool isStatic, bool isExternal)
    {
        auto *allocatorE2p = reinterpret_cast<Context *>(context)->allocator;
        ArenaVector<ir::TSInterfaceHeritage *> extendsArenaVector {reinterpret_cast<Context *>
        (context)->allocator->Adapter()};
     	for (size_t i = 0; i < extendsLen; ++i) {
 	    	auto *extendsElement1 = extends[i];
 	    	auto *extendsElement1E2p = reinterpret_cast<ir::TSInterfaceHeritage *>(extendsElement1);

 	    	extendsArenaVector.push_back(extendsElement1E2p);
 	    }
        auto es2pandaId = reinterpret_cast<ir::Identifier *>(id);
        auto es2pandaTypeParams = reinterpret_cast<ir::TSTypeParameterDeclaration *>(typeParams);
        auto es2pandaBody = reinterpret_cast<ir::TSInterfaceBody *>(body);
        ark::es2panda::Language es2pandaLang {Language::Id::ETS};

        auto *ctx = reinterpret_cast<Context *>(context);
        auto *ctxAllocator = ctx->allocator;
        auto *astNode = (ctxAllocator->New<ir::TSInterfaceDeclaration>(allocatorE2p, std::move(extendsArenaVector),
            ir::TSInterfaceDeclaration::ConstructorData
            {es2pandaId, es2pandaTypeParams, es2pandaBody, isStatic, isExternal, es2pandaLang}));
        astNode->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);
        return reinterpret_cast<es2panda_AstNode *>(astNode);
    })"};

    std::string targetAPIWithNoSpace = RemoveWhitespace(targetCAPI);
    EXPECT_TRUE(HasMatched(targetAPIWithNoSpace));
}
}  // namespace ark::es2panda
