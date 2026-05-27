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
#include <algorithm>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

#include "lsp/include/formatting/formatting.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/extract_type.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp/include/types.h"
#include "lsp/include/user_preferences.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {
using ark::es2panda::lsp::Initializer;

std::string ApplyEdits(const std::string &original, const std::vector<::TextChange> &edits)
{
    if (edits.empty()) {
        return original;
    }

    std::vector<const ::TextChange *> ordered;
    ordered.reserve(edits.size());
    for (const auto &change : edits) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const ::TextChange *lhs, const ::TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    std::string result;
    result.reserve(original.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, original.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, static_cast<size_t>(original.size()));
        if (cursor < start) {
            result.append(original, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = end;
    }

    if (cursor < original.size()) {
        result.append(original, cursor, original.size() - cursor);
    }
    return result;
}

class LspExtrInterfaceGetEditsTests : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractInterfaceRefactorTest.ets"};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

        ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
        ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};

        auto *refactorContext = new ark::es2panda::lsp::RefactorContext;
        refactorContext->context = ctx;
        refactorContext->textChangesContext = textChangesContext;
        refactorContext->span.pos = start;
        refactorContext->span.end = end;
        refactorContext->kind = "refactor.extract.interface";
        return refactorContext;
    }
};

void ExpectExtractionApplies(const std::string &source, ark::es2panda::lsp::RefactorContext *refactorContext,
                             const std::string &refactorName, const std::string &actionName,
                             const std::string &expected)
{
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const std::string actual = ApplyEdits(source, fileEdit.textChanges);
    EXPECT_EQ(actual, expected);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterfaceForInlineObjectVariable)
{
    const std::string code = R"(
'use static'
class A {}
let a: { n: number; s: string } = { n: 1, s: "value" };
)";
    const std::string expected = R"(
'use static'
class A {}
interface NewType {
  n: number;
  s: string;
}

let a: NewType = { n: 1, s: "value" };
)";
    const std::string target = R"({ n: number; s: string })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterfaceForClassMethodReturn)
{
    const std::string code = R"('use static'
class Circle {
    radius: number;
    constructor(radius: number) {
        this.radius = radius;
    }
    
    getBoundingBox(): { width: number; height: number } {
        return {
            width: this.radius * 2,
            height: this.radius * 2
        }
    }
})";
    const std::string expected = R"('use static'
interface NewType {
  width: number;
  height: number;
}

class Circle {
    radius: number;
    constructor(radius: number) {
        this.radius = radius;
    }
    
    getBoundingBox(): NewType {
        return {
            width: this.radius * 2,
            height: this.radius * 2
        }
    }
})";
    const std::string target = R"({ width: number; height: number })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterfaceForTypeAssertionObject)
{
    const std::string code = R"(
'use static'
const typed = (value as { n: number; s: string }).s.length;
)";
    const std::string expected = R"(
'use static'
interface NewType {
  n: number;
  s: string;
}

const typed = (value as NewType).s.length;)";
    const std::string target = R"({ n: number; s: string })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterfaceGeneratesUnderscoreSuffixForConflictingName)
{
    const std::string code = R"(
'use static'
interface NewType { old: string }
const data: { width: number; height: number } = { width: 1, height: 2 };
)";
    const std::string expected = R"(
'use static'
interface NewType { old: string }
interface NewType_1 {
  width: number;
  height: number;
}

const data: NewType_1 = { width: 1, height: 2 };)";
    const std::string target = R"({ width: number; height: number })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface1)
{
    const std::string code = R"(
'use static'
class Circle {
  getBoundingBox(): /*start*/{ width: number; height: number }/*end*/ {
    return {
      width: this.radius * 2,
      height: this.radius * 2,
    }
  }
}
)";
    const std::string expected = R"(
'use static'
interface NewType {
  width: number;
  height: number;
}

class Circle {
  getBoundingBox(): /*start*/NewType/*end*/ {
    return {
      width: this.radius * 2,
      height: this.radius * 2,
    }
  }
}
)";
    const std::string target = R"({ width: number; height: number })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface2)
{
    const std::string code = R"(
'use static'
namespace Sharp {
  class CircleEx {
    getBoundingBox(): /*start*/{ width: number; height: number }/*end*/ {
      return {
        width: this.radius * 2,
        height: this.radius * 2,
      }
    }
  }
}
)";
    const std::string expected = R"(
'use static'
interface NewType {
  width: number;
  height: number;
}

namespace Sharp {
  class CircleEx {
    getBoundingBox(): /*start*/NewType/*end*/ {
      return {
        width: this.radius * 2,
        height: this.radius * 2,
      }
    }
  }
}
)";
    const std::string target = R"({ width: number; height: number })";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto *initializer = new Initializer();
    auto *refactorContext = CreateExtractContext(initializer, code, spanStart, spanEnd);

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface3)
{
    const std::string code = R"(
'use static'

function createObject(): /*start*/{ x: number, y: number }/*end*/ {
  return { x: 1, y: 2 };
}
)";
    const std::string expected = R"(
