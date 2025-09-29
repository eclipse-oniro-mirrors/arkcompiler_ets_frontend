/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "recordTable.h"

#include "public/public.h"
#include "parser/program/program.h"
#include "varbinder/ETSBinder.h"
#include "ir/base/classDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/identifier.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "generated/signatures.h"

namespace ark::es2panda::varbinder {
BoundContext::BoundContext(RecordTable *recordTable, ir::ClassDefinition *classDef, bool force)
    : prev_(recordTable->boundCtx_),
      recordTable_(recordTable),
      currentRecord_(classDef),
      savedRecord_(recordTable->record_)
{
    if (classDef == nullptr || (!force && !recordTable_->classDefinitions_.insert(classDef).second)) {
        return;
    }

    recordTable_->boundCtx_ = this;
    recordTable_->record_ = classDef;
    recordIdent_ = classDef->Ident();
    if (classDef->InternalName().Empty()) {
        classDef->SetInternalName(FormRecordName());
    }
}

BoundContext::BoundContext(RecordTable *recordTable, ir::TSInterfaceDeclaration *interfaceDecl, bool force)
    : prev_(recordTable->boundCtx_),
      recordTable_(recordTable),
      currentRecord_(interfaceDecl),
      savedRecord_(recordTable->record_)
{
    if (interfaceDecl == nullptr || (!force && !recordTable_->interfaceDeclarations_.insert(interfaceDecl).second)) {
        return;
    }

    recordTable_->boundCtx_ = this;
    recordTable_->record_ = interfaceDecl;
    recordIdent_ = interfaceDecl->Id();
    if (interfaceDecl->InternalName().Empty()) {
        interfaceDecl->SetInternalName(FormRecordName());
    }
}

BoundContext::BoundContext(RecordTable *recordTable, ir::AnnotationDeclaration *annotationDecl, bool force)
    : prev_(recordTable->boundCtx_),
      recordTable_(recordTable),
      currentRecord_(annotationDecl),
      savedRecord_(recordTable->record_)
{
    if (annotationDecl == nullptr || (!force && !recordTable_->annotationDeclarations_.insert(annotationDecl).second)) {
        return;
    }

    recordTable_->boundCtx_ = this;
    recordTable_->record_ = annotationDecl;
    recordIdent_ = annotationDecl->GetBaseName();
    if (annotationDecl->InternalName().Empty()) {
        annotationDecl->SetInternalName(FormRecordName());
    }
}

BoundContext::~BoundContext()
{
    recordTable_->record_ = savedRecord_;
    recordTable_->boundCtx_ = prev_;
}

static constexpr std::array<std::string_view, 1> SPECIAL_MODULE_NAMES = {
    compiler::Signatures::SIMULT_MODULE_NAME,
};

static void GetFileName(util::UString &name, RecordTable *recordTable)
{
    auto const *const ctx = recordTable->Program()->VarBinder()->GetContext();
    if (ctx->sourceFileNames.empty()) {
        return;
    }

    std::string_view fileName = *ctx->sourceFileNames.begin();
    auto pos = fileName.rfind('/');
    if (pos == std::string_view::npos) {
        pos = fileName.rfind('\\');
    }

    if (pos != std::string_view::npos) {
        fileName = fileName.substr(pos + 1U);
    }

    pos = fileName.find('.');
    if (pos != std::string_view::npos) {
        fileName = fileName.substr(0U, pos);
    }

    if (!fileName.empty()) {
        std::string modulePrefix {fileName};
        modulePrefix.append("$.");
        const_cast<util::ModuleInfo &>(recordTable->Program()->ModuleInfo()).modulePrefix = modulePrefix;
        name.Append(recordTable->Program()->ModulePrefix());
    }
}

util::StringView BoundContext::FormRecordName() const
{
    //  Special processing for building in simultaneous mode when module prefix, as well as current file name
    //  are not available. Use the first file from the list instead.
    auto const getFileName = [this](util::UString &name) -> void { GetFileName(name, recordTable_); };

    auto const checkForSpecialModuleName = [](const std::string_view &moduleName) -> bool {
        for (const auto &specialName : SPECIAL_MODULE_NAMES) {
            if (moduleName == specialName) {
                return true;
            }
        }
        return false;
    };

    if (prev_ == nullptr) {
        util::UString recordName(recordTable_->program_->Allocator());
        if (checkForSpecialModuleName(recordTable_->program_->ModuleName())) {
            getFileName(recordName);
            recordName.Append(recordIdent_->Name());
            return recordName.View();
        }

        recordName.Append(recordTable_->program_->ModulePrefix());
        if (recordName.Empty() && recordTable_->program_->VarBinder()->Extension() == ScriptExtension::ETS) {
            getFileName(recordName);
        }

        recordName.Append(recordIdent_->Name());
        return recordName.View();
    }

    util::UString recordName(recordTable_->program_->Allocator());
    recordName.Append(prev_->FormRecordName());
    recordName.Append(compiler::Signatures::METHOD_SEPARATOR);
    if (std::holds_alternative<ir::ClassDefinition *>(currentRecord_)) {
        const auto *classDef = std::get<ir::ClassDefinition *>(currentRecord_);
        if (classDef->IsLocal()) {
            recordName.Append(classDef->LocalPrefix());
        }
    }

    recordName.Append(recordIdent_->Name());
    return recordName.View();
}

util::StringView RecordTable::RecordName() const
{
    if (std::holds_alternative<ir::ClassDefinition *>(record_)) {
        return std::get<ir::ClassDefinition *>(record_)->InternalName();
    }

    ES2PANDA_ASSERT(std::holds_alternative<ir::TSInterfaceDeclaration *>(record_));
    return std::get<ir::TSInterfaceDeclaration *>(record_)->InternalName();
}

}  // namespace ark::es2panda::varbinder
