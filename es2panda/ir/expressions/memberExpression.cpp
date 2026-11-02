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

#include "memberExpression.h"

#include <binder/binder.h>
#include <compiler/core/pandagen.h>
#include <ir/astDump.h>
#include <ir/expressions/identifier.h>
#include <ir/expressions/privateIdentifier.h>
#include <ir/expressions/literals/numberLiteral.h>
#include <ir/expressions/literals/stringLiteral.h>
#include <util/helpers.h>

namespace panda::es2panda::ir {

void MemberExpression::Iterate(const NodeTraverser &cb) const
{
    cb(object_);
    cb(property_);
}

void MemberExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "MemberExpression"},
                 {"object", object_},
                 {"property", property_},
                 {"computed", computed_},
                 {"optional", optional_}});
}

void MemberExpression::CompileObject(compiler::PandaGen *pg, compiler::VReg dest) const
{
    object_->Compile(pg);
    pg->StoreAccumulator(this, dest);
    pg->GetOptionalChain()->CheckNullish(optional_, dest);
}

compiler::Operand MemberExpression::CompileKey(compiler::PandaGen *pg) const
{
    return pg->ToPropertyKey(property_, computed_);
}

bool MemberExpression::IsThisByNameOptimizable(compiler::PandaGen *pg) const
{
    // Phase 1 folds only by-name access on the current frame `this` into ldthisbyname/
    // stthisbyname (whose receiver is GetThis(sp), implicit). Enabled from target API
    // 26.0.1 on; below that the generic ldlexvar + ldobjbyname/stobjbyname path is kept.
    // Exclude:
    //  - computed (by-value) access and optional chaining (this?.x short-circuit);
    //  - private members (independent AccessPrivateProperty opcode family);
    //  - super (super.x is a SuperExpression, not ThisExpression — naturally excluded);
    //  - derived constructors: `this` is rebound by super() and only the lexical-env slot
    //    (read by ldlexvar) is updated; GetThis(sp) (frame header thisObj) stays stale, so the
    //    optimization would read the wrong `this` there.
    if (!util::Helpers::IsSupportThisAndIteratorCloseOpt(pg->Binder()->Program()->TargetApiVersion(),
        pg->Binder()->Program()->TargetApiMinorVersion(),
        pg->Binder()->Program()->TargetApiPatchVersion())) {
        return false;
    }
    if (computed_ || optional_ || AccessPrivateProperty()) {
        return false;
    }
    if (!object_->IsThisExpression()) {
        return false;
    }
    if (pg->IsDerivedConstructor()) {
        return false;
    }
    if (pg->isDebuggerEvaluateExpressionMode()) {
        return false;
    }
    binder::ScopeFindResult res = pg->Scope()->Find(binder::Binder::MANDATORY_PARAM_THIS);
    // level == 0 ⇒ `this` is bound in the current frame, not an arrow's captured outer this.
    return res.variable != nullptr && res.level == 0;
}

void MemberExpression::Compile(compiler::PandaGen *pg) const
{
    if (IsThisByNameOptimizable(pg)) {
        compiler::Operand prop = CompileKey(pg);
        // Non-computed access ⇒ key is a string literal; emit the single ldthisbyname and skip
        // both the explicit `this` load (ldlexvar 0x0,0x0) and the objReg allocation.
        if (std::holds_alternative<util::StringView>(prop)) {
            pg->LdThisByName(this, std::get<util::StringView>(prop));
            return;
        }
        // Rare non-string non-computed key: fall through to the generic path below.
    }

    compiler::RegScope rs(pg);
    compiler::VReg objReg = pg->AllocReg();
    Compile(pg, objReg);
}

void MemberExpression::Compile(compiler::PandaGen *pg, compiler::VReg objReg) const
{
    CompileObject(pg, objReg);
    if (AccessPrivateProperty()) {
        auto name = property_->AsPrivateIdentifier()->Name();
        auto result = pg->Scope()->FindPrivateName(name);
        if (!result.result.isMethod) {
            pg->LoadAccumulator(this, objReg);
            pg->LoadPrivateProperty(this, result.lexLevel, result.result.slot);
            return;
        }
        if (result.result.isSetter) {
            pg->ThrowTypeError(this, "Property is not defined with Getter");
        }
        if (result.result.isStatic) {
            pg->LoadLexicalVar(this, result.lexLevel, result.result.validateMethodSlot);
            pg->Equal(this, objReg);
            pg->ThrowTypeErrorIfFalse(this, "Object does not have private property");
        } else {
            pg->LoadAccumulator(this, objReg);
            pg->LoadPrivateProperty(this, result.lexLevel, result.result.validateMethodSlot);
        }
        
        if (result.result.isGetter) {
            pg->LoadAccumulator(this, objReg);
            pg->LoadPrivateProperty(this, result.lexLevel, result.result.slot);
            return;
        }
        pg->LoadLexicalVar(this, result.lexLevel, result.result.slot);
        return;
    }
    compiler::Operand prop = CompileKey(pg);

    if (object_->IsSuperExpression()) {
        pg->LoadSuperProperty(property_, objReg, prop);
    } else {
        pg->LoadObjProperty(property_, objReg, prop);
    }
}

void MemberExpression::UpdateSelf(const NodeUpdater &cb, [[maybe_unused]] binder::Binder *binder)
{
    object_ = std::get<ir::AstNode *>(cb(object_))->AsExpression();
    property_ = std::get<ir::AstNode *>(cb(property_))->AsExpression();
}

}  // namespace panda::es2panda::ir
