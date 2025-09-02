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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::Initializer;

// CC-OFF(G.NAM.03-CPP) project code style
auto g_fileSource =
    "let a: number;\nlet b: byte;\nlet c: short;\nlet d: int;\nlet e: long;\nlet f: "
    "float;\nlet g: double;\nlet h: char;\nlet i: boolean;";

// CC-OFFNXT(huge_method) test function
TEST_F(LSPAPITests, GetTypeOfSymbolAtLocation1)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("types.ets", ES2PANDA_STATE_CHECKED, g_fileSource);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto checker = reinterpret_cast<Context *>(ctx)->GetChecker()->AsETSChecker();
    auto astNode = GetAstFromContext<AstNode>(ctx);
    auto targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "a"; });
    auto type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    std::cout << type->ToString() << std::endl;
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalDoubleBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "b"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalByteBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "c"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalShortBuiltinType()));
    ;

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "d"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalIntBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "e"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalLongBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "f"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalFloatBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "g"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalDoubleBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "h"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalCharBuiltinType()));

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "i"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(checker->Relation()->IsIdenticalTo(type, checker->GlobalETSBooleanBuiltinType()));
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, GetTypeOfSymbolAtLocation2)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "types.ets", ES2PANDA_STATE_CHECKED,
        "let j: object;\nlet k: string;\nlet l: [];\nlet m: bigint;\nlet n: never;\nlet o: null;\nlet p: "
        "undefined;\nlet tuple: [number, number] = [1, 2];\nlet union: int | null;");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto checker = reinterpret_cast<Context *>(ctx)->GetChecker()->AsETSChecker();
    auto astNode = GetAstFromContext<AstNode>(ctx);
    auto targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "j"; });
    auto type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSObjectType());

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "k"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSStringType());

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "l"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSTupleType());

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "m"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSBigIntType());

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "n"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSNeverType());

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "o"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSNullType());

    targetNode =
        astNode->FindChild([](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "p"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSUndefinedType());

    targetNode = astNode->FindChild(
        [](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "tuple"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSTupleType());

    targetNode = astNode->FindChild(
        [](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "union"; });
    type = ark::es2panda::lsp::GetTypeOfSymbolAtLocation(checker, targetNode);
    ASSERT_TRUE(type->IsETSUnionType());
    initializer.DestroyContext(ctx);
}

}  // namespace
