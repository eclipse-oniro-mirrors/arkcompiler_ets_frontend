/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "class_hierarchy_info.h"
#include "internal_api.h"
#include "public/public.h"
#include "compiler/lowering/util.h"
#include "quick_info.h"

namespace ark::es2panda::lsp {
std::string GetNameFromIdentifierNode(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return "";
    }
    return node->AsIdentifier()->ToString();
}

// Currently only considering enum scenarios.
bool IsClassLiteralDefinition(const ir::AstNode *node)
{
    return !(compiler::ClassDefinitionIsEnumTransformed(node));
}

ir::ClassDefinition *GetClassDefinitionFromIdentifierNode(const ir::AstNode *node)
{
    auto decl = compiler::DeclarationFromIdentifier(node->AsIdentifier());
    if (decl == nullptr) {
        return nullptr;
    }
    if (decl->IsClassDeclaration()) {
        decl = decl->AsClassDeclaration()->Definition();
    }
    if (!IsClassLiteralDefinition(decl)) {
        return nullptr;
    }
    return decl->AsClassDefinition();
}

std::string SpliceFunctionDetailStr(const std::string &functionName, const std::vector<FunctionParamStyle> &params,
                                    const std::string &returnType)
{
    std::string result;
    if (functionName.empty()) {
        return result;
    }
    result.append(functionName).append("(");
    auto iter = params.cbegin();
    while (iter != params.cend()) {
        auto name = iter->GetParamName();
        auto kind = iter->GetParamKind();
        if (name.empty() || kind.empty()) {
            ++iter;
            continue;
        }
        result.append(name).append(": ").append(kind);
        ++iter;
        if (iter != params.cend()) {
            result.append(", ");
        }
    }
    result.append(")");
    if (!returnType.empty()) {
        result.append(": ").append(returnType);
    }
    return result;
}

std::string SplicePropertyDetailStr(const std::string &name, const std::string &type)
{
    if (name.empty() || type.empty()) {
        return "";
    }
    return name + ": " + type;
}

std::string GetFunctionNameFromScriptFunction(const ir::ScriptFunction *function)
{
    if (function == nullptr || function->Id() == nullptr) {
        return "";
    }
    return function->Id()->ToString();
}

std::vector<FunctionParamStyle> GetParamListFromScriptFunction(const ir::ScriptFunction *function)
{
    std::vector<FunctionParamStyle> params;
    if (function == nullptr) {
        return params;
    }
    auto nodeParams = function->Params();
    for (const auto *it : nodeParams) {
        if (it == nullptr || !it->IsETSParameterExpression()) {
            continue;
        }
        std::string paramName;
        std::string paramKind;
        auto nodeParam = it->AsETSParameterExpression();
        if (nodeParam->IsRestParameter()) {
            paramName = nodeParam->RestParameter()->ToString();
        } else {
            paramName = GetNameFromIdentifierNode(nodeParam->Ident());
        }
        if (paramName == INVALID_EXPRESSION || nodeParam->TypeAnnotation() == nullptr) {
            continue;
        }
        paramKind = GetNameForTypeNode(nodeParam->TypeAnnotation());
        params.emplace_back(FunctionParamStyle(std::move(paramName), std::move(paramKind)));
    }
    return params;
}

std::string GetReturnTypeFromScriptFunction(const ir::ScriptFunction *function)
{
    if (function == nullptr) {
        return "";
    }
    auto returnNode = function->ReturnTypeAnnotation();
    if (returnNode == nullptr) {
        return "";
    }
    auto returnType = GetNameForTypeNode(returnNode);
    return returnType;
}

SetterStyle CreateSetterStyle(ir::MethodDefinitionKind kind)
{
    SetterStyle setter = SetterStyle::NONE;
    switch (kind) {
        case ir::MethodDefinitionKind::GET:
        case ir::MethodDefinitionKind::EXTENSION_GET:
            setter = SetterStyle::GETTER;
            break;
        case ir::MethodDefinitionKind::SET:
        case ir::MethodDefinitionKind::EXTENSION_SET:
            setter = SetterStyle::SETTER;
            break;
        default:
            break;
    }
    return setter;
}

