/**
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_IR_TS_IMPORT_EQUALS_DECLARATION_H
#define ES2PANDA_IR_TS_IMPORT_EQUALS_DECLARATION_H

#include "ir/statement.h"

namespace ark::es2panda::ir {
class Expression;

class TSImportEqualsDeclaration : public Statement {
public:
    explicit TSImportEqualsDeclaration(Identifier *id, Expression *moduleReference, bool isExport)
        : Statement(AstNodeType::TS_IMPORT_EQUALS_DECLARATION),
          id_(id),
          moduleReference_(moduleReference),
          isExport_(isExport)
    {
    }

    const Identifier *Id() const
    {
        return id_;
    }

    const Expression *ModuleReference() const
    {
        return moduleReference_;
    }

    bool IsExport() const
    {
        return isExport_;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

private:
    Identifier *id_;
    Expression *moduleReference_;
    bool isExport_;
};
}  // namespace ark::es2panda::ir

#endif