'use static'

interface NewType {
  x: number;
  y: number;
}

function createObject(): /*start*/NewType/*end*/ {
  return { x: 1, y: 2 };
}
)";
    const std::string target = R"({ x: number, y: number })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface4)
{
    const std::string code = R"(
'use static'
class A {}
class Logger {
    getFormatter(): /*start*/{ format: (msg: string) => string }/*end*/ {
        return { format: (msg) => msg };
    }
}
)";
    const std::string expected = R"(
'use static'
class A {}
interface NewType {
  format: (msg: string) => string;
}

class Logger {
  getFormatter(): /*start*/NewType/*end*/ {
    return { format: (msg) => msg };
  }
}
)";
    const std::string target = R"({ format: (msg: string) => string })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface5)
{
    const std::string code = R"(
'use static'
const getPoint = (): /*start*/{ x: number, y: number }/*end*/ => {
  return { x: 0, y: 0 };
};
)";
    const std::string expected = R"(
'use static'
interface NewType {
  x: number;
  y: number;
}

const getPoint = (): /*start*/NewType/*end*/ => {
  return { x: 0, y: 0 };
};
)";
    const std::string target = R"({ x: number, y: number })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface6)
{
    const std::string code = R"(
'use static'
function getUsers(): /*start*/{ id: number, name: string }[]/*end*/ {
  return [{ id: 1, name: "Alice" }];
}
)";
    const std::string target = R"({ id: number, name: string }[])";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(applicable.empty());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface7)
{
    const std::string code = R"(
'use static'
class Config {
    getData(): /*start*/{ a: { b: { c: { d: number, e: string } } } }/*end*/ {
        return { a: { b: { c: { d: 1, e: "test" } } } };
    }
}
)";
    const std::string expected = R"(
'use static'
interface NewType {
  a: { b: { c: { d: number, e: string } } };
}

class Config {
    getData(): /*start*/NewType/*end*/ {
        return { a: { b: { c: { d: 1, e: "test" } } } };
    }
}
)";
    const std::string target = R"({ a: { b: { c: { d: number, e: string } } } })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface8)
{
    const std::string code = R"(
'use static'
class A {}
interface Handler {
    (data: /*start*/{ value: number, timestamp: number }/*end*/): void;
}
)";
    const std::string expected = R"(
'use static'
class A {}
interface NewType {
  value: number;
  timestamp: number;
}

interface Handler {
    (data: /*start*/NewType/*end*/): void;
}
)";
    const std::string target = R"({ value: number, timestamp: number })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface9)
{
    const std::string code = R"(
'use static'
function process(data: /*start*/{ type: "a", value: number } | { type: "b", label: string }/*end*/): void {
}
)";
    const std::string target = R"({ type: "a", value: number } | { type: "b", label: string })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(applicable.empty());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface10)
{
    const std::string code = R"(
'use static'
function merge(a: /*start*/{ x: number } & { y: string }/*end*/): void {
}
)";
    const std::string target = R"({ x: number } & { y: string })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(applicable.empty());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface11)
{
    const std::string code = R"(
'use static'
class A {}
interface Handler {
    (data: /*start*/{ value: number, timestamp: number }/*end*/): void;
}
async function fetchData(): Promise</*start*/{ id: number, name: string }/*end*/> {
  return { id: 1, name: "Alice" };
}
)";
    const std::string expected = R"(
'use static'
class A {}
interface Handler {
    (data: /*start*/{ value: number, timestamp: number }/*end*/): void;
}
interface NewType {
  id: number;
  name: string;
}

async function fetchData(): Promise</*start*/NewType/*end*/> {
  return { id: 1, name: "Alice" };
}
)";
    const std::string target = R"({ id: number, name: string })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());

    const std::string_view actionName = ark::es2panda::lsp::EXTRACT_INTERFACE_ACTION.name;
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_TYPE_NAME;
    const bool found =
        std::any_of(applicable.begin(), applicable.end(), [&](const ark::es2panda::lsp::ApplicableRefactorInfo &info) {
            return info.action.name == actionName;
        });
    ASSERT_TRUE(found);

    ExpectExtractionApplies(code, refactorContext, std::string(refactorName), std::string(actionName), expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrInterfaceGetEditsTests, ExtractInterface12)
{
    const std::string code = R"(
interface I { a: 1 | 2 | 3 }
let i: I = /*start*/{ a: 1 }/*end*/;
)";
    const std::string target = R"({ a: 1 })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(applicable.empty());

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
