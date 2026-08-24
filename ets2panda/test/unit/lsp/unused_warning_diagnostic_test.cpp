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

#include "lsp_api_test.h"

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LSPUnusedWarningDiagnosticTests : public LSPAPITests {};

DiagnosticReferences GetSemanticDiagnostics(const std::string &fileName, const std::string &source)
{
    Initializer initializer;
    auto *context = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    EXPECT_NE(context, nullptr);
    if (context == nullptr) {
        return {};
    }

    LSPAPI const *lspApi = GetImpl();
    auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);
    return diagnostics;
}

std::pair<DiagnosticReferences, DiagnosticReferences> GetSyntacticAndSemanticDiagnostics(const std::string &fileName,
                                                                                         const std::string &source)
{
    Initializer initializer;
    auto *context = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    EXPECT_NE(context, nullptr);
    if (context == nullptr) {
        return {};
    }

    LSPAPI const *lspApi = GetImpl();
    auto syntacticDiagnostics = lspApi->getSyntacticDiagnostics(context);
    auto semanticDiagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);
    return {syntacticDiagnostics, semanticDiagnostics};
}

bool HasDiagnosticMessage(const DiagnosticReferences &diagnostics, const std::string &message)
{
    return std::any_of(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                       [&message](const auto &diagnostic) { return diagnostic.message_ == message; });
}

bool HasUnusedDiagnosticData(const DiagnosticReferences &diagnostics, const std::string &message)
{
    return std::any_of(
        diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(), [&message](const auto &diagnostic) {
            if (diagnostic.message_ != message || !std::holds_alternative<std::string>(diagnostic.data_)) {
                return false;
            }
            return std::get<std::string>(diagnostic.data_) == "unusedSymbol";
        });
}

void ExpectDiagnostics(const DiagnosticReferences &diagnostics, const std::vector<std::string> &expectedMessages)
{
    for (const auto &message : expectedMessages) {
        EXPECT_TRUE(HasDiagnosticMessage(diagnostics, message)) << message;
        EXPECT_TRUE(HasUnusedDiagnosticData(diagnostics, message)) << message;
    }
}

void ExpectNoDiagnostics(const DiagnosticReferences &diagnostics, const std::vector<std::string> &unexpectedMessages)
{
    for (const auto &message : unexpectedMessages) {
        EXPECT_FALSE(HasDiagnosticMessage(diagnostics, message)) << message;
    }
}

constexpr char LOCAL_PARAMETER_FOR_SOURCE[] = R"(
function check(unusedParam: number, writtenOnlyParam: number, usedParam: number, ...unusedRest: number[]): number {
    let usedLocal = usedParam + 1
    let unusedLocal = 42
    let writtenOnlyLocal = 0
    let total = 0

    writtenOnlyParam = 1
    writtenOnlyLocal = 2

    for (let unusedFor = 0; usedLocal < 3; usedLocal++) {
    }

    for (let usedFor = 0; usedFor < 2; usedFor++) {
        total += usedFor
    }

    return usedLocal + total
}

check(1, 2, 3)
)";

constexpr char TOP_LEVEL_DECLARATIONS_SOURCE[] = R"(
let unusedTopLevelValue: number = 1
let unusedTopLevelArrow = () => {}
let usedTopLevelArrow = () => {
    return 1
}
export let exportedTopLevelArrow: () => void = () => {}

class UnusedClass {}
function unusedFunction(): void {}
enum UnusedEnum {
    VALUE,
}
interface UnusedInterface {}
type UnusedTypeAlias = number

class UsedAsBaseClass {}
class DerivedFromUsedAsBaseClass extends UsedAsBaseClass {
    value(): number {
        return 1
    }
}
interface UsedAsParentInterface {}
interface DerivedFromUsedAsParentInterface extends UsedAsParentInterface {}
type UsedTypeAlias = number
function usesTypeAlias(value: UsedTypeAlias): UsedTypeAlias {
    return value
}

function useDeclarations(): number {
    let derived = new DerivedFromUsedAsBaseClass()
    return usedTopLevelArrow() + derived.value() + usesTypeAlias(1)
}

useDeclarations()
)";