std::shared_ptr<ClassMethodItem> CreateClassMethodItem(const ir::MethodDefinition *methodDefinition,
                                                       const std::string &funcName, std::string detail)
{
    if (methodDefinition == nullptr || funcName.empty() || detail.empty()) {
        return nullptr;
    }
    auto setter = CreateSetterStyle(methodDefinition->Kind());
    AccessModifierStyle access = AccessModifierStyle::PUBLIC;
    if (methodDefinition->IsProtected()) {
        access = AccessModifierStyle::PROTECTED;
    }
    auto item = std::make_shared<ClassMethodItem>(access, std::move(detail), setter);
    item->SetFunctionName(funcName);
    return item;
}

std::shared_ptr<ClassPropertyItem> CreateClassPropertyItem(const ir::ClassProperty *property,
                                                           const std::string &propertyName, std::string detail)
{
    if (property == nullptr || propertyName.empty() || detail.empty()) {
        return nullptr;
    }
    AccessModifierStyle access = AccessModifierStyle::PUBLIC;
    if (property->IsProtected()) {
        access = AccessModifierStyle::PROTECTED;
    }
    auto item = std::make_shared<ClassPropertyItem>(access, std::move(detail));
    item->SetVariableName(propertyName);
    return item;
}

std::shared_ptr<ClassMethodItem> ParseFunctionStyleWithCreateItem(const ir::MethodDefinition *methodDefinition,
                                                                  bool isCurrentToken)
{
    if (methodDefinition == nullptr) {
        return nullptr;
    }
    if ((isCurrentToken && methodDefinition->IsStatic()) ||
        (!isCurrentToken &&
         (methodDefinition->IsPrivate() || methodDefinition->IsStatic() || methodDefinition->IsConstructor()))) {
        return nullptr;
    }
    auto function = methodDefinition->Function();
    auto functionName = GetFunctionNameFromScriptFunction(function);
    if (functionName.empty()) {
        return nullptr;
    }
    auto paramList = GetParamListFromScriptFunction(function);
    auto returnType = GetReturnTypeFromScriptFunction(function);
    auto functionDetail = SpliceFunctionDetailStr(functionName, paramList, returnType);
    return CreateClassMethodItem(methodDefinition, functionName, functionDetail);
}

ir::Identifier *GetIdentFromNewClassExprPart(const ir::Expression *value)
{
    if (value == nullptr || !value->IsETSNewClassInstanceExpression()) {
        return nullptr;
    }
    auto typeRef = value->AsETSNewClassInstanceExpression()->GetTypeRef();
    if (typeRef == nullptr || !typeRef->IsETSTypeReference()) {
        return nullptr;
    }
    auto part = typeRef->AsETSTypeReference()->Part();
    if (part == nullptr) {
        return nullptr;
    }
    return part->GetIdent();
}

std::shared_ptr<ClassPropertyItem> ParsePropertyStyleWithCreateItem(const ir::ClassProperty *property,
                                                                    bool isCurrentToken)
{
    if (property == nullptr) {
        return nullptr;
    }
    if ((isCurrentToken && property->IsStatic()) ||
        (!isCurrentToken && (property->IsPrivate() || property->IsStatic()))) {
        return nullptr;
    }
    std::string propertyName = GetNameFromIdentifierNode(property->Key());
    std::string type;
    if (property->TypeAnnotation() == nullptr) {
        auto value = property->Value();
        auto ident = GetIdentFromNewClassExprPart(value);
        type = GetNameFromIdentifierNode(ident);
    } else {
        type = GetNameForTypeNode(property->TypeAnnotation());
    }
    if (propertyName == INVALID_EXPRESSION || type == INVALID_EXPRESSION) {
        return nullptr;
    }
    auto detail = SplicePropertyDetailStr(propertyName, type);
    return CreateClassPropertyItem(property, propertyName, detail);
}

