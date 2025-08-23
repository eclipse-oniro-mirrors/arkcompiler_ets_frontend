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

#include <cstdint>
#include <memory>
#include "file.h"
#include "macros.h"
#include "os/file.h"
#include "public/es2panda_lib.h"
#include "util.h"
#include "parser/program/program.h"
#include "ir/statements/blockStatement.h"
#include "ir/astNode.h"
#include "ir/statements/classDeclaration.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "public/public.h"
#include "util/options.h"
#include "abc2program_driver.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

namespace {
constexpr size_t CLASS_DEFINITION_INDEX = 1;
constexpr size_t METHOD_DEFINITION_INDEX = 0;
constexpr size_t METHOD_BODY_INDEX = 2;

enum Const {
    LITERAL_INDEX_0 = 0,
    LITERAL_INDEX_1 = 1,
    LITERAL_INDEX_2 = 2,
    LITERAL_INDEX_3 = 3,
    LITERAL_INDEX_4 = 4,
    LITERAL_INDEX_5 = 5,
    EXPECTED_VALUE_1 = 1,
    EXPECTED_VALUE_2 = 2,
};

const std::string SOURCE_CODE =
    "   function foo():void {\n"
    "   }\n"
    "   class A{}\n";

const std::string EXPECTED_CLASS_DEFINITION = "class A {\n  public constructor() {}\n  \n}\n";

const std::string EXPECTED_METHOD_DEFINITION = "function foo(): void {}\n";

int CheckLiteralValues(ark::es2panda::public_lib::Context *ctx, const ark::pandasm::Program &prog)
{
    auto file = ark::panda_file::OpenPandaFile(ctx->config->options->GetOutput());
    if (!file) {
        return TEST_ERROR_CODE;
    }

    auto literalArrIt = prog.literalarrayTable.find("0");
    if (literalArrIt == prog.literalarrayTable.end()) {
        return TEST_ERROR_CODE;
    }
    auto &literalArr = literalArrIt->second.literals;
    if (EXPECTED_VALUE_2 != std::get<uint8_t>(literalArr[LITERAL_INDEX_1].value) ||
        EXPECTED_VALUE_1 != std::get<uint8_t>(literalArr[LITERAL_INDEX_4].value)) {
        return TEST_ERROR_CODE;
    }

    const auto offsetA = std::get<uint32_t>(literalArr[LITERAL_INDEX_0].value);
    const auto stringDataA = file->GetStringData(ark::panda_file::File::EntityId(offsetA));
    auto stringA = std::string(reinterpret_cast<const char *>(stringDataA.data), stringDataA.utf16Length);

    const auto offsetFoo = std::get<uint32_t>(literalArr[LITERAL_INDEX_3].value);
    const auto stringDataFoo = file->GetStringData(ark::panda_file::File::EntityId(offsetFoo));
    auto stringFoo = std::string(reinterpret_cast<const char *>(stringDataFoo.data), stringDataFoo.utf16Length);

    if (stringA != EXPECTED_CLASS_DEFINITION) {
        return TEST_ERROR_CODE;
    }

    if (stringFoo != EXPECTED_METHOD_DEFINITION) {
        return TEST_ERROR_CODE;
    }

    return 0;
}

void ProcessProgram(ark::es2panda::parser::Program *programPtr)
{
    auto *classDef = programPtr->Ast()->Statements()[CLASS_DEFINITION_INDEX]->AsClassDeclaration()->Definition();
    programPtr->AddDeclGenExportNode(classDef->DumpEtsSrc(), classDef);

    auto *methodDef = programPtr->Ast()
                          ->Statements()[METHOD_DEFINITION_INDEX]
                          ->AsClassDeclaration()
                          ->Definition()
                          ->AsClassDefinition()
                          ->Body()[METHOD_BODY_INDEX]
                          ->AsMethodDefinition();
    programPtr->AddDeclGenExportNode(methodDef->DumpEtsSrc(), methodDef);
}
}  // namespace

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
    auto config = impl->CreateConfig(argc - 1, args);
    auto config_ = reinterpret_cast<ark::es2panda::public_lib::ConfigImpl *>(config);
    const_cast<ark::es2panda::util::Options *>(config_->options)->SetWithExportTable(true);
    auto context = impl->CreateContextFromString(config, SOURCE_CODE.data(), argv[argc - 1]);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    CheckForErrors("BOUND", context);

    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    CheckForErrors("CHECKED", context);

    auto program = impl->ContextProgram(context);
    auto programPtr = reinterpret_cast<ark::es2panda::parser::Program *>(program);
    ProcessProgram(programPtr);

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);

    impl->ProceedToState(context, ES2PANDA_STATE_ASM_GENERATED);
    CheckForErrors("ASM", context);

    impl->ProceedToState(context, ES2PANDA_STATE_BIN_GENERATED);
    CheckForErrors("BIN", context);

    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto abc2program = new ark::abc2program::Abc2ProgramDriver();
    abc2program->Compile("./use_plugin_to_test_export_table.abc");
    auto res = CheckLiteralValues(ctx, abc2program->GetProgram());
    impl->DestroyConfig(config);
    return res;
}

// NOLINTEND