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

#include <parser/program/program.h>
#include <parser/parserImpl.h>
#include <es2panda.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>
#include <ir/statements/blockStatement.h>

namespace panda::es2panda::parser {

using mem::MemConfig;

class MemManager {
public:
    explicit MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;
        MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        MemConfig::Finalize();
    }
};

class ProgramTest : public ::testing::Test {
protected:
    // Constants for testing
    static constexpr int defaultTargetApiVersion = 100;
    static constexpr int testAllocatorValue = 42;
    static constexpr int memoryLeakTestIterations = 100;

    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
    }

    void TearDown() override
    {
        mm_.reset();
    }

    // Helper method to parse source code and return a Program with valid AST
    Program ParseSource(const std::string &source, ScriptExtension ext = ScriptExtension::JS,
                        ScriptKind kind = ScriptKind::SCRIPT)
    {
        ParserImpl parser(ext);
        SourceFile sourceFile("test.js", "test", kind, ext);
        sourceFile.source = source;
        CompilerOptions options;
        return parser.Parse(sourceFile, options);
    }

    std::unique_ptr<MemManager> mm_;
};

// Test Program construction with JS extension
TEST_F(ProgramTest, Constructor_JSExtension_InitializesCorrectly)
{
    Program program(ScriptExtension::JS);
    EXPECT_EQ(program.Extension(), ScriptExtension::JS);
    EXPECT_NE(program.Allocator(), nullptr);
    EXPECT_NE(program.Binder(), nullptr);
    EXPECT_EQ(program.Ast(), nullptr);
}

// Test Program construction with TS extension
TEST_F(ProgramTest, Constructor_TSExtension_InitializesCorrectly)
{
    Program program(ScriptExtension::TS);
    EXPECT_EQ(program.Extension(), ScriptExtension::TS);
    EXPECT_NE(program.Allocator(), nullptr);
    EXPECT_NE(program.Binder(), nullptr);
}

// Test Program construction with AS extension
TEST_F(ProgramTest, Constructor_ASExtension_InitializesCorrectly)
{
    Program program(ScriptExtension::AS);
    EXPECT_EQ(program.Extension(), ScriptExtension::AS);
    EXPECT_NE(program.Allocator(), nullptr);
    EXPECT_NE(program.Binder(), nullptr);
}

// Test Program construction with ABC extension
TEST_F(ProgramTest, Constructor_ABCExtension_InitializesCorrectly)
{
    Program program(ScriptExtension::ABC);
    EXPECT_EQ(program.Extension(), ScriptExtension::ABC);
    EXPECT_NE(program.Allocator(), nullptr);
    EXPECT_NE(program.Binder(), nullptr);
}

// Test move constructor transfers ownership correctly
TEST_F(ProgramTest, MoveConstructor_TransfersOwnership)
{
    auto original = ParseSource("var x = 42;");
    original.SetHasTLA(true);
    original.SetDebug(true);
    original.SetTargetApiVersion(defaultTargetApiVersion);
    original.SetRecordName("testModule");

    // Capture pointers before move for verification
    auto originalBinder = original.Binder();
    auto originalAllocator = original.Allocator();
    auto originalAst = original.Ast();
    auto originalRecordName = original.RecordName();
    auto originalFormatedRecordName = original.FormatedRecordName();
    ASSERT_NE(originalAst, nullptr);

    // Perform move construction
    Program moved(std::move(original));

    // Verify all critical properties are transferred
    EXPECT_EQ(moved.Extension(), ScriptExtension::JS);
    EXPECT_TRUE(moved.HasTLA());
    EXPECT_TRUE(moved.IsDebug());
    EXPECT_EQ(moved.TargetApiVersion(), defaultTargetApiVersion);
    EXPECT_EQ(moved.Binder(), originalBinder);
    EXPECT_EQ(moved.Allocator(), originalAllocator);
    EXPECT_EQ(moved.Ast(), originalAst);
    EXPECT_NE(moved.Ast(), nullptr);
    EXPECT_TRUE(moved.Ast()->IsBlockStatement());
    EXPECT_EQ(moved.RecordName(), originalRecordName);
    EXPECT_EQ(moved.FormatedRecordName(), originalFormatedRecordName);

    // Verify original is in valid moved-from state
    EXPECT_EQ(original.Binder(), nullptr);
    EXPECT_EQ(original.Ast(), nullptr);
}

