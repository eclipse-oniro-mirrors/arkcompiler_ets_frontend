/**
 * Copyright (c) 2021 - 2025 Huawei Device Co., Ltd.
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

#include "savedBindingsCtx.h"

#include <ir/module/exportDefaultDeclaration.h>
#include <ir/module/exportAllDeclaration.h>
#include <ir/statements/functionDeclaration.h>
#include <ir/base/scriptFunction.h>

namespace ark::es2panda::compiler {

void ImportDeclarationContext::BindImportDecl(ir::ImportDeclaration *importDecl)
{
    varbinder::ModuleScope::ImportDeclList declList(Allocator()->Adapter());

    for (const auto &[name, variable] : VarBinder()->GetScope()->OrderedBindings(Allocator())) {
        if (SavedBindings().find(name) != SavedBindings().end()) {
            continue;
        }

        declList.push_back(variable->Declaration()->AsImportDecl());
    }

    VarBinder()->GetScope()->AsModuleScope()->AddImportDecl(importDecl, std::move(declList));
}

void ExportDeclarationContext::BindExportDecl(ir::AstNode *exportDecl)
{
    if (VarBinder() == nullptr) {
        return;
    }

    varbinder::ModuleScope::ExportDeclList declList(Allocator()->Adapter());

    if (exportDecl->IsExportDefaultDeclaration()) {
        auto *decl = exportDecl->AsExportDefaultDeclaration();
        auto *rhs = decl->Decl();

        if (VarBinder()->GetScope()->Bindings().size() == SavedBindings().size()) {
            if (rhs->IsFunctionDeclaration()) {
                VarBinder()->AddDecl<varbinder::FunctionDecl>(rhs->Start(), VarBinder()->Allocator(),
                                                              util::StringView(DEFAULT_EXPORT),
                                                              rhs->AsFunctionDeclaration()->Function());
            } else {
                VarBinder()->AddDecl<varbinder::ConstDecl>(rhs->Start(), util::StringView(DEFAULT_EXPORT), rhs);
            }
        }
    }

    for (const auto &[name, variable] : VarBinder()->GetScope()->Bindings()) {
        if (SavedBindings().find(name) != SavedBindings().end()) {
            continue;
        }

        util::StringView exportName(exportDecl->IsExportDefaultDeclaration() ? "default" : name);

        variable->AddFlag(varbinder::VariableFlags::LOCAL_EXPORT);
        auto *decl =
            VarBinder()->AddDecl<varbinder::ExportDecl>(variable->Declaration()->Node()->Start(), exportName, name);
        declList.push_back(decl);
    }

    auto *moduleScope = VarBinder()->GetScope()->AsModuleScope();
    moduleScope->AddExportDecl(exportDecl, std::move(declList));
}

}  // namespace ark::es2panda::compiler