constexpr char PRIVATE_MEMBERS_SOURCE[] = R"(
class PrivateMemberCases {
    private unusedField: number = 1
    private writtenOnlyField: number = 2
    private usedField: number = 3

    private unusedMethod(): number {
        return 1
    }

    private usedMethod(): number {
        return 2
    }

    test(): number {
        this.writtenOnlyField = 4
        return this.usedField + this.usedMethod()
    }
}

class SamePrivateFieldA {
    private value: number = 1

    test(): number {
        return this.value
    }
}

class SamePrivateFieldB {
    private value: number = 2

    test(): number {
        return 1
    }
}

class SamePrivateMethodA {
    private helper(): number {
        return 1
    }

    test(): number {
        return this.helper()
    }
}

class SamePrivateMethodB {
    private helper(): number {
        return 2
    }

    test(): number {
        return 1
    }
}

function useClasses(): number {
    let privateCases = new PrivateMemberCases()
    let sameFieldA = new SamePrivateFieldA()
    let sameFieldB = new SamePrivateFieldB()
    let sameMethodA = new SamePrivateMethodA()
    let sameMethodB = new SamePrivateMethodB()
    return privateCases.test() + sameFieldA.test() + sameFieldB.test() + sameMethodA.test() + sameMethodB.test()
}

useClasses()
)";

constexpr char NAMESPACE_MEMBERS_SOURCE[] = R"(
namespace NamespaceA {
    let value: number = 1
    let usedByLocalInitializer: number = value
}

namespace NamespaceB {
    let value: number = 1
}

namespace NamespaceFunctionA {
    function helper(value: number): number {
        return value
    }

    let result: number = helper(1)
}

namespace NamespaceFunctionB {
    function helper(value: number): number {
        return value
    }
}
)";

constexpr char IMPORT_SOURCE_FILE_SOURCE[] = R"(
export class UsedImport {}
export class UnusedImport {}
)";

constexpr char IMPORT_MAIN_FILE_SOURCE[] = R"(
import { UsedImport } from "./unused_import_source"
import * as UnusedModule from "./unused_import_source"

function useImport(): UsedImport {
    return new UsedImport()
}

useImport()
)";

constexpr char DATA_SPLIT_SOURCE[] = R"(
function entry(): number {
    let unusedLocal = 1
    return 0
}

entry()
)";

constexpr char MEMBER_EXPRESSION_SOURCE[] = R"(
class Holder {
    value: number = 1
}

function useMembers(): number {
    let holder = new Holder()
    let values = [1, 2]
    let index = 0
    let assignedHolder = new Holder()
    assignedHolder.value = 1
    return holder.value + values[index]
}

useMembers()
)";

constexpr char CATCH_PARAMETER_SOURCE[] = R"(
function unusedCatchParam(): void {
    try {
        console.log("x")
    } catch (unusedError) {
    }
}

function usedCatchParam(): void {
    try {
        console.log("x")
    } catch (usedError) {
        console.log(usedError)
    }
}

unusedCatchParam()
usedCatchParam()
)";

constexpr char ENUM_MEMBER_ACCESS_SOURCE[] = R"(
enum Color {
    RED,
    GREEN,
    BLUE,
}

function useEnum(): number {
    let color = Color.RED
    return color
}

useEnum()
)";