// Test move assignment operator transfers ownership correctly
TEST_F(ProgramTest, MoveAssignmentOperator_TransfersOwnership)
{
    auto program1 = ParseSource("var x = 42;");
    program1.SetHasTLA(true);
    program1.SetDebug(true);
    program1.SetTargetApiVersion(defaultTargetApiVersion);

    auto originalBinder = program1.Binder();
    auto originalAllocator = program1.Allocator();
    auto originalAst = program1.Ast();
    ASSERT_NE(originalAst, nullptr);

    auto program2 = ParseSource("const y = 100;");
    ASSERT_NE(program2.Ast(), nullptr);

    program2 = std::move(program1);

    EXPECT_EQ(program2.Extension(), ScriptExtension::JS);
    EXPECT_TRUE(program2.HasTLA());
    EXPECT_TRUE(program2.IsDebug());
    EXPECT_EQ(program2.TargetApiVersion(), defaultTargetApiVersion);
    EXPECT_EQ(program2.Binder(), originalBinder);
    EXPECT_EQ(program2.Allocator(), originalAllocator);
    EXPECT_EQ(program2.Ast(), originalAst);
    EXPECT_NE(program2.Ast(), nullptr);
    EXPECT_TRUE(program2.Ast()->IsBlockStatement());
    EXPECT_EQ(program1.Binder(), nullptr);
    EXPECT_EQ(program1.Ast(), nullptr);
}

// Test self move assignment handles gracefully
TEST_F(ProgramTest, MoveAssignmentOperator_SelfAssignment_HandlesCorrectly)
{
    auto program = ParseSource("var x = 42;");

    // Capture pointers before self-move
    auto originalBinder = program.Binder();
    auto originalAllocator = program.Allocator();
    auto originalAst = program.Ast();

    // Perform self-move assignment
    Program *ptr = &program;
    program = std::move(*ptr);

    // Verify program remains valid after self-move
    EXPECT_EQ(program.Binder(), originalBinder);
    EXPECT_EQ(program.Allocator(), originalAllocator);
    EXPECT_EQ(program.Ast(), originalAst);
    EXPECT_NE(program.Ast(), nullptr);
    EXPECT_TRUE(program.Ast()->IsBlockStatement());
}

// Test SetSource with isDtsFile=false sets IsDtsFile to false
TEST_F(ProgramTest, SetSource_WithNonDtsFile_SetsIsDtsFileToFalse)
{
    Program program(ScriptExtension::TS);
    program.SetSource("const x: number = 5;", "test.ts", false);

    EXPECT_EQ(program.SourceCode().Utf8(), "const x: number = 5;");
    EXPECT_EQ(program.SourceFile().Utf8(), "test.ts");
    EXPECT_FALSE(program.IsDtsFile());
}

// Test SetSource with isDtsFile=true sets IsDtsFile to true
TEST_F(ProgramTest, SetSource_WithDtsFile_SetsIsDtsFileToTrue)
{
    Program program(ScriptExtension::TS);
    program.SetSource("declare const x: number;", "test.d.ts", true);

    EXPECT_EQ(program.SourceCode().Utf8(), "declare const x: number;");
    EXPECT_EQ(program.SourceFile().Utf8(), "test.d.ts");
    EXPECT_TRUE(program.IsDtsFile());
}

// Test SetSource with empty source code handles correctly
TEST_F(ProgramTest, SetSource_EmptySourceCode_HandlesCorrectly)
{
    Program program(ScriptExtension::JS);
    program.SetSource("", "test.js", false);

    EXPECT_EQ(program.SourceCode().Utf8(), "");
    EXPECT_EQ(program.SourceFile().Utf8(), "test.js");
    EXPECT_FALSE(program.IsDtsFile());
}

// Test SetSource with empty filename handles correctly
TEST_F(ProgramTest, SetSource_EmptyFilename_HandlesCorrectly)
{
    Program program(ScriptExtension::JS);
    program.SetSource("const x = 5;", "", false);

    EXPECT_EQ(program.SourceCode().Utf8(), "const x = 5;");
    EXPECT_EQ(program.SourceFile().Utf8(), "");
    EXPECT_FALSE(program.IsDtsFile());
}

// Test SetSource with both empty source and filename handles correctly
TEST_F(ProgramTest, SetSource_EmptySourceAndFilename_HandlesCorrectly)
{
    Program program(ScriptExtension::JS);
    program.SetSource("", "", false);

    EXPECT_EQ(program.SourceCode().Utf8(), "");
    EXPECT_EQ(program.SourceFile().Utf8(), "");
    EXPECT_FALSE(program.IsDtsFile());
}

// Test SetRecordName sets both RecordName and FormatedRecordName
TEST_F(ProgramTest, SetRecordName_SetsRecordNameAndFormattedName)
{
    Program program(ScriptExtension::JS);
    const std::string recordName = "myModule";

    program.SetRecordName(recordName);

    EXPECT_EQ(program.RecordName().Utf8(), recordName);
    EXPECT_EQ(program.FormatedRecordName().Utf8(), recordName + ".");
}

// Test SetRecordName with empty string handles correctly
TEST_F(ProgramTest, SetRecordName_EmptyString_HandlesCorrectly)
{
    Program program(ScriptExtension::JS);
    program.SetRecordName("");

    EXPECT_EQ(program.RecordName().Utf8(), "");
    EXPECT_EQ(program.FormatedRecordName().Utf8(), ".");
}

// Test SetAst and Ast getter with parsed AST
TEST_F(ProgramTest, SetAstAndGetAst_SetsAndReturnsAst)
{
    auto program1 = ParseSource("var x = 42;");
    ASSERT_NE(program1.Ast(), nullptr);

    ir::BlockStatement *parsedAst = program1.Ast();
    EXPECT_EQ(program1.Ast(), parsedAst);

    // Test const version of Ast()
    const Program &constProgram = program1;
    EXPECT_EQ(constProgram.Ast(), parsedAst);

    // Verify different parsed programs have different ASTs
    auto program2 = ParseSource("const y = 100;");
    ASSERT_NE(program2.Ast(), nullptr);
    EXPECT_NE(program2.Ast(), program1.Ast());
}

// Test SetAst directly sets AST
TEST_F(ProgramTest, SetAst_SetsAstDirectly)
{
    Program program(ScriptExtension::JS);
    EXPECT_EQ(program.Ast(), nullptr);

    // Get an AST from a parsed program
    auto sourceProgram = ParseSource("var x = 42;");
    ASSERT_NE(sourceProgram.Ast(), nullptr);
    ir::BlockStatement *ast = sourceProgram.Ast();

    // Set AST to a new program
    program.SetAst(ast);

    EXPECT_EQ(program.Ast(), ast);
    EXPECT_NE(program.Ast(), nullptr);
    EXPECT_TRUE(program.Ast()->IsBlockStatement());
}

// Test SetAst with nullptr
TEST_F(ProgramTest, SetAst_WithNullptr_SetsToNull)
{
    auto program = ParseSource("var x = 42;");
    ASSERT_NE(program.Ast(), nullptr);

    // Set AST to nullptr
    program.SetAst(nullptr);

    EXPECT_EQ(program.Ast(), nullptr);
}

// Test Ast getter returns nullptr when not set
TEST_F(ProgramTest, GetAst_WhenNotSet_ReturnsNullptr)
{
    Program program(ScriptExtension::JS);

    EXPECT_EQ(program.Ast(), nullptr);
    EXPECT_EQ(program.Ast(), nullptr); // Const version
}

// Test SetKind with SCRIPT sets kind to SCRIPT
TEST_F(ProgramTest, SetKind_SCRIPT_SetsKindToScript)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::SCRIPT);

    EXPECT_EQ(program.Kind(), ScriptKind::SCRIPT);
    EXPECT_FALSE(program.IsCommonjs());
    EXPECT_EQ(program.ModuleRecord(), nullptr);
    EXPECT_EQ(program.TypeModuleRecord(), nullptr);
}

