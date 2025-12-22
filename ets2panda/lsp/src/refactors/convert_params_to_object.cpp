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

#include "refactors/convert_params_to_object.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include "compiler/lowering/util.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "refactor_provider.h"
#include "refactors/refactor_types.h"
#include "services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

static constexpr size_t K_FIRST_PARAMETER_INDEX = 0;
static constexpr size_t K_SECOND_PARAMETER_INDEX = 1;

ConvertParamsToObjectRefactor::ConvertParamsToObjectRefactor()
{
    AddKind(std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind));
}

static RefactorParameterInfo SetParameterInfo(ir::ETSParameterExpression *param)
{
    RefactorParameterInfo info {};

    auto *ident = param->Ident();
    if (ident != nullptr) {
        info.SetName(std::string(ident->Name()));
    }

    info.SetTypeAnnotation(param->TypeAnnotation());
    info.SetIsOptional(param->IsOptional());
    info.SetDefaultValue(param->Initializer());

    return info;
}

static bool HasThisParameter(const ArenaVector<ir::Expression *> &params)
{
    if (params.empty() || !params[K_FIRST_PARAMETER_INDEX]->IsETSParameterExpression()) {
        return false;
    }

    auto *firstParam = params[K_FIRST_PARAMETER_INDEX]->AsETSParameterExpression();
    auto *ident = firstParam->Ident();
    return ident != nullptr && ident->Name() == "this";
}

static void CollectFunctionParameters(RefactorFunctionInfo &result, const ArenaVector<ir::Expression *> &params)
{
    size_t startIdx = result.HasThisParameter() ? K_SECOND_PARAMETER_INDEX : K_FIRST_PARAMETER_INDEX;

    for (size_t i = startIdx; i < params.size(); ++i) {
        auto *param = params[i];

        if (!param->IsETSParameterExpression()) {
            continue;
        }

        auto *etsParam = param->AsETSParameterExpression();
        if (etsParam->IsRestParameter()) {
            continue;
        }

        auto paramInfo = SetParameterInfo(etsParam);
        if (!paramInfo.Name().empty()) {
            result.Parameters().push_back(paramInfo);
        }
    }
}

static RefactorFunctionInfo GetFunctionAtPosition(const RefactorContext &context)
{
    RefactorFunctionInfo result {};

    auto *token = GetTouchingToken(context.context, context.span.pos, false);
    if (token == nullptr) {
        return result;
    }

    auto *methodDef = FindAncestor(token, [](ir::AstNode *node) { return node->IsMethodDefinition(); });
    if (methodDef == nullptr || !methodDef->IsMethodDefinition()) {
        return result;
    }

    result.SetMethodDef(methodDef->AsMethodDefinition());

    auto *key = result.MethodDef()->Key();
    if (key != nullptr && key->IsIdentifier()) {
        result.SetFunctionName(std::string(key->AsIdentifier()->Name()));
    }

    auto *scriptFunc = result.MethodDef()->Function();
    if (scriptFunc == nullptr) {
        result.SetMethodDef(nullptr);
        return result;
    }

    const auto &params = scriptFunc->Params();
    result.SetHasThisParameter(HasThisParameter(params));
    CollectFunctionParameters(result, params);

    return result;
}

static bool IsValidForRefactor(const RefactorFunctionInfo &funcInfo)
{
    if (funcInfo.MethodDef() == nullptr || funcInfo.FunctionName().empty() || funcInfo.Parameters().empty()) {
        return false;
    }

    for (const auto &param : funcInfo.Parameters()) {
        if (param.TypeAnnotation() == nullptr) {
            return false;
        }
    }

    return true;
}

static ir::CallExpression *TryGetCallExpression(ir::AstNode *node)
{
    auto *parent = node->Parent();
    if (parent == nullptr) {
        return nullptr;
    }

    if (parent->IsCallExpression() && parent->AsCallExpression()->Callee() == node) {
        return parent->AsCallExpression();
    }

    if (parent->IsMemberExpression()) {
        auto *memberParent = parent->Parent();
        if (memberParent != nullptr && memberParent->IsCallExpression() &&
            memberParent->AsCallExpression()->Callee() == parent) {
            return memberParent->AsCallExpression();
        }
    }

    return nullptr;
}

static bool IsMatchingIdentifier(ir::AstNode *node, ir::Identifier *funcId, ir::MethodDefinition *methodDef)
{
    if (!node->IsIdentifier()) {
        return false;
    }

    auto *identifier = node->AsIdentifier();
    if (identifier->Name() != funcId->Name()) {
        return false;
    }

    auto *decl = compiler::DeclarationFromIdentifier(identifier);
    return decl == methodDef;
}

static std::vector<ir::CallExpression *> FindCallSites(es2panda_Context *context, const RefactorFunctionInfo &funcInfo)
{
    std::vector<ir::CallExpression *> callSites;

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return callSites;
    }

    auto *funcId = funcInfo.MethodDef()->Key();
    if (funcId == nullptr || !funcId->IsIdentifier()) {
        return callSites;
    }

    auto *ast = ctx->parserProgram->Ast();
    ast->FindChild([&](ir::AstNode *node) {
        if (!IsMatchingIdentifier(node, funcId->AsIdentifier(), funcInfo.MethodDef())) {
            return false;
        }

        auto *callExpr = TryGetCallExpression(node);
        if (callExpr != nullptr) {
            callSites.push_back(callExpr);
        }

        return false;
    });

    return callSites;
}

