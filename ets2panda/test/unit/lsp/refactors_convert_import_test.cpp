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
#include "lsp/include/refactors/convert_import.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <string>

namespace {

class LSPConvertImport : public LSPAPITests {};

TEST_F(LSPConvertImport, DefaultToNamed_IdentifierForm2)
{
    const std::string consumer = R"(
import Local from "./DefaultToNamed_IdentifierForm2";
)";
    const std::string mod = R"(
export default const Identifier = 42;
)";

    auto tempFiles =
        CreateTempFile({"DefaultToNamed_IdentifierForm1.ets", "DefaultToNamed_IdentifierForm2.ets"}, {consumer, mod});
    ASSERT_EQ(tempFiles.size(), 2U);

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_BOUND);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor ref;
    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    auto applicable = ref.GetAvailableActions(refactorContext);
    ASSERT_EQ(applicable[0].action.kind, ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);

    ark::es2panda::lsp::FormatCodeSettings settings;
    TextChangesContext tcc = {{}, ark::es2panda::lsp::GetFormatContext(settings), {}};

    refactorContext.textChangesContext = &tcc;

    auto edits = ref.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(edits);

    const auto &textChanges = edits->GetFileTextChanges();
    ASSERT_GE(textChanges.size(), 2U);
    ASSERT_FALSE(textChanges[1].textChanges.empty());
    EXPECT_EQ(textChanges[1].textChanges[0].newText,
              "import { Identifier as Local } from \"./DefaultToNamed_IdentifierForm2\";\n");

    ASSERT_FALSE(textChanges[0].textChanges.empty());
    EXPECT_EQ(textChanges[0].textChanges[0].newText, "export const Identifier = 42;");

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertImport, DefaultToNamed_Function_WithAlias)
{
    const std::string consumer = R"(
import Local from "./DefaultToNamed_Function_WithAlias2";
)";
    const std::string mod = R"(
export default function foo() {}
)";

    auto tempFiles = CreateTempFile(
        {"DefaultToNamed_Function_WithAlias1.ets", "DefaultToNamed_Function_WithAlias2.ets"}, {consumer, mod});
    ASSERT_EQ(tempFiles.size(), 2U);

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_BOUND);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor ref;
    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    auto applicable = ref.GetAvailableActions(refactorContext);
    ASSERT_EQ(applicable[0].action.kind, ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);

    ark::es2panda::lsp::FormatCodeSettings settings;
    TextChangesContext tcc = {{}, ark::es2panda::lsp::GetFormatContext(settings), {}};
    refactorContext.textChangesContext = &tcc;

    auto edits = ref.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(edits);

    const auto &textChanges = edits->GetFileTextChanges();
    ASSERT_GE(textChanges.size(), 2U);
    ASSERT_FALSE(textChanges[1].textChanges.empty());
    EXPECT_EQ(textChanges[1].textChanges[0].newText,
              "import { foo as Local } from \"./DefaultToNamed_Function_WithAlias2\";\n");
    ASSERT_FALSE(textChanges[0].textChanges.empty());
    EXPECT_EQ(textChanges[0].textChanges[0].newText, "export function foo() {}\n");
    init.DestroyContext(ctx);
}

TEST_F(LSPConvertImport, DefaultToNamed_Class_WithAlias)
{
    const std::string consumer = R"(
import DefaultExportedItemBindedName from "./DefaultToNamed_Class_WithAlias2";
function foo () {
  let v1 = new DefaultExportedItemBindedName()
}
)";
    const std::string someFile = R"(
export default class SomeClass{}
)";

    auto tempFiles = CreateTempFile({"DefaultToNamed_Class_WithAlias1.ets", "DefaultToNamed_Class_WithAlias2.ets"},
                                    {consumer, someFile});
    ASSERT_EQ(tempFiles.size(), 2U);

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_BOUND);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor refactor;
    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    auto applicable = refactor.GetAvailableActions(refactorContext);
    ASSERT_EQ(applicable[0].action.kind, ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext = {{}, formatContext, {}};
    refactorContext.textChangesContext = &textChangesContext;
    auto editsPtr =
        refactor.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(editsPtr);

    const auto &textChanges = editsPtr->GetFileTextChanges();
    ASSERT_GE(textChanges.size(), 2U);
    const std::string expectedImport =
        "import { SomeClass as DefaultExportedItemBindedName } from \"./DefaultToNamed_Class_WithAlias2\";\n";
    ASSERT_FALSE(textChanges[1].textChanges.empty());
    EXPECT_EQ(textChanges[1].textChanges[0].newText, expectedImport);
    ASSERT_FALSE(textChanges[0].textChanges.empty());
    const std::string expectedChng = "export class SomeClass {\n  public constructor() {}\n  \n}\n";
    EXPECT_EQ(textChanges[0].textChanges[0].newText, expectedChng);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertImport, DefaultToNamed_Class_SameName_NoAlias)
{
    const std::string consumer = R"(
import SomeClass from "./DefaultToNamed_Class_SameName2";
)";
    const std::string someFile = R"(
export default class SomeClass {}
)";

    auto tempFiles = CreateTempFile({"DefaultToNamed_Class_SameName1.ets", "DefaultToNamed_Class_SameName2.ets"},
                                    {consumer, someFile});
    ASSERT_EQ(tempFiles.size(), 2U);

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_BOUND);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor refactor;
    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    auto applicable = refactor.GetAvailableActions(refactorContext);
    ASSERT_EQ(applicable[0].action.kind, ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);

    ark::es2panda::lsp::FormatCodeSettings settings;
    TextChangesContext tcc = {{}, ark::es2panda::lsp::GetFormatContext(settings), {}};
    refactorContext.textChangesContext = &tcc;
    auto edits =
        refactor.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(edits);

    const auto &textChanges = edits->GetFileTextChanges();
    ASSERT_GE(textChanges.size(), 2U);
    ASSERT_FALSE(textChanges[1].textChanges.empty());
    EXPECT_EQ(textChanges[1].textChanges[0].newText,
              "import { SomeClass } from \"./DefaultToNamed_Class_SameName2\";\n");

    ASSERT_FALSE(textChanges[0].textChanges.empty());
    const std::string expectedChng = "export class SomeClass {\n  public constructor() {}\n  \n}\n";
    EXPECT_EQ(textChanges[0].textChanges[0].newText, expectedChng);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertImport, DefaultToNamed_IdentifierForm_ClassThenExportDefault_WithAlias)
{
    const std::string consumer = R"(
import {default as DefaultExportedItemNewName} from "./DefaultToNamed_Identifier2"
)";

    const std::string someFile = R"(
class SomeClass {}
export default SomeClass;
)";

    auto tempFiles =
        CreateTempFile({"DefaultToNamed_Identifier1.ets", "DefaultToNamed_Identifier2.ets"}, {consumer, someFile});
    ASSERT_EQ(tempFiles.size(), 2U);

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_BOUND);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor refactor;

    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    auto applicable = refactor.GetAvailableActions(refactorContext);
    ASSERT_EQ(applicable[0].action.kind, ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);

    ark::es2panda::lsp::FormatCodeSettings settings;
    TextChangesContext tcc = {{}, ark::es2panda::lsp::GetFormatContext(settings), {}};

    refactorContext.textChangesContext = &tcc;

    auto edits =
        refactor.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(edits);

    const auto &textChanges = edits->GetFileTextChanges();
    ASSERT_GE(textChanges.size(), 2U);
    ASSERT_FALSE(textChanges[1].textChanges.empty());
    EXPECT_EQ(textChanges[1].textChanges[0].newText,
              "import { SomeClass as DefaultExportedItemNewName } from \"./DefaultToNamed_Identifier2\";\n");

    ASSERT_FALSE(textChanges[0].textChanges.empty());
    EXPECT_EQ(textChanges[0].textChanges[0].newText, "export class SomeClass {\n  public constructor() {}\n  \n}\n");

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertImport, NamespaceToNamed_Basic)
{
    const std::string consumer = R"(
import * as lib from "./NamespaceToNamed_Basic2";
lib.a + lib.b;
)";
    const std::string libFile = R"(
export const a = 1;
export const b = 2;
)";

    auto tempFiles =
        CreateTempFile({"NamespaceToNamed_Basic1.ets", "NamespaceToNamed_Basic2.ets"}, {consumer, libFile});

    const size_t kExpectedTempFiles = 2U;
    ASSERT_EQ(tempFiles.size(), kExpectedTempFiles);

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor refactor;
    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    auto applicable = refactor.GetAvailableActions(refactorContext);
    ASSERT_EQ(applicable[0].action.kind, ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    ark::es2panda::lsp::FormatCodeSettings settings;
    TextChangesContext tcc = {{}, ark::es2panda::lsp::GetFormatContext(settings), {}};

    refactorContext.textChangesContext = &tcc;

    auto edits =
        refactor.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(edits);
    const auto &fc = edits->GetFileTextChanges();
    ASSERT_FALSE(fc.empty());
    const size_t kExpectedChanges = 3U;
    ASSERT_EQ(fc.front().textChanges.size(), kExpectedChanges);

    const std::array<std::string, kExpectedChanges> kExpectedTexts = {
        "import { a, b } from \"./NamespaceToNamed_Basic2\";\n", "a", "b"};
    for (size_t i = 0; i < kExpectedTexts.size(); ++i) {
        EXPECT_EQ(fc.front().textChanges[i].newText, kExpectedTexts[i]);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertImport, NamespaceToNamed_DuplicateUsages)
{
    const std::string consumer = R"(
import * as lib from "./NamespaceToNamed_DuplicateUsages2";
lib.a + lib.a;
)";
    const std::string libFile = R"(
export const a = 1;
)";

    auto temp = CreateTempFile({"NamespaceToNamed_DuplicateUsages1.ets", "NamespaceToNamed_DuplicateUsages2.ets"},
                               {consumer, libFile});

    const size_t kExpectedTempFiles = 2U;
    ASSERT_EQ(temp.size(), kExpectedTempFiles);

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(temp[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::ConvertImportRefactor ref;
    const size_t pos = consumer.find("import");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::FormatCodeSettings settings;
    TextChangesContext tcc = {{}, ark::es2panda::lsp::GetFormatContext(settings), {}};

    ark::es2panda::lsp::RefactorContext refactorContext {};
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.kind);
    refactorContext.span.pos = pos;
    refactorContext.span.end = pos;
    refactorContext.textChangesContext = &tcc;

    auto edits = ref.GetEditsForAction(refactorContext, std::string(ark::es2panda::lsp::TO_NAMED_IMPORT_ACTION.name));
    ASSERT_TRUE(edits);

    const auto &fc = edits->GetFileTextChanges();
    ASSERT_FALSE(fc.empty());

    const size_t kExpectedChanges = 3U;
    ASSERT_EQ(fc.front().textChanges.size(), kExpectedChanges);

    const std::array<std::string, kExpectedChanges> kExpectedTexts = {
        "import { a } from \"./NamespaceToNamed_DuplicateUsages2\";\n", "a", "a"};

    for (size_t i = 0; i < kExpectedTexts.size(); ++i) {
        EXPECT_EQ(fc.front().textChanges[i].newText, kExpectedTexts[i]);
    }

    init.DestroyContext(ctx);
}
}  // namespace