ClassHierarchyInfo CreateClassHierarchyInfoFromBody(const ir::ClassDefinition *classDefinition,
                                                    const std::string &className, bool isCurrentToken)
{
    ClassHierarchyInfo result;
    if (classDefinition == nullptr) {
        return result;
    }
    result.SetClassName(className);
    auto bodyNodes = classDefinition->Body();
    for (const auto &node : bodyNodes) {
        if (node == nullptr) {
            continue;
        }
        if (node->IsMethodDefinition()) {
            auto methodDefinition = node->AsMethodDefinition();
            if (methodDefinition == nullptr) {
                continue;
            }
            auto item = ParseFunctionStyleWithCreateItem(methodDefinition, isCurrentToken);
            result.AddItemToMethodList(item);
            auto overLoads = methodDefinition->Overloads();
            for (const auto *overLoadMethodDefinition : overLoads) {
                auto overLoadItem = ParseFunctionStyleWithCreateItem(overLoadMethodDefinition, isCurrentToken);
                result.AddItemToMethodList(overLoadItem);
            }
        } else if (node->IsClassProperty()) {
            auto property = node->AsClassProperty();
            if (property == nullptr) {
                continue;
            }
            auto item = ParsePropertyStyleWithCreateItem(property, isCurrentToken);
            result.AddItemToPropertyList(item);
        }
    }
    return result;
}

ir::AstNode *GetSuperClassNode(const ir::ClassDefinition *classDefinition)
{
    if (classDefinition == nullptr) {
        return nullptr;
    }
    auto super = const_cast<ir::Expression *>(classDefinition->Super());
    if (super == nullptr) {
        return nullptr;
    }
    return GetIdentifierFromSuper(super);
}

void ComputeClassHierarchyInfo(const ClassHierarchyInfo &deriveInfo, ClassHierarchyInfo &superInfo)
{
    auto deriveMethods = deriveInfo.GetMethodItemList();
    for (const auto &method : deriveMethods) {
        superInfo.DeleteTargetItemInMethodList(method.second);
    }
    auto deriveProperties = deriveInfo.GetPropertyItemList();
    for (const auto &property : deriveProperties) {
        superInfo.DeleteTargetItemInPropertyList(property.second);
    }
}

void FillBaseClassHierarchyInfo(const ClassHierarchyInfo &extraInfo, ClassHierarchyInfo &baseInfo)
{
    auto extraMethods = extraInfo.GetMethodItemList();
    for (const auto &method : extraMethods) {
        baseInfo.AddItemToMethodList(method.second);
    }
    auto extraProperties = extraInfo.GetPropertyItemList();
    for (const auto &property : extraProperties) {
        baseInfo.AddItemToPropertyList(property.second);
    }
}

void ProcessClassHierarchy(const ir::AstNode *token, ClassHierarchyInfo &baseInfo, ClassHierarchy &result)
{
    if (token == nullptr || !token->IsIdentifier()) {
        return;
    }
    std::string className = GetNameFromIdentifierNode(token);
    auto classDefinition = GetClassDefinitionFromIdentifierNode(token);
    if (classDefinition == nullptr) {
        return;
    }
    auto info = CreateClassHierarchyInfoFromBody(classDefinition, className, false);
    if (!className.empty()) {
        // Calculate the difference between the obtained parent class info and the current clicked node class info.
        ComputeClassHierarchyInfo(baseInfo, info);
        if (info.GetName() == className && (!info.GetMethodItemList().empty() || !info.GetPropertyItemList().empty())) {
            result.emplace_back(info);
            FillBaseClassHierarchyInfo(info, baseInfo);
        }
    }
    auto superClass = GetSuperClassNode(classDefinition);
    if (superClass == nullptr) {
        return;
    }
    ProcessClassHierarchy(superClass, baseInfo, result);
}

ir::AstNode *GetTargetClassDeclarationByPosition(es2panda_Context *context, size_t position)
{
    if (context == nullptr) {
        return nullptr;
    }
    auto token = GetTouchingToken(context, position, false);
    auto tmp = token;
    while (tmp != nullptr) {
        if (tmp->IsClassDeclaration()) {
            return tmp;
        }
        tmp = tmp->Parent();
    }
    return nullptr;
}

ClassHierarchy GetClassHierarchyInfoImpl(es2panda_Context *context, size_t position)
{
    ClassHierarchy result;
    auto classDeclaration = GetTargetClassDeclarationByPosition(context, position);
    if (classDeclaration == nullptr) {
        return result;
    }
    auto classDefinition = classDeclaration->AsClassDeclaration()->Definition();
    auto currentInfo = CreateClassHierarchyInfoFromBody(classDefinition, "", true);
    auto superClass = GetSuperClassNode(classDefinition);
    if (superClass == nullptr) {
        return result;
    }
    ProcessClassHierarchy(superClass, currentInfo, result);
    return result;
}
}  // namespace ark::es2panda::lsp