static std::string GenerateInterfaceName(const std::string &functionName)
{
    if (functionName.empty()) {
        return "Params";
    }

    std::string result = functionName;
    if (!result.empty()) {
        result[K_FIRST_PARAMETER_INDEX] =
            static_cast<char>(std::toupper(static_cast<unsigned char>(result[K_FIRST_PARAMETER_INDEX])));
    }
    result += "Params";

    return result;
}

static std::string CreateInterfaceCode(const RefactorFunctionInfo &funcInfo)
{
    std::ostringstream oss;
    std::string interfaceName = GenerateInterfaceName(funcInfo.FunctionName());

    oss << "interface " << interfaceName << " {\n";

    for (const auto &param : funcInfo.Parameters()) {
        oss << "    " << param.Name();
        if (param.IsOptional()) {
            oss << "?";
        }
        oss << ": " << param.TypeAnnotation()->DumpEtsSrc() << ";\n";
    }

    oss << "}";

    return oss.str();
}

static std::string CreateDestructuredParameter(const RefactorFunctionInfo &funcInfo)
{
    std::ostringstream oss;
    std::string interfaceName = GenerateInterfaceName(funcInfo.FunctionName());

    oss << "{ ";
    for (size_t i = K_FIRST_PARAMETER_INDEX; i < funcInfo.Parameters().size(); ++i) {
        if (i > K_FIRST_PARAMETER_INDEX) {
            oss << ", ";
        }
        oss << funcInfo.Parameters()[i].Name();
        if (funcInfo.Parameters()[i].DefaultValue() != nullptr) {
            oss << " = " << funcInfo.Parameters()[i].DefaultValue()->DumpEtsSrc();
        }
    }
    oss << " }: " << interfaceName;

    return oss.str();
}

static std::string CreateObjectLiteral(const RefactorFunctionInfo &funcInfo, const ir::CallExpression *callExpr)
{
    std::ostringstream oss;
    const auto &args = callExpr->Arguments();

    oss << "{ ";
    size_t minSize = std::min(args.size(), funcInfo.Parameters().size());
    for (size_t i = K_FIRST_PARAMETER_INDEX; i < minSize; ++i) {
        if (i > K_FIRST_PARAMETER_INDEX) {
            oss << ", ";
        }
        oss << funcInfo.Parameters()[i].Name() << ": " << args[i]->DumpEtsSrc();
    }
    oss << " }";

    return oss.str();
}

static void ApplyChanges(ChangeTracker &tracker, const RefactorContext &context, const RefactorFunctionInfo &funcInfo,
                         const std::vector<ir::CallExpression *> &callSites)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto *sourceFile = ctx->sourceFile;

    std::string interfaceCode = CreateInterfaceCode(funcInfo);
    size_t insertPos = funcInfo.MethodDef()->Start().index;
    tracker.InsertText(sourceFile, insertPos, interfaceCode + "\n\n");

    auto *scriptFunc = funcInfo.MethodDef()->Function();
    if (scriptFunc != nullptr && !funcInfo.Parameters().empty()) {
        const auto &params = scriptFunc->Params();
        size_t firstIdx = funcInfo.HasThisParameter() ? K_SECOND_PARAMETER_INDEX : K_FIRST_PARAMETER_INDEX;
        if (firstIdx < params.size()) {
            size_t firstStart = params[firstIdx]->Start().index;
            size_t lastEnd = params.back()->End().index;

            std::string newParam = CreateDestructuredParameter(funcInfo);
            TextRange paramRange = {firstStart, lastEnd};
            tracker.ReplaceRangeWithText(sourceFile, paramRange, newParam);
        }
    }

    for (const auto *callExpr : callSites) {
        const auto &args = callExpr->Arguments();
        if (args.empty()) {
            continue;
        }

        size_t argsStart = args.front()->Start().index;
        size_t argsEnd = args.back()->End().index;

        std::string objectLiteral = CreateObjectLiteral(funcInfo, callExpr);
        TextRange argRange = {argsStart, argsEnd};
        tracker.ReplaceRangeWithText(sourceFile, argRange, objectLiteral);
    }
}

std::vector<ApplicableRefactorInfo> ConvertParamsToObjectRefactor::GetAvailableActions(
    const RefactorContext &context) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;

    if (!context.kind.empty() && !IsKind(context.kind)) {
        return res;
    }

    auto funcInfo = GetFunctionAtPosition(context);
    if (!IsValidForRefactor(funcInfo)) {
        return res;
    }

    applicableRef.name = refactor_name::CONVERT_PARAMS_TO_OBJECT;
    applicableRef.description = refactor_description::CONVERT_PARAMS_TO_OBJECT_DESC;
    applicableRef.action.name = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.name);
    applicableRef.action.description = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.description);
    applicableRef.action.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    res.push_back(applicableRef);
    return res;
}

std::unique_ptr<RefactorEditInfo> ConvertParamsToObjectRefactor::GetEditsForAction(const RefactorContext &context,
                                                                                   const std::string &actionName) const
{
    if (!actionName.empty() && actionName != CONVERT_PARAMS_TO_OBJECT_ACTION.name) {
        return nullptr;
    }

    auto funcInfo = GetFunctionAtPosition(context);
    if (!IsValidForRefactor(funcInfo)) {
        return nullptr;
    }

    auto callSites = FindCallSites(context.context, funcInfo);
    auto edits = ChangeTracker::With(*context.textChangesContext, [&](ChangeTracker &tracker) {
        ApplyChanges(tracker, context, funcInfo, callSites);
    });
    if (edits.empty()) {
        return nullptr;
    }

    return std::make_unique<RefactorEditInfo>(std::move(edits));
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertParamsToObjectRefactor> g_convertParamsToObjectRefactor("ConvertParamsToObjectRefactor");

}  // namespace ark::es2panda::lsp