// Test SetKind with MODULE sets kind to MODULE and creates module records
TEST_F(ProgramTest, SetKind_MODULE_SetsKindToModuleAndCreatesModuleRecords)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::MODULE);

    EXPECT_EQ(program.Kind(), ScriptKind::MODULE);
    EXPECT_FALSE(program.IsCommonjs());
    EXPECT_NE(program.ModuleRecord(), nullptr);
    EXPECT_NE(program.TypeModuleRecord(), nullptr);
}

// Test SetKind with COMMONJS sets kind to COMMONJS
TEST_F(ProgramTest, SetKind_COMMONJS_SetsKindToCommonjs)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::COMMONJS);

    EXPECT_EQ(program.Kind(), ScriptKind::COMMONJS);
    EXPECT_TRUE(program.IsCommonjs());
}

// Test IsCommonjs returns correct value for different kinds
TEST_F(ProgramTest, IsCommonjs_WithDifferentKinds_ReturnsCorrectly)
{
    Program program1(ScriptExtension::JS);
    program1.SetKind(ScriptKind::COMMONJS);
    EXPECT_TRUE(program1.IsCommonjs());

    Program program2(ScriptExtension::JS);
    program2.SetKind(ScriptKind::MODULE);
    EXPECT_FALSE(program2.IsCommonjs());

    Program program3(ScriptExtension::JS);
    program3.SetKind(ScriptKind::SCRIPT);
    EXPECT_FALSE(program3.IsCommonjs());
}

// Test SetHasTLA sets HasTLA flag correctly
TEST_F(ProgramTest, SetHasTLA_SetsHasTLAFlag)
{
    Program program(ScriptExtension::JS);

    EXPECT_FALSE(program.HasTLA());

    program.SetHasTLA(true);
    EXPECT_TRUE(program.HasTLA());

    program.SetHasTLA(false);
    EXPECT_FALSE(program.HasTLA());
}

// Test SetDebug sets IsDebug flag correctly
TEST_F(ProgramTest, SetDebug_SetsIsDebugFlag)
{
    Program program(ScriptExtension::JS);

    EXPECT_FALSE(program.IsDebug());

    program.SetDebug(true);
    EXPECT_TRUE(program.IsDebug());

    program.SetDebug(false);
    EXPECT_FALSE(program.IsDebug());
}

// Test SetTargetApiVersion sets target API version correctly
TEST_F(ProgramTest, SetTargetApiVersion_SetsTargetApiVersion)
{
    Program program(ScriptExtension::JS);
    const int apiVersion = 42;  // Test example API version

    program.SetTargetApiVersion(apiVersion);

    EXPECT_EQ(program.TargetApiVersion(), apiVersion);
}

// Test SetTargetApiVersion with zero value sets to zero
TEST_F(ProgramTest, SetTargetApiVersion_Zero_SetsToZero)
{
    Program program(ScriptExtension::JS);
    program.SetTargetApiVersion(defaultTargetApiVersion);  // Set a non-zero value first
    program.SetTargetApiVersion(0);

    EXPECT_EQ(program.TargetApiVersion(), 0);
}

// Test SetTargetApiVersion with negative value sets to negative
TEST_F(ProgramTest, SetTargetApiVersion_Negative_SetsToNegative)
{
    Program program(ScriptExtension::JS);
    const int negativeApiVersion = -1;

    program.SetTargetApiVersion(negativeApiVersion);

    EXPECT_EQ(program.TargetApiVersion(), negativeApiVersion);
}

// Test SetTargetApiSubVersion sets target API sub version correctly
TEST_F(ProgramTest, SetTargetApiSubVersion_SetsTargetApiSubVersion)
{
    Program program(ScriptExtension::JS);
    const std::string subVersion = "1.2.3";

    program.SetTargetApiSubVersion(subVersion);

    EXPECT_EQ(program.GetTargetApiSubVersion(), subVersion);
}

// Test SetDefineSemantic sets UseDefineSemantic flag correctly
TEST_F(ProgramTest, SetDefineSemantic_SetsUseDefineSemantic)
{
    Program program(ScriptExtension::JS);

    EXPECT_TRUE(program.UseDefineSemantic());

    program.SetDefineSemantic(false);
    EXPECT_FALSE(program.UseDefineSemantic());

    program.SetDefineSemantic(true);
    EXPECT_TRUE(program.UseDefineSemantic());
}

