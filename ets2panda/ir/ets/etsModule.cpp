/*
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

#include "etsModule.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::ir {

void ETSModule::Dump(ir::SrcDumper *dumper) const
{
    if (IsNamespace()) {
        if (IsExported()) {
            dumper->Add("export ");
        }

        if (IsDefaultExported()) {
            dumper->Add("export default ");
        }

        if (IsDeclare() && !(parent_ != nullptr && parent_->IsDeclare())) {
            dumper->Add("declare ");
        }

        dumper->Add("namespace ");
        Ident()->Dump(dumper);
        dumper->Add(" {");
        dumper->IncrIndent();
    }

    if (!Statements().empty()) {
        dumper->Endl();
        for (auto elem : Statements()) {
            elem->Dump(dumper);
            if (elem != Statements().back()) {
                dumper->Endl();
            }
        }
    }
    if (IsNamespace()) {
        dumper->DecrIndent();
    }
    dumper->Endl();
    if (IsNamespace()) {
        dumper->Add("}");
    }
}

ETSModule *ETSModule::Construct(ArenaAllocator *allocator)
{
    ArenaVector<Statement *> statementList(allocator->Adapter());
    return allocator->New<ETSModule>(allocator, std::move(statementList), nullptr, ModuleFlag::NONE, lang_, nullptr);
}

void ETSModule::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsETSModule();

    otherImpl->ident_ = ident_;
    otherImpl->flag_ = flag_;
    otherImpl->program_ = program_;
    otherImpl->globalClass_ = globalClass_;

    AnnotationAllowed<BlockStatement>::CopyTo(other);
}

}  // namespace ark::es2panda::ir
