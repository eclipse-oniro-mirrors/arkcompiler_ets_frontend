/*
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

#include "initModuleLowering.h"

#include "compiler/lowering/util.h"
#include "generated/signatures.h"
#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/memberExpression.h"
#include "parser/innerSourceParser.h"
#include "util/es2pandaMacros.h"
#include "util/helpers.h"

namespace ark::es2panda::compiler {

static bool IsInitModuleCall(ir::AstNode *node)
{
    if (!node->IsCallExpression()) {
        return false;
    }
    auto *callee = node->AsCallExpression()->Callee();
    if (callee->IsIdentifier() && callee->AsIdentifier()->Name() == compiler::Signatures::INIT_MODULE_METHOD) {
        return true;
    }
    return false;
}

static bool IsESValueLoadCall(ir::AstNode *node)
{
    if (!node->IsCallExpression()) {
        return false;
    }
    auto *callee = node->AsCallExpression()->Callee();
    if (!callee->IsMemberExpression() || !callee->AsMemberExpression()->Object()->IsIdentifier() ||
        !callee->AsMemberExpression()->Property()->IsIdentifier()) {
        return false;
    }
    auto *object = callee->AsMemberExpression()->Object();
    auto *property = callee->AsMemberExpression()->Property();
    if (object->AsIdentifier()->Name() == compiler::Signatures::ESVALUE &&
        property->AsIdentifier()->Name() == compiler::Signatures::LOAD) {
        return true;
    }
    return false;
}

static ir::AstNode *CreateModuleCallExpressionForDynamic(public_lib::Context *ctx, ir::CallExpression *callExpr,
                                                         std::string_view ohmUrl)
{
    auto allocator = ctx->allocator;
    auto esvalueIdent = util::NodeAllocator::Alloc<ir::Identifier>(allocator, Signatures::ESVALUE, allocator);
    auto loadOp = util::NodeAllocator::Alloc<ir::Identifier>(allocator, Signatures::LOAD, allocator);
    auto memberExpr = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, esvalueIdent, loadOp, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    ArenaVector<ir::Expression *> arguments(allocator->Adapter());
    auto ohmurlPath = util::NodeAllocator::Alloc<ir::StringLiteral>(allocator, util::UString(ohmUrl, allocator).View());
    arguments.emplace_back(ohmurlPath);
    auto loweredCallExpr = util::NodeAllocator::ForceSetParent<ir::CallExpression>(
        allocator, memberExpr, std::move(arguments), nullptr, false);
    loweredCallExpr->SetParent(callExpr->Parent());
    return loweredCallExpr;
}

static ir::AstNode *TransformESValueLoadCallExpression(ir::CallExpression *callExpr, public_lib::Context *ctx,
                                                       parser::Program *program)
{
    auto importPath = callExpr->Arguments().front();
    if (!importPath->IsStringLiteral()) {
        return callExpr;
    }
    auto *parser = ctx->parser->AsETSParser();
    auto metaData = parser->GetImportPathManager()->GatherImportMetadata(program, util::ImportFlags::NONE,
                                                                         importPath->AsStringLiteral());
    if (metaData.ohmUrl == util::ImportPathManager::DUMMY_PATH) {
        return callExpr;
    }
    return CreateModuleCallExpressionForDynamic(ctx, callExpr, metaData.ohmUrl);
}

static ir::AstNode *TransformInitModuleCallExpression(ir::CallExpression *callExpr, public_lib::Context *ctx,
                                                      parser::Program *program)
{
    auto *parser = ctx->parser->AsETSParser();
    auto *allocator = ctx->allocator;
    auto metaData = parser->GetImportPathManager()->GatherImportMetadata(
        program, util::ImportFlags::NONE, callExpr->Arguments().front()->AsStringLiteral());

    bool isSimultaneous = ctx->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE;
    auto sources = isSimultaneous ? ctx->parserProgram->ExternalSources() : program->DirectExternalSources();
    auto dependentProg = SearchExternalProgramInImport(sources, metaData);
    if (dependentProg == nullptr) {
        dependentProg = SearchExternalProgramInImport(program->ExternalSources(), metaData);
    }
    if (dependentProg == nullptr) {
        if (program->AbsoluteName() == metaData.resolvedSource || program->AbsoluteName() == metaData.declPath) {
            dependentProg = program;
        } else {
            // Replace the broken "InitModule" expression with error node. The error message has been logged in parser.
            ES2PANDA_ASSERT(ctx->diagnosticEngine->IsAnyError());
            auto node = util::NodeAllocator::Alloc<ir::Identifier>(allocator, allocator);
            node->SetRange(callExpr->Range());
            node->SetParent(callExpr->Parent());
            return node;
        }
    }

    if (dependentProg->IsDeclForDynamicStaticInterop()) {
        return CreateModuleCallExpressionForDynamic(ctx, callExpr, metaData.ohmUrl);
    }

    ArenaVector<ir::Expression *> params(allocator->Adapter());
    auto moduleStr = util::UString {
        dependentProg->ModuleInfo().modulePrefix.Mutf8().append(compiler::Signatures::ETS_GLOBAL), allocator};
    auto moduleName = util::NodeAllocator::Alloc<ir::StringLiteral>(allocator, moduleStr.View());

    params.emplace_back(moduleName);
    // Note (daizihan): #27086, we should not use stringLiteral as argument in ETSIntrinsicNode, should be TypeNode.
    auto moduleNode = util::NodeAllocator::Alloc<ir::ETSIntrinsicNode>(allocator, "typereference", std::move(params));
    auto initIdent =
        util::NodeAllocator::Alloc<ir::Identifier>(allocator, compiler::Signatures::CLASS_INITIALIZE_METHOD, allocator);
    auto callee = util::NodeAllocator::Alloc<ir::MemberExpression>(
        allocator, moduleNode, initIdent, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    auto const newCallExpr = util::NodeAllocator::Alloc<ir::CallExpression>(
        allocator, callee, ArenaVector<ir::Expression *>(allocator->Adapter()), nullptr, false, false);
    newCallExpr->SetParent(callExpr->Parent());
    return newCallExpr;
}

bool InitModuleLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx, program](checker::AstNodePtr node) -> checker::AstNodePtr {
            if (IsInitModuleCall(node)) {
                return TransformInitModuleCallExpression(node->AsCallExpression(), ctx, program);
            }
            if (IsESValueLoadCall(node)) {
                return TransformESValueLoadCallExpression(node->AsCallExpression(), ctx, program);
            }
            return node;
        },
        Name());
    return true;
}

}  // namespace ark::es2panda::compiler