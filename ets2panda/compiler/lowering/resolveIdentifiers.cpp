/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

static void InsertReExported(parser::Program *program, varbinder::ETSBinder *pVarBinder, parser::Program *extProgram)
{
    auto etsBinder = extProgram->VarBinder()->AsETSBinder();
    auto &reExportedImports = pVarBinder->ReExportImports();
    for (auto &[progWithReexports, reexports] : etsBinder->ReExportImports()) {
        if (progWithReexports != program) {
            reExportedImports[progWithReexports] = reexports;
        }
    }

    auto &aliasMap = pVarBinder->GetSelectiveExportAliasMultimap();
    aliasMap.insert(etsBinder->GetSelectiveExportAliasMultimap().begin(),
                    etsBinder->GetSelectiveExportAliasMultimap().end());
}

void ResolveIdentifiers::Setup()
{
    // NOTE(dkofanov): If present, the whole cache should be restored at once, at program-restoration. To be moved.
    auto *program = Context()->parserProgram;
    program->GetExternalSources()->Visit([program](auto *extProgram) {
        if (extProgram->IsASTLowered() || !extProgram->IsProgramModified()) {
            InsertReExported(program, program->VarBinder()->AsETSBinder(), extProgram);
        }
    });
}

static void DumpAstOutput(parser::Program *program, [[maybe_unused]] const std::string &dumpAstFile)
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
    std::cout << program->Dump() << std::endl;
#endif
}

bool ResolveIdentifiers::Perform()
{
    ES2PANDA_ASSERT(Context()->parserProgram != nullptr);
    auto *varbinder = Context()->parserProgram->VarBinder()->AsETSBinder();
    varbinder->SetProgram(Context()->parserProgram);

    static bool firstDump = true;
    if (Options()->IsDumpAst() && firstDump) {
        firstDump = false;
        if (!Options()->GetDumpAstOutput().empty()) {
            DumpAstOutput(varbinder->Program(), Options()->GetDumpAstOutput());
        } else {
            std::cout << varbinder->Program()->Dump() << std::endl;
        }
    }

    if (Options()->IsDumpAstOnlySilent()) {
        varbinder->Program()->DumpSilent();
    }

    if (Options()->IsParseOnly()) {
        return false;
    }

    varbinder->SetGenStdLib(Options()->GetCompilationMode() == CompilationMode::GEN_STD_LIB);
    varbinder->IdentifierAnalysis();

    return true;
}

}  // namespace ark::es2panda::compiler
