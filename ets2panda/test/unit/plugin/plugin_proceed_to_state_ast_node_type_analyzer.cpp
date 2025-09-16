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

#include <iostream>
#include <string>
#include <cstring>
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static es2panda_Context *ctx = nullptr;

static std::string source = R"(
interface UserProfile {
    id: number;
    address: string;
}

class Demo {
    private value = new Array<string>();
    private strLiteral: "Hello" = "Hello";

    method (arg: (string | number)) {
    }

    getInstance (): this {
        return this;
    }

    run () {
        return () => { console.log(this.value) }
    }
}

function Log(msg: string): void {
    console.log(msg);
}

function identity<T>(arg: T): T {
    return arg;
}

const demo = new Demo();
export { demo };
)";

struct NodeCounter {
    int classPropertyCount = 0;
    int newExpressionCount = 0;
    int functionExpressionCount = 0;
    int arrowFunctionCount = 0;
    int blockStatementCount = 0;
    int exportSpecifierCount = 0;
    int tsInterfaceCount = 0;
    int tsTypeParamDeclCount = 0;
    int tsThisTypeCount = 0;
    int etsStringLiteralTypeCount = 0;
};

static NodeCounter counter;

static void ProcessClassProperty(es2panda_Context *context, es2panda_AstNode *node)
{
    counter.classPropertyCount++;
    auto *key = impl->PropertyKey(context, node);
    if (key != nullptr && impl->IsIdentifier(key)) {
        const char *name = impl->IdentifierName(context, key);
        if (name != nullptr) {
            // Print classProperty
            std::cout << "Detected classProperty: " << name << std::endl;
        }

        auto *value = impl->PropertyValue(context, node);
        if (value && impl->IsETSNewClassInstanceExpression(value)) {
            auto *typeRef = impl->ETSTypeReferencePartName(context, value);
            auto *part = impl->ETSTypeReferencePart(context, typeRef);
            auto *typeName = impl->ETSTypeReferencePartName(context, part);
            if (impl->IsIdentifier(typeName)) {
                const char *className = impl->IdentifierName(context, typeName);
                // Print classProperty
                std::cout << " - Property initialized with new " << className << "()" << std::endl;
            }
        }
    }
}

static void ProcessTSInterface(es2panda_Context *context, es2panda_AstNode *node)
{
    counter.tsInterfaceCount++;
    auto *id = impl->TSInterfaceDeclarationId(context, node);
    if (id && impl->IsIdentifier(id)) {
        const char *name = impl->IdentifierName(context, id);
        // Print TSInterface
        std::cout << "Detected TSInterface: " << name << std::endl;
    }
}

static void ProcessNewExpression(es2panda_Context *context, es2panda_AstNode *node)
{
    counter.newExpressionCount++;
    auto *typeRef = node ? impl->ETSTypeReferencePartName(context, node) : nullptr;
    if (!typeRef) {
        return;
    }

    auto *part = impl->ETSTypeReferencePart(context, typeRef);
    if (!part) {
        return;
    }

    auto *typeName = impl->ETSTypeReferencePartName(context, part);
    if (typeName && impl->IsIdentifier(typeName)) {
        const char *className = impl->IdentifierName(context, typeName);
        if (className) {
            // Print newExpression
            std::cout << "Detected newExpression: new " << className << "()" << std::endl;
        }
    }
}

static void ProcessFunctionExpression(es2panda_Context *context, es2panda_AstNode *node)
{
    counter.functionExpressionCount++;
    auto *func = impl->FunctionExpressionFunction(context, node);
    auto *id = impl->ScriptFunctionId(context, func);
    if (id && impl->IsIdentifier(id)) {
        const char *funcName = impl->IdentifierName(context, id);
        if (funcName && std::strcmp(funcName, "constructor") != 0) {
            // Print functionExpression
            std::cout << "Detected functionExpression: " << funcName << "()" << std::endl;
        }
    }
}

