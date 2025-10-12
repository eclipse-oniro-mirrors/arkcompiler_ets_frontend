/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "resolveIdentifiers.h"

#include "varbinder/ETSBinder.h"
#include "util/options.h"

namespace ark::es2panda::compiler {

void ResolveIdentifiers::InsertReExported(parser::Program *program, varbinder::ETSBinder *pVarBinder,
                                          parser::Program *extProgram)
{
    auto etsBinder = extProgram->VarBinder()->AsETSBinder();
    auto &reExportedImports = pVarBinder->ReExportImports();
    for (auto &it : etsBinder->ReExportImports()) {
        if (it->GetTopStatement()->AsETSModule()->Program()->SourceFile().GetPath() !=
            program->SourceFile().GetPath()) {
            reExportedImports.insert(it);
        }
    }

    auto &aliasMap = pVarBinder->GetSelectiveExportAliasMultimap();
    aliasMap.insert(etsBinder->GetSelectiveExportAliasMultimap().begin(),
                    etsBinder->GetSelectiveExportAliasMultimap().end());
}

void ResolveIdentifiers::FetchCache([[maybe_unused]] public_lib::Context *ctx,
                                    [[maybe_unused]] parser::Program *program)
{
    auto pVarBinder = program->VarBinder()->AsETSBinder();
    for (auto &[package, extPrograms] : program->ExternalSources()) {
        auto *extProgram = extPrograms.front();
        if (!extProgram->IsStdLib() && extProgram->IsASTLowered()) {
            InsertReExported(program, pVarBinder, extProgram);
        }
    }
}

void ResolveIdentifiers::DumpAstOutput(parser::Program *program, const std::string &dumpAstFile)
{
#ifdef ARKTSCONFIG_USE_FILESYSTEM
    auto dumpAstFilePath = fs::path(dumpAstFile);
    fs::create_directories(dumpAstFilePath.parent_path());
    std::ofstream outputFile(dumpAstFilePath);
    if (outputFile.is_open()) {
        outputFile << program->Dump() << std::endl;
        outputFile.close();
    }
#else
    std::cout << varbinder->Program()->Dump() << std::endl;
#endif
}

bool ResolveIdentifiers::Perform(public_lib::Context *ctx, [[maybe_unused]] parser::Program *program)
{
    FetchCache(ctx, program);
    auto const *options = ctx->config->options;
    auto *varbinder = ctx->parserProgram->VarBinder()->AsETSBinder();

    static bool firstDump = true;
    if (options->IsDumpAst() && firstDump) {
        firstDump = false;
        if (!options->GetDumpAstOutput().empty()) {
            DumpAstOutput(program, options->GetDumpAstOutput());
        } else {
            std::cout << varbinder->Program()->Dump() << std::endl;
        }
    }

    if (options->IsDumpAstOnlySilent()) {
        varbinder->Program()->DumpSilent();
    }

    if (options->IsParseOnly()) {
        return false;
    }

    varbinder->SetGenStdLib(options->GetCompilationMode() == CompilationMode::GEN_STD_LIB);
    varbinder->IdentifierAnalysis();

    return true;
}

}  // namespace ark::es2panda::compiler
