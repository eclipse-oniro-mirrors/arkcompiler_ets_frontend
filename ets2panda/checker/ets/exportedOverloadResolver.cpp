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

#include "checker/ets/exportedOverloadResolver.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsFunctionType.h"
#include "ir/astNode.h"

namespace ark::es2panda::checker::ets {

static bool IsIndividuallyExportedSignature(const Signature *signature)
{
    if (signature == nullptr || !signature->HasFunction()) {
        return false;
    }

    const ir::AstNode *owner = signature->Function();
    while (owner != nullptr && !owner->IsMethodDefinition()) {
        owner = owner->Parent();
    }
    return owner != nullptr && (owner->IsExported() || owner->IsDefaultExported());
}

ExportedOverloadView ResolveExportedOverloadView(ETSChecker *checker, Type *sourceType, varbinder::LocalVariable *owner,
                                                 const ir::AstNode *exportOrigin, bool exportsWholeBinding)
{
    if (sourceType == nullptr || !sourceType->IsETSFunctionType() || exportsWholeBinding ||
        (exportOrigin != nullptr && exportOrigin->IsExportSpecifier())) {
        return {sourceType, false};
    }

    const auto &signatures = sourceType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow();
    ArenaVector<Signature *> exported {checker->ProgramAllocator()->Adapter()};
    for (auto *signature : signatures) {
        if (IsIndividuallyExportedSignature(signature)) {
            exported.push_back(signature);
        }
    }
    if (exported.empty() || exported.size() == signatures.size()) {
        return {sourceType, false};
    }

    auto *filtered = checker->CreateETSMethodType(sourceType->AsETSFunctionType()->Name(), std::move(exported));
    if (owner != nullptr) {
        filtered->SetVariable(owner);
    }
    return {filtered, true};
}

}  // namespace ark::es2panda::checker::ets
