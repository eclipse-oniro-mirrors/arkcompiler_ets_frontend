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

#ifndef ES2PANDA_EVALUATE_VARBINDER_SCOPES_H
#define ES2PANDA_EVALUATE_VARBINDER_SCOPES_H

#include "parser/program/program.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/recordTable.h"

namespace ark::es2panda::evaluate {

// This scope must be used before running VarBinder or Checker on nodes from another Program.
class ProgramScope final {
public:
    explicit ProgramScope(varbinder::ETSBinder *varBinder, parser::Program *program)
        : varBinder_(varBinder),
          prevProgram_(varBinder->Program()),
          prevRecordTable_(varBinder->GetRecordTable()),
          prevTopScope_(varBinder->TopScope()),
          prevVarScope_(varBinder->VarScope()),
          prevScope_(varBinder->GetScope())
    {
        ES2PANDA_ASSERT(varBinder);
        ES2PANDA_ASSERT(program);
        ES2PANDA_ASSERT(varBinder_->CheckRecordTablesConsistency(program));

        varBinder_->SetProgram(program);
        varBinder_->SetRecordTable(program->GetRecordTable());
        varBinder_->ResetAllScopes(program->GlobalScope(), program->GlobalScope(), program->GlobalScope());
    }

    ~ProgramScope() noexcept
    {
        ES2PANDA_ASSERT(prevProgram_->GetRecordTable() == prevRecordTable_);
        varBinder_->SetProgram(prevProgram_);
        varBinder_->SetRecordTable(prevRecordTable_);
        varBinder_->ResetAllScopes(prevTopScope_, prevVarScope_, prevScope_);
    }

    NO_COPY_SEMANTIC(ProgramScope);
    NO_MOVE_SEMANTIC(ProgramScope);

    void *operator new(size_t) = delete;
    void *operator new[](size_t) = delete;

private:
    varbinder::ETSBinder *varBinder_ {nullptr};
    parser::Program *prevProgram_ {nullptr};
    varbinder::RecordTable *prevRecordTable_ {nullptr};
    varbinder::GlobalScope *prevTopScope_ {nullptr};
    varbinder::VariableScope *prevVarScope_ {nullptr};
    varbinder::Scope *prevScope_ {nullptr};
};

// The scope is required for running VarBinder or Checker on nodes from another class,
// so that entities will be registered with correct names in record table.
class RecordTableClassScope final {
public:
    explicit RecordTableClassScope(varbinder::ETSBinder *varBinder, ir::AstNode *recordClass) : varBinder_(varBinder)
    {
        ES2PANDA_ASSERT(varBinder_);

        auto *recordTable = varBinder_->GetRecordTable();
        ES2PANDA_ASSERT(recordTable);

        prevRecordClass_ = recordTable->ClassDefinition();
        if (prevRecordClass_ == nullptr) {
            prevRecordClass_ = recordTable->InterfaceDeclaration();
        }

        if (recordClass != nullptr) {
            if (recordClass->IsClassDefinition()) {
                recordTable->SetClassDefinition(recordClass->AsClassDefinition());
            } else if (recordClass->IsAnnotationDeclaration()) {
                recordTable->SetAnnotationDeclaration(recordClass->AsAnnotationDeclaration());
            } else {
                recordTable->SetInterfaceDeclaration(recordClass->AsTSInterfaceDeclaration());
            }
        } else {
            ir::ClassDefinition *nullDef = nullptr;
            recordTable->SetClassDefinition(nullDef);
        }
    }

    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~RecordTableClassScope() noexcept
    {
        auto *recordTable = varBinder_->GetRecordTable();
        ES2PANDA_ASSERT(recordTable != nullptr);

        if (prevRecordClass_ != nullptr) {
            if (prevRecordClass_->IsClassDefinition()) {
                recordTable->SetClassDefinition(prevRecordClass_->AsClassDefinition());
            } else {
                recordTable->SetInterfaceDeclaration(prevRecordClass_->AsTSInterfaceDeclaration());
            }
        } else {
            ir::ClassDefinition *nullDef = nullptr;
            recordTable->SetClassDefinition(nullDef);
        }
    }

    NO_COPY_SEMANTIC(RecordTableClassScope);
    NO_MOVE_SEMANTIC(RecordTableClassScope);

    void *operator new(size_t) = delete;
    void *operator new[](size_t) = delete;

private:
    varbinder::ETSBinder *varBinder_ {nullptr};
    ir::AstNode *prevRecordClass_ {nullptr};
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_VARBINDER_SCOPES_H
