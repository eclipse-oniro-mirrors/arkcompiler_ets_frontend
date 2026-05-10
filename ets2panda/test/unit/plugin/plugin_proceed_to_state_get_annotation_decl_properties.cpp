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

#include <string>
#include <vector>
#include <utility>
#include <map>

#include "libarkbase/os/library_loader.h"

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static std::string source = R"(
@interface Component {
    name: string = "default"
    value: number = 10
}

@Component({name:"ss"})
class A {}
)";

static std::vector<es2panda_AstNode *> classDeclarations;
static std::vector<es2panda_AstNode *> annotationDeclarations;
static std::vector<es2panda_AstNode *> mergedProperties;
static std::vector<es2panda_AstNode *> propertyChildren;

static void CollectPropertyChildren(es2panda_AstNode *node)
{
    propertyChildren.push_back(node);
}

static const char *GetPropertyName(es2panda_Context *context, es2panda_AstNode *property)
{
    if (!impl->IsClassProperty(property)) {
        return nullptr;
    }

    propertyChildren.clear();
    impl->AstNodeIterateConst(context, property, CollectPropertyChildren);

    for (size_t i = 0; i < propertyChildren.size(); i++) {
        auto *child = propertyChildren[i];
        if (impl->IsIdentifier(child)) {
            return impl->IdentifierName(context, child);
        }
    }

    return nullptr;
}

static void CollectDeclarations(es2panda_AstNode *node)
{
    if (impl->IsClassDeclaration(node)) {
        classDeclarations.push_back(node);
    }
    if (impl->IsAnnotationDeclaration(node)) {
        annotationDeclarations.push_back(node);
    }
}

static es2panda_AstNode *FindAnnotationDeclarationByName(es2panda_Context *context, const char *name)
{
    for (auto *decl : annotationDeclarations) {
        auto *expr = impl->AnnotationDeclarationExpr(context, decl);
        if (expr != nullptr && impl->IsIdentifier(expr)) {
            const char *declName = impl->IdentifierName(context, expr);
            if (declName != nullptr && std::string(name) == std::string(declName)) {
                return decl;
            }
        }
    }
    return nullptr;
}

static es2panda_AstNode *GetAnnotationUsageFromClass(es2panda_Context *context, es2panda_AstNode *classDecl)
{
    size_t n;
    auto *classDef = impl->ClassDeclarationDefinition(context, classDecl);
    auto **annotations = impl->ClassDefinitionAnnotations(context, classDef, &n);
    if (n != 1) {
        return nullptr;
    }
    return annotations[0];
}

static es2panda_AstNode **GetAnnotationDeclarationProperties(es2panda_Context *context,
                                                             es2panda_AstNode *annotationUsageNode,
                                                             size_t *propertiesLen)
{
    if (!impl->IsAnnotationUsage(annotationUsageNode)) {
        return nullptr;
    }

    size_t usagePropsLen = 0;
    auto **usageProps = impl->AnnotationUsageIrPropertiesConst(context, annotationUsageNode, &usagePropsLen);

    auto *expr = impl->AnnotationUsageIrExpr(context, annotationUsageNode);
    if (expr == nullptr || !impl->IsIdentifier(expr)) {
        return nullptr;
    }
    const char *annotationName = impl->IdentifierName(context, expr);

    auto *declNode = FindAnnotationDeclarationByName(context, annotationName);
    if (declNode == nullptr) {
        return nullptr;
    }

    size_t declPropsLen = 0;
    auto **declProps = impl->AnnotationDeclarationPropertiesConst(context, declNode, &declPropsLen);
    if (declProps == nullptr) {
        return nullptr;
    }

    mergedProperties.clear();
    for (size_t i = 0; i < declPropsLen; i++) {
        auto *declProp = declProps[i];
        const char *declPropName = GetPropertyName(context, declProp);
        if (declPropName == nullptr) {
            return nullptr;
        }

        es2panda_AstNode *selectedProp = declProp;
        for (size_t j = 0; j < usagePropsLen; j++) {
            const char *usagePropName = GetPropertyName(context, usageProps[j]);
            if (usagePropName != nullptr && std::string(declPropName) == std::string(usagePropName)) {
                selectedProp = usageProps[j];
                break;
            }
        }

        mergedProperties.push_back(selectedProp);
    }

    *propertiesLen = mergedProperties.size();
    return mergedProperties.data();
}

static bool ValidateProperties(es2panda_Context *context, es2panda_AstNode **properties, size_t propertiesLen)
{
    std::vector<std::string> expectedNames = {"name", "value"};
    if (propertiesLen != expectedNames.size()) {
        return false;
    }

    for (size_t i = 0; i < propertiesLen; i++) {
        auto *property = properties[i];
        if (!impl->IsClassProperty(property)) {
            return false;
        }

        const char *name = GetPropertyName(context, property);
        if (name == nullptr) {
            return false;
        }

        if (expectedNames[i] != name) {
            return false;
        }
    }

    return true;
}

static bool TestGetAnnotationDeclarationProperties(es2panda_Context *context)
{
    auto *ast = impl->ProgramAst(context, impl->ContextProgram(context));

    classDeclarations.clear();
    annotationDeclarations.clear();
    impl->AstNodeIterateConst(context, ast, CollectDeclarations);

    if (classDeclarations.empty()) {
        return false;
    }

    auto *annotationUsage = GetAnnotationUsageFromClass(context, classDeclarations.front());
    if (annotationUsage == nullptr) {
        return false;
    }

    size_t propertiesLen = 0;
    auto **properties = GetAnnotationDeclarationProperties(context, annotationUsage, &propertiesLen);
    if (properties == nullptr) {
        return false;
    }

    return ValidateProperties(context, properties, propertiesLen);
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_PARSED] = {TestGetAnnotationDeclarationProperties};

    ProccedToStatePluginTestData data = {argc, argv, &impl, testFunctions, true, source, ES2PANDA_STATE_PARSED};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND