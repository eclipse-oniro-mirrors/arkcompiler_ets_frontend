/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "generate_constructor.h"
#include "internal_api.h"
#include "compiler/lowering/util.h"
#include "public/public.h"
#include "class_hierarchy.h"
#include "completions.h"
#include "quick_info.h"

namespace ark::es2panda::lsp {

ir::AstNode *GetConstructorNode(const ir::AstNode *classNode)
{
    size_t start = classNode->Start().index;
    size_t end = classNode->End().index;
    ir::AstNode *constructorNode = classNode->FindChild([start, end](ir::AstNode *node) {
        if (node == nullptr) {
            return false;
        }
        return node->Start().index >= start && node->End().index <= end && node->IsConstructor();
    });

    return constructorNode;
}

std::vector<ir::AstNode *> GetClassProperties(ir::AstNode *classNode, const std::vector<std::string> &properties)
{
    std::vector<ir::AstNode *> classProperties = {};
    auto bodyNodes = classNode->AsClassDeclaration()->Definition()->Body();
    for (const auto &triggerWord : properties) {
        auto property = ark::es2panda::lsp::FilterFromBody(bodyNodes, triggerWord);
        for (const auto &node : property) {
            if (node->IsStatic() || !node->IsClassProperty()) {
                continue;
            }
            if (classNode->AsClassDeclaration()->Definition()->IsAbstract() && node->IsAbstract()) {
                continue;
            }
            classProperties.emplace_back(node);
        }
    }

    return classProperties;
}

std::vector<ir::AstNode *> GetExtendedClassProperties(ir::AstNode *classNode)
{
    auto baseNode = ark::es2panda::lsp::GetEffectiveBaseTypeNode(classNode);
    if (baseNode == nullptr) {
        return {};
    }

    auto constructorNode = GetConstructorNode(baseNode);
    if (constructorNode == nullptr) {
        return {};
    }

    std::vector<ir::AstNode *> extendedClassProperties;
    auto params = constructorNode->AsMethodDefinition()
                      ->Value()
                      ->AsFunctionExpression()
                      ->Function()
                      ->AsScriptFunction()
                      ->Params();
    for (auto param : params) {
        auto id = param->AsETSParameterExpression()->Ident();
        auto tmp = compiler::DeclarationFromIdentifier(id);
        extendedClassProperties.emplace_back(tmp);
    }

    return extendedClassProperties;
}

void RemoveTrailingChar(std::string &str, const std::string &lastChar)
{
    if (!str.empty()) {
        size_t lastPos = str.find_last_of(lastChar);
        if (lastPos != std::string::npos) {
            str.erase(lastPos);
        }
    }
}

std::string FilterSubstring(const std::string &input, const std::string &toRemove)
{
    std::string result = input;
    size_t pos = 0;

    while ((pos = result.find(toRemove)) != std::string::npos) {
        result.erase(pos, toRemove.length());
    }

    return result;
}

std::string GetFunctionBody(const std::vector<std::string> &strVec, bool isSuper)
{
    std::string functionBody;
    if (isSuper) {
        functionBody += "  super(";
        for (const auto &str : strVec) {
            functionBody += str;
            functionBody += ", ";
        }
        RemoveTrailingChar(functionBody, ",");
        functionBody += ");\n";
    } else {
        for (const auto &str : strVec) {
            functionBody += "  this.";
            functionBody += str;
            functionBody += " = ";
            functionBody += str;
            functionBody += ";\n";
        }
    }

    return functionBody;
}

std::string GetNameForFunctionExpression(const ir::Expression *type)
{
    auto function = type->AsArrowFunctionExpression()->Function();
    if (function == nullptr || !function->IsScriptFunction()) {
        return "undefined";
    }

    std::string returnType;
    auto statements = function->AsScriptFunction()->Body()->AsBlockStatement()->Statements();
    if (!statements.empty()) {
        auto argType = statements.at(0)->AsReturnStatement()->Argument();
        if (argType->IsStringLiteral()) {
            returnType = "String";
        } else if (argType->IsNumberLiteral()) {
            returnType = "Number";
        } else if (argType->IsBooleanLiteral()) {
            returnType = "Boolean";
        } else {
            returnType = "void";
        }
    }

    return "(() => " + returnType + ")";
}

std::string GetNameForValue(const ir::AstNode *propertyNode)
{
    auto valueType = propertyNode->AsClassProperty()->Value();
    if (valueType == nullptr) {
        return "undefined";
    }

    if (valueType->IsStringLiteral()) {
        return "String";
    }
    if (valueType->IsNumberLiteral()) {
        return "Number";
    }
    if (valueType->IsBooleanLiteral()) {
        return "Boolean";
    }
    if (valueType->IsArrowFunctionExpression()) {
        return GetNameForFunctionExpression(valueType);
    }

    return "undefined";
}

void GetParameterListAndFunctionBody(std::string &parameterList, std::string &functionBody,
                                     const std::vector<ir::AstNode *> &nodeList, bool isSuper)
{
    std::vector<std::string> strVec = {};
    for (auto propertyNode : nodeList) {
        auto nodeName = GetIdentifierName(propertyNode);
        auto propertyName = FilterSubstring(nodeName, "<property>");
        ark::es2panda::ir::TypeNode *typeAnnotation = nullptr;
        if (propertyNode->IsETSParameterExpression()) {
            typeAnnotation = propertyNode->AsETSParameterExpression()->TypeAnnotation();
        } else if (propertyNode->IsClassProperty()) {
            typeAnnotation = propertyNode->AsClassProperty()->TypeAnnotation();
        }

        std::string propertyType;
        if (typeAnnotation == nullptr) {
            propertyType = GetNameForValue(propertyNode);
        } else {
            propertyType = GetNameForTypeNode(typeAnnotation);
        }

        auto str = propertyName;
        str += ": ";
        str += propertyType;
        str += ", ";

        if (parameterList.find(str) == std::string::npos) {
            parameterList += str;
        }
        strVec.push_back(propertyName);
    }

    auto body = GetFunctionBody(strVec, isSuper);
    functionBody += body;
}

bool HasBaseNode(ir::AstNode *classNode)
{
    return nullptr != ark::es2panda::lsp::GetEffectiveBaseTypeNode(classNode);
}

std::string CollectConstructorInfo(ir::AstNode *classNode, const std::vector<ir::AstNode *> &classProperties,
                                   const std::vector<ir::AstNode *> &extendedClassProperties)
{
    std::string constructorInfoText = "constructor(";
    std::string parameterList;
    std::string functionBody;

    if (HasBaseNode(classNode)) {
        GetParameterListAndFunctionBody(parameterList, functionBody, extendedClassProperties, true);
    }
    GetParameterListAndFunctionBody(parameterList, functionBody, classProperties, false);
    RemoveTrailingChar(parameterList, ",");
    constructorInfoText += parameterList;
    constructorInfoText += ") {\n";
    constructorInfoText += functionBody;
    constructorInfoText += "}";
    return constructorInfoText;
}

void GetInsertNodePosition(ir::AstNode *classNode, size_t &insertPosition)
{
    if (classNode == nullptr || !classNode->IsClassDeclaration()) {
        return;
    }

    bool isExitProperty = false;
    auto classBody = classNode->AsClassDeclaration()->Definition()->Body();
    for (auto node : classBody) {
        if (node->IsClassProperty() && !isExitProperty) {
            insertPosition = node->AsClassProperty()->Start().index;
            isExitProperty = true;
            break;
        }
    }

    if (!isExitProperty) {
        const int offset = 2;
        insertPosition = classNode->End().index - offset;
    }
}

std::vector<FileTextChanges> GetRefactorActionsToGenerateConstructor(es2panda_Context *context, size_t position,
                                                                     const std::vector<std::string> &properties)
{
    if (context == nullptr) {
        return {};
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return {};
    }

    ir::AstNode *classDeclaration = ark::es2panda::lsp::GetTargetDeclarationNodeByPosition(context, position);
    if (!IsDefinedClassOrStruct(classDeclaration)) {
        return {};
    }

    if (GetConstructorNode(classDeclaration) != nullptr) {
        return {};
    }

    std::vector<ir::AstNode *> classProperties = GetClassProperties(classDeclaration, properties);
    std::vector<ir::AstNode *> extendedClassProperties = GetExtendedClassProperties(classDeclaration);

    std::string text = CollectConstructorInfo(classDeclaration, classProperties, extendedClassProperties);
    size_t insertPosition = 0;
    GetInsertNodePosition(classDeclaration, insertPosition);

    std::vector<FileTextChanges> fileTextChanges;
    TextSpan span(insertPosition, text.size());
    std::vector<TextChange> textChanges;
    textChanges.emplace_back(TextChange(span, text));
    auto fileName = ctx->sourceFileName;
    FileTextChanges textChange(fileName, textChanges);
    fileTextChanges.push_back(textChange);

    return fileTextChanges;
}

}  // namespace ark::es2panda::lsp