// Test SetShared sets IsShared flag correctly
TEST_F(ProgramTest, SetShared_SetsIsSharedFlag)
{
    Program program(ScriptExtension::JS);

    EXPECT_FALSE(program.IsShared());

    program.SetShared(true);
    EXPECT_TRUE(program.IsShared());

    program.SetShared(false);
    EXPECT_FALSE(program.IsShared());
}

// Test SetModuleRecordFieldName sets module record field name correctly
TEST_F(ProgramTest, SetModuleRecordFieldName_SetsModuleRecordFieldName)
{
    Program program(ScriptExtension::JS);
    const std::string fieldName = "moduleRecord";

    program.SetModuleRecordFieldName(fieldName);

    EXPECT_EQ(program.ModuleRecordFieldName(), fieldName);
}

// Test SetModuleRecordFieldName with empty string sets to empty
TEST_F(ProgramTest, SetModuleRecordFieldName_EmptyString_SetsToEmpty)
{
    Program program(ScriptExtension::JS);
    program.SetModuleRecordFieldName("");
    EXPECT_EQ(program.ModuleRecordFieldName(), "");
}

// Test SetEnableAnnotations sets EnableAnnotations flag correctly
TEST_F(ProgramTest, SetEnableAnnotations_SetsEnableAnnotationsFlag)
{
    Program program(ScriptExtension::JS);

    EXPECT_FALSE(program.IsEnableAnnotations());

    program.SetEnableAnnotations(true);
    EXPECT_TRUE(program.IsEnableAnnotations());

    program.SetEnableAnnotations(false);
    EXPECT_FALSE(program.IsEnableAnnotations());
}

// Test SetEnableEtsImplements sets EnableEtsImplements flag correctly
TEST_F(ProgramTest, SetEnableEtsImplements_SetsEnableEtsImplementsFlag)
{
    Program program(ScriptExtension::JS);

    EXPECT_FALSE(program.IsEnableEtsImplements());

    program.SetEnableEtsImplements(true);
    EXPECT_TRUE(program.IsEnableEtsImplements());

    program.SetEnableEtsImplements(false);
    EXPECT_FALSE(program.IsEnableEtsImplements());
}

// Test SetSourceLang with "ets" sets source language to ARKTS
TEST_F(ProgramTest, SetSourceLang_Ets_SetsToArkts)
{
    Program program(ScriptExtension::JS);
    program.SetSourceLang("ets");

    EXPECT_EQ(program.SourceLang(), panda::pandasm::extensions::Language::ARKTS);
}

// Test SetSourceLang with "ts" sets source language to TYPESCRIPT
TEST_F(ProgramTest, SetSourceLang_Ts_SetsToTypescript)
{
    Program program(ScriptExtension::JS);
    program.SetSourceLang("ts");

    EXPECT_EQ(program.SourceLang(), panda::pandasm::extensions::Language::TYPESCRIPT);
}

// Test SetSourceLang with "js" sets source language to JAVASCRIPT
TEST_F(ProgramTest, SetSourceLang_Js_SetsToJavascript)
{
    Program program(ScriptExtension::JS);
    program.SetSourceLang("js");

    EXPECT_EQ(program.SourceLang(), panda::pandasm::extensions::Language::JAVASCRIPT);
}

// Test SetSourceLang with unknown string sets source language to ECMASCRIPT
TEST_F(ProgramTest, SetSourceLang_Unknown_SetsToEcmascript)
{
    Program program(ScriptExtension::JS);
    program.SetSourceLang("unknown");

    EXPECT_EQ(program.SourceLang(), panda::pandasm::extensions::Language::ECMASCRIPT);
}

// Test SetSourceLang with empty string sets source language to ECMASCRIPT
TEST_F(ProgramTest, SetSourceLang_EmptyString_SetsToEcmascript)
{
    Program program(ScriptExtension::JS);
    program.SetSourceLang("");

    EXPECT_EQ(program.SourceLang(), panda::pandasm::extensions::Language::ECMASCRIPT);
}