TEST_F(LSPUnusedWarningDiagnosticTests, ReportsLocalVariablesParametersRestAndForVariables)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_local_parameter_for.ets", LOCAL_PARAMETER_FOR_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedParam' is never used",
                                       "'writtenOnlyParam' is never used",
                                       "'unusedRest' is never used",
                                       "'unusedLocal' is never used",
                                       "'writtenOnlyLocal' is never used",
                                       "'unusedFor' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'check' is never used",
                                         "'usedParam' is never used",
                                         "'usedLocal' is never used",
                                         "'total' is never used",
                                         "'usedFor' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, ReportsUnusedTopLevelAndTypeDeclarations)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_top_level_declarations.ets", TOP_LEVEL_DECLARATIONS_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedTopLevelValue' is never used",
                                       "'unusedTopLevelArrow' is never used",
                                       "'UnusedClass' is never used",
                                       "'unusedFunction' is never used",
                                       "'UnusedEnum' is never used",
                                       "'UnusedInterface' is never used",
                                       "'UnusedTypeAlias' is never used",
                                       "'DerivedFromUsedAsParentInterface' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'exportedTopLevelArrow' is never used",
                                         "'UsedAsBaseClass' is never used",
                                         "'DerivedFromUsedAsBaseClass' is never used",
                                         "'UsedAsParentInterface' is never used",
                                         "'UsedTypeAlias' is never used",
                                         "'usesTypeAlias' is never used",
                                         "'useDeclarations' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, ReportsUnusedPrivateMembersWithScopedNames)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_private_members.ets", PRIVATE_MEMBERS_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedField' is never used",
                                       "'writtenOnlyField' is never used",
                                       "'unusedMethod' is never used",
                                       "'value' is never used",
                                       "'helper' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'PrivateMemberCases' is never used",
                                         "'SamePrivateFieldA' is never used",
                                         "'SamePrivateFieldB' is never used",
                                         "'SamePrivateMethodA' is never used",
                                         "'SamePrivateMethodB' is never used",
                                         "'usedField' is never used",
                                         "'usedMethod' is never used",
                                         "'useClasses' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, ReportsUnusedNamespaceMembersWithScopedNames)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_namespace_members.ets", NAMESPACE_MEMBERS_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'NamespaceA' is never used",
                                       "'usedByLocalInitializer' is never used",
                                       "'NamespaceB' is never used",
                                       "'value' is never used",
                                       "'NamespaceFunctionA' is never used",
                                       "'result' is never used",
                                       "'NamespaceFunctionB' is never used",
                                       "'helper' is never used",
                                   });
}

TEST_F(LSPUnusedWarningDiagnosticTests, MarksMemberExpressionObjectAndComputedPropertyReferences)
{
    const auto diagnostics =
        GetSemanticDiagnostics("unused_member_expression_references.ets", MEMBER_EXPRESSION_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'holder' is never used",
                                         "'values' is never used",
                                         "'index' is never used",
                                         "'assignedHolder' is never used",
                                         "'useMembers' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, ReportsUnusedCatchParameters)
{
    const auto diagnostics = GetSemanticDiagnostics("unused_catch_parameters.ets", CATCH_PARAMETER_SOURCE);

    ExpectDiagnostics(diagnostics, {
                                       "'unusedError' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'unusedCatchParam' is never used",
                                         "'usedCatchParam' is never used",
                                         "'usedError' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, DoesNotReportEnumUsedByMemberAccess)
{
    const auto diagnostics = GetSemanticDiagnostics("used_enum_member_access.ets", ENUM_MEMBER_ACCESS_SOURCE);

    ExpectNoDiagnostics(diagnostics, {
                                         "'Color' is never used",
                                         "'useEnum' is never used",
                                         "'color' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, ReportsUnusedNamespaceImport)
{
    const std::vector<std::string> files = {"unused_import_source.ets", "unused_import_main.ets"};
    const std::vector<std::string> contents = {IMPORT_SOURCE_FILE_SOURCE, IMPORT_MAIN_FILE_SOURCE};
    auto paths = CreateTempFile(files, contents);
    ASSERT_EQ(paths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED, contents[1].c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto diagnostics = lspApi->getSemanticDiagnostics(context);
    initializer.DestroyContext(context);

    ExpectDiagnostics(diagnostics, {
                                       "'UnusedModule' is never used",
                                   });
    ExpectNoDiagnostics(diagnostics, {
                                         "'UsedImport' is never used",
                                         "'useImport' is never used",
                                     });
}

TEST_F(LSPUnusedWarningDiagnosticTests, ReturnsUnusedDataFromSemanticDiagnosticsOnly)
{
    const auto [syntacticDiagnostics, semanticDiagnostics] =
        GetSyntacticAndSemanticDiagnostics("unused_data_split.ets", DATA_SPLIT_SOURCE);
    const std::string message = "'unusedLocal' is never used";

    EXPECT_FALSE(HasDiagnosticMessage(syntacticDiagnostics, message));
    EXPECT_TRUE(HasDiagnosticMessage(semanticDiagnostics, message));
    EXPECT_TRUE(HasUnusedDiagnosticData(semanticDiagnostics, message));
}

}  // namespace
