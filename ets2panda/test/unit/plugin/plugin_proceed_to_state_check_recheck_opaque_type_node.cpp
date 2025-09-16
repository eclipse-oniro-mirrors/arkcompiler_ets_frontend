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
#include <ostream>
#include <string>
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN
static std::string prefix = "returns_";

static std::string source = R"(
class C {
}

interface I {
    x: int
}

class Test {
    static returns_void() {
    }

    static returns_int() {
        return 1
    }

    static returns_arrow() {
        return () => {}
    }
    
    static returns_C() {
        return new C()
    }

    static returns_I() {
        return { x: 1 } as I
    }
    
    static returns_union(cond: boolean) {
        if (cond) {
            return 1
        } else {
            return "hello"
        }
    }
}
)";

static es2panda_Impl *impl = nullptr;
static es2panda_Config *config = nullptr;
static es2panda_Context *context = nullptr;

static int countFound = 0;
static int countApplied = 0;

void setScriptFunctionReturnType(es2panda_AstNode *node, [[maybe_unused]] void *arg)
{
    if (impl->IsScriptFunction(node)) {
        es2panda_AstNode *id = impl->ScriptFunctionId(context, node);
        if (!id) {
            return;
        }
        if (std::string(impl->IdentifierNameConst(context, id)).substr(0, prefix.length()) != prefix) {
            return;
        }
        countFound++;
        es2panda_Signature *signature = impl->ScriptFunctionSignature(context, node);
        if (!signature) {
            return;
        }
        es2panda_Type *returnType = impl->SignatureReturnType(context, signature);
        if (!returnType) {
            return;
        }
        es2panda_AstNode *returnTypeAnnotation = impl->CreateOpaqueTypeNode(context, returnType);
        if (!returnTypeAnnotation) {
            return;
        }
        impl->ScriptFunctionSetReturnTypeAnnotation(context, node, returnTypeAnnotation);
        countApplied++;
    }
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    impl = GetImpl();

    const char **args = const_cast<const char **>(&(argv[1]));
    config = impl->CreateConfig(argc - 1, args);
    context = impl->CreateContextFromString(config, source.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    auto *program = impl->ContextProgram(context);
    es2panda_AstNode *ast = impl->ProgramAst(context, program);

    impl->AstNodeForEach(ast, setScriptFunctionReturnType, nullptr);
    if (countFound == 0) {
        return TEST_ERROR_CODE;
    }
    if (countFound != countApplied) {
        return TEST_ERROR_CODE;
    }

    impl->AstNodeRecheck(context, ast);
    CheckForErrors("RECHECKED", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    impl->DestroyContext(context);
    impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND