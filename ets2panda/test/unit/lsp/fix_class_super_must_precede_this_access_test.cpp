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

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_class_super_must_precede_this_access.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS;

constexpr auto ERROR_CODES = CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetSupportedCodeNumbers();
constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t DEFAULT_LENGTH = 4;
constexpr size_t SUPER_KEYWORD_LENGTH = 5;

class FixClassSuperMustPrecedeThisAccessTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

protected:
    void RunCodeFixTest(const std::string &initialContent, const std::string &expectedContent, size_t errLine,
                        size_t errCol, size_t length = DEFAULT_LENGTH)
    {
        std::vector<std::string> fileNames = {"test_case.ets"};
        std::vector<std::string> fileContents = {initialContent};

        auto filePaths = CreateTempFile(fileNames, fileContents);
        ASSERT_EQ(fileNames.size(), filePaths.size());

        Initializer initializer;
        auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        size_t start = LineColToPos(context, errLine, errCol);
        std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
        ark::es2panda::lsp::FormatCodeSettings settings;
        settings.SetNewLineCharacter("\n");
        CodeFixOptions options = {CreateNonCancellationToken(), settings, {}};

        const std::string expectedFixName(CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId());
        const std::string expectedFixDescription = "Fix 'super' access before 'this'";
        const size_t expectedFixCount = 1;
        const size_t expectedTextChangesCount = 2;

        auto fixResult =
            ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

        ASSERT_EQ(fixResult.size(), expectedFixCount);
        ASSERT_EQ(fixResult[0].description_, expectedFixDescription);
        ASSERT_EQ(fixResult[0].fixName_, expectedFixName);
        const auto &changes = fixResult[0].changes_[0].textChanges;
        ASSERT_EQ(changes.size(), expectedTextChangesCount);

        std::string actualContent = ApplyTextChanges(initialContent, changes);
        std::string actualClean = RemoveEmptyLines(actualContent);
        std::string expectedClean = RemoveEmptyLines(expectedContent);
        EXPECT_EQ(actualClean, expectedClean);

        initializer.DestroyContext(context);
    }

    static std::string ApplyTextChanges(const std::string &source, const std::vector<TextChange> &changes)
    {
        std::string result = source;
        auto sortedChanges = changes;
        std::sort(sortedChanges.begin(), sortedChanges.end(),
                  [](const auto &a, const auto &b) { return a.span.start > b.span.start; });

        for (const auto &change : sortedChanges) {
            result.replace(change.span.start, change.span.length, change.newText);
        }
        return result;
    }

    static std::string RemoveEmptyLines(const std::string &input)
    {
        std::string result;
        std::stringstream ss(input);
        std::string line;
        bool first = true;
        while (std::getline(ss, line)) {
            bool isEmpty = true;
            for (char c : line) {
                if (std::isspace(c) == 0) {
                    isEmpty = false;
                    break;
                }
            }
            if (!isEmpty) {
                if (!first) {
                    result += "\n";
                }
                result += line;
                first = false;
            }
        }
        result += "\n";
        return result;
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

TEST_F(FixClassSuperMustPrecedeThisAccessTests, TestSuperMustPrecedeThisAccess)
{
    std::string initialContent = R"(
class Animal {
    constructor(public name: string) {}
}

class Dog extends Animal {
    constructor(name: string) {
        this.bark();
        super(name);
    }
    bark() { console.log("Woof!"); }
}
)";
    std::string expectedContent = R"(
class Animal {
    constructor(public name: string) {}
}

class Dog extends Animal {
    constructor(name: string) {
super(name);
        this.bark();
    }
    bark() { console.log("Woof!"); }
}
)";
    const size_t errLine = 8;
    const size_t errCol = 9;
    RunCodeFixTest(initialContent, expectedContent, errLine, errCol);
}

TEST_F(FixClassSuperMustPrecedeThisAccessTests, TestPropertyAssignmentBeforeSuper)
{
    std::string initialContent = R"(
class Base {}
class Derived extends Base {
    prop: number;
    constructor() {
        this.prop = 5;
        super();
    }
}
)";
    std::string expectedContent = R"(
class Base {}
class Derived extends Base {
    prop: number;
    constructor() {
super();
        this.prop = 5;
    }
}
)";
    const size_t errLine = 6;
    const size_t errCol = 9;
    RunCodeFixTest(initialContent, expectedContent, errLine, errCol);
}

TEST_F(FixClassSuperMustPrecedeThisAccessTests, TestThisAccessInArgumentBeforeSuper)
{
    std::string initialContent = R"(
class Base {}
class Derived extends Base {
    constructor() {
        console.log(this);
        super();
    }
}
)";
    std::string expectedContent = R"(
class Base {}
class Derived extends Base {
    constructor() {
super();
        console.log(this);
    }
}
)";
    const size_t errLine = 5;
    const size_t errCol = 21;
    RunCodeFixTest(initialContent, expectedContent, errLine, errCol);
}

TEST_F(FixClassSuperMustPrecedeThisAccessTests, TestTriggerOnSuper)
{
    std::string initialContent = R"(
class Base {}
class Derived extends Base {
    constructor() {
        console.log(1);
        super();
    }
}
)";
    std::string expectedContent = R"(
class Base {}
class Derived extends Base {
    constructor() {
super();
        console.log(1);
    }
}
)";
    const size_t errLine = 6;
    const size_t errCol = 9;
    RunCodeFixTest(initialContent, expectedContent, errLine, errCol, SUPER_KEYWORD_LENGTH);
}
}  // namespace