// Test SourceCode returns correct source code
TEST_F(ProgramTest, SourceCode_ReturnsCorrectSourceCode)
{
    Program program(ScriptExtension::JS);
    const std::string source = "function test() { return 42; }";

    program.SetSource(source, "test.js", false);

    EXPECT_EQ(program.SourceCode().Utf8(), source);
}

// Test SourceFile returns correct source file name
TEST_F(ProgramTest, SourceFile_ReturnsCorrectSourceFile)
{
    Program program(ScriptExtension::JS);
    const std::string filename = "myTestFile.js";

    program.SetSource("source", filename, false);

    EXPECT_EQ(program.SourceFile().Utf8(), filename);
}

// Test GetLineIndex returns valid line index
TEST_F(ProgramTest, GetLineIndex_ReturnsValidLineIndex)
{
    Program program(ScriptExtension::JS);
    program.SetSource("line1\nline2\nline3", "test.js", false);

    EXPECT_NO_THROW((void)program.GetLineIndex());
}

// Test Binder returns non-null binder
TEST_F(ProgramTest, Binder_ReturnsNonNullBinder)
{
    Program program(ScriptExtension::JS);

    auto *binder = program.Binder();
    EXPECT_NE(binder, nullptr);
    EXPECT_EQ(binder, program.Binder());
}

// Test Binder (const version) returns non-null binder
TEST_F(ProgramTest, Binder_Const_ReturnsNonNullBinder)
{
    const Program program(ScriptExtension::JS);

    auto *binder = program.Binder();
    EXPECT_NE(binder, nullptr);
}

// Test Allocator returns non-null allocator
TEST_F(ProgramTest, Allocator_ReturnsNonNullAllocator)
{
    Program program(ScriptExtension::JS);

    auto *allocator = program.Allocator();
    EXPECT_NE(allocator, nullptr);
    EXPECT_EQ(allocator, program.Allocator());
}

// Test AddPatchFixHelper sets patch fix helper correctly
TEST_F(ProgramTest, AddPatchFixHelper_SetsPatchFixHelper)
{
    Program program(ScriptExtension::JS);
    EXPECT_EQ(program.PatchFixHelper(), nullptr);

    util::PatchFix *mockPatchFix = reinterpret_cast<util::PatchFix *>(0x1234);
    program.AddPatchFixHelper(mockPatchFix);

    EXPECT_EQ(program.PatchFixHelper(), mockPatchFix);
}

// Test PatchFixHelper returns nullptr when not set
TEST_F(ProgramTest, PatchFixHelper_WhenNotSet_ReturnsNullptr)
{
    Program program(ScriptExtension::JS);
    EXPECT_EQ(program.PatchFixHelper(), nullptr);
}

// Test Dump returns non-empty string with valid AST
TEST_F(ProgramTest, Dump_ReturnsNonEmptyStringWithValidAst)
{
    auto program = ParseSource("const x = 5;");
    ASSERT_NE(program.Ast(), nullptr);

    std::string dumpResult = program.Dump();
    EXPECT_FALSE(dumpResult.empty());

    const Program &constProgram = program;
    std::string constDumpResult = constProgram.Dump();
    EXPECT_FALSE(constDumpResult.empty());
}

// Test ModuleRecord with MODULE kind returns non-null
TEST_F(ProgramTest, ModuleRecord_WithModuleKind_ReturnsNonNull)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::MODULE);

    EXPECT_NE(program.ModuleRecord(), nullptr);
}

// Test ModuleRecord with SCRIPT kind returns null
TEST_F(ProgramTest, ModuleRecord_WithScriptKind_ReturnsNull)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::SCRIPT);

    EXPECT_EQ(program.ModuleRecord(), nullptr);
}

// Test TypeModuleRecord with MODULE kind returns non-null
TEST_F(ProgramTest, TypeModuleRecord_WithModuleKind_ReturnsNonNull)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::MODULE);

    EXPECT_NE(program.TypeModuleRecord(), nullptr);
}

// Test TypeModuleRecord with SCRIPT kind returns null
TEST_F(ProgramTest, TypeModuleRecord_WithScriptKind_ReturnsNull)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::SCRIPT);

    EXPECT_EQ(program.TypeModuleRecord(), nullptr);
}

