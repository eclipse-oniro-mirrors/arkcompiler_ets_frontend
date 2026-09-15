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

#ifndef ES2PANDA_IR_MODULE_AST_IMPORT_SPECIFIER_H
#define ES2PANDA_IR_MODULE_AST_IMPORT_SPECIFIER_H

#include "ir/statement.h"

namespace ark::es2panda::ir {
class Identifier;
class ImportSpecifierTypeOnly;

class ImportSpecifier : public Statement {
public:
    explicit ImportSpecifier(Identifier *imported, Identifier *local)
        : Statement(AstNodeType::IMPORT_SPECIFIER), imported_(imported), local_(local)
    {
    }

    Identifier *Imported()
    {
        return imported_;
    }

    const Identifier *Imported() const
    {
        return imported_;
    }

    Identifier *Local()
    {
        return local_;
    }

    const Identifier *Local() const
    {
        return local_;
    }

    bool IsRemovable() const
    {
        return isRemovable_;
    }

    void SetRemovable(bool isRemovable)
    {
        isRemovable_ = isRemovable;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

private:
    // Kept private with a friend accessor: this header is in HEADERS_TO_BE_PARSED, so every new public method adds an
    // entry in the middle of the positional es2panda_Impl table. Incremental builds then reuse libarkts objects made
    // against the old copied headers (panda_sdk_run declares directory outputs), mixing two layouts => SIGSEGV.
    friend class ImportSpecifierTypeOnly;

    Identifier *imported_;
    Identifier *local_;
    bool isRemovable_ {false};
    bool isTypeOnly_ {false};
};
}  // namespace ark::es2panda::ir

#endif
