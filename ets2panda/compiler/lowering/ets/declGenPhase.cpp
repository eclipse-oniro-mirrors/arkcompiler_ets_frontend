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

#include "declGenPhase.h"

#include <fstream>
#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "ir/ets/etsPackageDeclaration.h"
#include "util/helpers.h"
#include "util/path.h"
#include "util/importPathManager.h"
#include "parser/program/program.h"

namespace ark::es2panda::compiler {

constexpr std::string_view MODULE_DECLARATION_NAME {"ModuleDeclaration"};
constexpr std::string_view DECLARATION_STRING {"declaration"};
static bool GenerateAnnotation(public_lib::Context *ctx, parser::Program *program, const std::string &decls)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *phaseManager = ctx->phaseManager;
    auto *allocator = ctx->Allocator();

    // Arena cause we want declaration be life until codegen happens
    auto *declaration = allocator->New<ArenaString>(decls, allocator->Adapter());
    ES2PANDA_ASSERT(declaration != nullptr);

    auto *const annoUsageIdent = checker->AllocNode<ir::Identifier>(MODULE_DECLARATION_NAME, checker->Allocator());
    annoUsageIdent->SetAnnotationUsage();

    auto flags = ir::ModifierFlags::ANNOTATION_USAGE;
    ArenaVector<ir::AstNode *> properties(checker->Allocator()->Adapter());
    auto *valueParamName = checker->AllocNode<ir::Identifier>(DECLARATION_STRING, checker->Allocator());
    auto *declarationLiteral = checker->AllocNode<ir::StringLiteral>(declaration->c_str());
    auto *valueProp = checker->AllocNode<ir::ClassProperty>(valueParamName, declarationLiteral, nullptr, flags,
                                                            checker->Allocator(), false);
    properties.push_back(valueProp);

    auto *annotationUsage = checker->AllocNode<ir::AnnotationUsage>(annoUsageIdent, std::move(properties));
    annotationUsage->AddModifier(flags);
    annotationUsage->SetParent(program->GlobalClass());

    program->GlobalClass()->EmplaceAnnotation(annotationUsage);
    Recheck(phaseManager, checker->VarBinder()->AsETSBinder(), checker, annotationUsage);
    return true;
}

static bool CallDeclgen(public_lib::Context *ctx, const ArenaVector<parser::Program *> &programs)
{
    ir::Declgen dg {ctx};
    ir::SrcDumper dumper {&dg};
    for (const auto *program : programs) {
        program->Ast()->Dump(&dumper);
    }
    dumper.GetDeclgen()->Run();
    std::string res = "'use static'\n";
    dg.DumpImports(res);
    res += dumper.Str();
    return GenerateAnnotation(ctx, programs[0], res);
}

static bool HandleGenStdlib(public_lib::Context *ctx, parser::Program *program)
{
    if (!program->FileName().Is("etsstdlib")) {
        // Generation is handled only for main program for all modules at once:
        return true;
    }

    for (const auto &[moduleName, extPrograms] : program->ExternalSources()) {
        CallDeclgen(ctx, extPrograms);
    }
    return true;
}

bool DeclGenPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (ctx->config->options->IsGenStdlib()) {
        return HandleGenStdlib(ctx, program);
    }

    if (!ctx->config->options->IsEmitDeclaration()) {
        return true;
    }

    const auto &sourceFilePath = program->AbsoluteName().Utf8();
    if (sourceFilePath.empty()) {
        return true;
    }

    auto programs = ArenaVector<parser::Program *> {program->Allocator()->Adapter()};
    programs.push_back(program);
    CallDeclgen(ctx, programs);

    return true;
}

}  // namespace ark::es2panda::compiler