// Test FormatedRecordName returns correctly formatted name with period suffix
TEST_F(ProgramTest, FormatedRecordName_ReturnsCorrectFormattedName)
{
    Program program(ScriptExtension::JS);
    const std::string recordName = "testModule";

    program.SetRecordName(recordName);

    EXPECT_EQ(program.FormatedRecordName().Utf8(), recordName + ".");

    const std::string anotherRecordName = "anotherModule";
    program.SetRecordName(anotherRecordName);

    EXPECT_EQ(program.FormatedRecordName().Utf8(), anotherRecordName + ".");
    EXPECT_NE(program.RecordName().Utf8(), program.FormatedRecordName().Utf8());
    EXPECT_TRUE(program.FormatedRecordName().Utf8().find('.') != std::string::npos);
}

// Test Extension returns correct script extension for different program types
TEST_F(ProgramTest, Extension_ReturnsCorrectExtension)
{
    Program jsProgram(ScriptExtension::JS);
    EXPECT_EQ(jsProgram.Extension(), ScriptExtension::JS);

    Program tsProgram(ScriptExtension::TS);
    EXPECT_EQ(tsProgram.Extension(), ScriptExtension::TS);

    Program asProgram(ScriptExtension::AS);
    EXPECT_EQ(asProgram.Extension(), ScriptExtension::AS);

    jsProgram.SetSource("test", "test.js", false);
    jsProgram.SetKind(ScriptKind::MODULE);
    EXPECT_EQ(jsProgram.Extension(), ScriptExtension::JS);

    Program abcProgram(ScriptExtension::ABC);
    EXPECT_EQ(abcProgram.Extension(), ScriptExtension::ABC);
}

// Test Kind before SetKind returns default value (SCRIPT)
TEST_F(ProgramTest, Kind_BeforeSetKind_ReturnsDefault)
{
    Program program(ScriptExtension::JS);
    EXPECT_EQ(program.Kind(), ScriptKind::SCRIPT);
}

// Test Kind after SetKind returns the set kind value
TEST_F(ProgramTest, Kind_AfterSetKind_ReturnsCorrectKind)
{
    Program program(ScriptExtension::JS);
    program.SetKind(ScriptKind::MODULE);

    EXPECT_EQ(program.Kind(), ScriptKind::MODULE);
}

// Test multiple SetSource calls update source correctly
TEST_F(ProgramTest, MultipleSetSourceCalls_UpdatesSourceCorrectly)
{
    Program program(ScriptExtension::JS);
    program.SetSource("source1", "file1.js", false);
    EXPECT_EQ(program.SourceCode().Utf8(), "source1");
    EXPECT_EQ(program.SourceFile().Utf8(), "file1.js");

    program.SetSource("source2", "file2.js", true);
    EXPECT_EQ(program.SourceCode().Utf8(), "source2");
    EXPECT_EQ(program.SourceFile().Utf8(), "file2.js");
    EXPECT_TRUE(program.IsDtsFile());
}

// Test multiple SetRecordName calls update record name correctly
TEST_F(ProgramTest, MultipleSetRecordNameCalls_UpdatesRecordNameCorrectly)
{
    Program program(ScriptExtension::JS);
    program.SetRecordName("record1");
    EXPECT_EQ(program.RecordName().Utf8(), "record1");

    program.SetRecordName("record2");
    EXPECT_EQ(program.RecordName().Utf8(), "record2");
}

// Test multiple boolean flags can be set independently
TEST_F(ProgramTest, MultipleBooleanFlags_AllSetIndependently)
{
    Program program(ScriptExtension::JS);
    program.SetSource("test", "test.d.ts", true);

    program.SetHasTLA(true);
    program.SetDebug(true);
    program.SetShared(true);
    program.SetEnableAnnotations(true);
    program.SetEnableEtsImplements(true);
    program.SetDefineSemantic(false);

    EXPECT_TRUE(program.IsDtsFile());
    EXPECT_TRUE(program.HasTLA());
    EXPECT_TRUE(program.IsDebug());
    EXPECT_TRUE(program.IsShared());
    EXPECT_TRUE(program.IsEnableAnnotations());
    EXPECT_TRUE(program.IsEnableEtsImplements());
    EXPECT_FALSE(program.UseDefineSemantic());
}