static void ProcessCommonNodes(es2panda_AstNode *node)
{
    if (impl->IsArrowFunctionExpression(node)) {
        counter.arrowFunctionCount++;
        // Found arrowFunction
        std::cout << "Detected arrowFunction: () => { ... }" << std::endl;
    } else if (impl->IsBlockStatement(node)) {
        counter.blockStatementCount++;
        // Found blockStatement
        std::cout << "Detected blockStatement: { ... }" << std::endl;
    } else if (impl->IsTSTypeParameterDeclaration(node)) {
        counter.tsTypeParamDeclCount++;
        // Found typeParamDecl
        std::cout << "Detected typeParamDecl " << std::endl;
    } else if (impl->IsTSThisType(node)) {
        counter.tsThisTypeCount++;
        // Found thisType
        std::cout << "Detected thisType " << std::endl;
    } else if (impl->IsETSStringLiteralType(node)) {
        counter.etsStringLiteralTypeCount++;
        // Found stringLiteralType
        std::cout << "Detected stringLiteralType " << std::endl;
    }
}

static void DetectNodeTypes(es2panda_Context *context, es2panda_AstNode *node)
{
    if (node == nullptr || context == nullptr) {
        return;
    }

    if (impl->IsClassProperty(node)) {
        ProcessClassProperty(context, node);
    } else if (impl->IsTSInterfaceDeclaration(node)) {
        ProcessTSInterface(context, node);
    } else if (impl->IsETSNewClassInstanceExpression(node)) {
        ProcessNewExpression(context, node);
    } else if (impl->IsExportNamedDeclaration(node)) {
        size_t exportCount = 0;
        auto **exports = impl->ExportNamedDeclarationSpecifiersConst(context, node, &exportCount);
        counter.exportSpecifierCount += exportCount;
        for (size_t i = 0; i < exportCount; i++) {
            if (!impl->IsExportSpecifier(exports[i])) {
                continue;
            }
            auto *spec = exports[i];
            if (impl->IsIdentifier(spec)) {
                const char *name = impl->IdentifierName(context, spec);
                // Print exportNamedDeclaration
                std::cout << "Detected exportNamedDeclaration: " << name << std::endl;
            }
        }
    } else if (impl->IsFunctionExpression(node)) {
        ProcessFunctionExpression(context, node);
    } else {
        ProcessCommonNodes(node);
    }
}

static void TraverseAST(es2panda_Context *context, es2panda_AstNode *ast)
{
    if (ast == nullptr) {
        return;
    }

    DetectNodeTypes(context, ast);

    impl->AstNodeTransformChildrenRecursively(
        context, ast,
        [](es2panda_AstNode *node) {
            DetectNodeTypes(ctx, node);
            return node;
        },
        const_cast<char *>("NodeTypeAnalysis"));
}

static void PrintStatistics()
{
    // Print node statistics
    std::cout << "\n===== Node statistics =====" << std::endl;
    std::cout << "ClassPropertyCount: " << counter.classPropertyCount << std::endl;
    std::cout << "NewExpressionCount: " << counter.newExpressionCount << std::endl;
    std::cout << "FunctionExpressionCount: " << counter.functionExpressionCount << std::endl;
    std::cout << "ArrowFunctionCount: " << counter.arrowFunctionCount << std::endl;
    std::cout << "BlockStatementCount: " << counter.blockStatementCount << std::endl;
    std::cout << "ExportSpecifierCount: " << counter.exportSpecifierCount << std::endl;
    std::cout << "TsInterfaceCount: " << counter.tsInterfaceCount << std::endl;
    std::cout << "TsTypeParamDeclCount: " << counter.tsTypeParamDeclCount << std::endl;
    std::cout << "TsTypeParamDeclCount: " << counter.tsThisTypeCount << std::endl;
    std::cout << "EtsStringLiteralTypeCount: " << counter.etsStringLiteralTypeCount << std::endl;
    std::cout << "========================" << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    ctx = context;

    impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", ctx);

    auto *root = impl->ProgramAst(ctx, impl->ContextProgram(context));

    TraverseAST(ctx, root);

    PrintStatistics();

    impl->ProceedToState(ctx, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", ctx);

    impl->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", ctx);

    impl->AstNodeRecheck(ctx, root);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", ctx);

    impl->ProceedToState(ctx, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", ctx);

    impl->ProceedToState(ctx, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", ctx);

    impl->DestroyContext(ctx);
    impl->DestroyConfig(config);
    return 0;
}

// NOLINTEND