// Test default values of all Program properties
TEST_F(ProgramTest, DefaultValues_CheckInitialDefaults)
{
    Program program(ScriptExtension::JS);

    EXPECT_EQ(program.TargetApiVersion(), 0);
    EXPECT_TRUE(program.UseDefineSemantic());
    EXPECT_FALSE(program.IsShared());
    EXPECT_FALSE(program.IsDebug());
    EXPECT_FALSE(program.HasTLA());
    EXPECT_FALSE(program.IsDtsFile());
    EXPECT_FALSE(program.IsEnableAnnotations());
    EXPECT_FALSE(program.IsEnableEtsImplements());
    EXPECT_EQ(program.GetTargetApiSubVersion(), "beta1");
    EXPECT_EQ(program.ModuleRecordFieldName(), "");
}

// Test Allocator can create objects successfully
TEST_F(ProgramTest, Allocator_CanCreateObjects)
{
    Program program(ScriptExtension::JS);
    auto *allocator = program.Allocator();

    auto *obj = allocator->New<int>(testAllocatorValue);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(*obj, testAllocatorValue);
}

// Test Binder is properly initialized with correct Program reference
TEST_F(ProgramTest, Binder_IsProperlyInitialized)
{
    Program program(ScriptExtension::JS);
    auto *binder = program.Binder();

    EXPECT_NE(binder, nullptr);
    EXPECT_EQ(binder->Program(), &program);
}

// Test LineIndex is updated when source changes
TEST_F(ProgramTest, LineIndex_UpdatedWhenSourceChanges)
{
    Program program(ScriptExtension::JS);
    program.SetSource("line1\nline2", "test.js", false);

    EXPECT_NO_THROW((void)program.GetLineIndex());

    program.SetSource("line1\nline2\nline3\nline4", "test.js", false);

    EXPECT_NO_THROW((void)program.GetLineIndex());
    EXPECT_EQ(program.SourceCode().Utf8(), "line1\nline2\nline3\nline4");
}

// Test different programs have different allocators
TEST_F(ProgramTest, DifferentPrograms_HaveDifferentAllocators)
{
    Program program1(ScriptExtension::JS);
    Program program2(ScriptExtension::JS);

    EXPECT_NE(program1.Allocator(), program2.Allocator());
}

// Test allocator is preserved after move construction
TEST_F(ProgramTest, MoveConstructor_PreservesAllocator)
{
    Program original(ScriptExtension::JS);
    original.SetSource("var x = 42;", "test.js", false);

    auto *originalAllocator = original.Allocator();
    ASSERT_NE(originalAllocator, nullptr);

    Program moved(std::move(original));

    EXPECT_EQ(moved.Allocator(), originalAllocator);
    EXPECT_EQ(original.Allocator(), nullptr);
}

// Test allocator is preserved after move assignment
TEST_F(ProgramTest, MoveAssignmentOperator_PreservesAllocator)
{
    Program program1(ScriptExtension::JS);
    program1.SetSource("var x = 42;", "file1.js", false);

    Program program2(ScriptExtension::JS);
    program2.SetSource("var y = 100;", "file2.js", false);

    auto *originalAllocator = program1.Allocator();
    ASSERT_NE(originalAllocator, nullptr);

    program2 = std::move(program1);

    EXPECT_EQ(program2.Allocator(), originalAllocator);
    EXPECT_EQ(program1.Allocator(), nullptr);
}

// Test multiple program creation and destruction (for ASAN memory leak detection)
TEST_F(ProgramTest, MultipleProgramCreation_NoMemoryLeak)
{
    for (int i = 0; i < memoryLeakTestIterations; i++) {
        Program program(ScriptExtension::JS);
        program.SetSource("var x = 42;", "test.js", false);
        auto *allocator = program.Allocator();
        // Allocate some objects to test allocator functionality
        auto *obj = allocator->New<int>(i);
        EXPECT_EQ(*obj, i);
        // Program will be destroyed here
    }
    // Test with ASAN. If this passes without crash, no memory leak
}

} // namespace panda::es2panda::parser
