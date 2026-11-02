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

#include "iterators.h"

#include <compiler/core/pandagen.h>
#include <compiler/base/catchTable.h>
#include <compiler/core/compilerContext.h>
#include <compiler/function/functionBuilder.h>
#include <util/helpers.h>

namespace panda::es2panda::compiler {

// Iterator

Iterator::Iterator(PandaGen *pg, const ir::AstNode *node, IteratorType type)
    : pg_(pg), node_(node), closed_(pg->AllocReg()), method_(pg->AllocReg()), iterator_(pg->AllocReg()),
      nextResult_(pg->AllocReg()), type_(type)
{
    if (type_ == IteratorType::ASYNC) {
        pg_->GetAsyncIterator(node);
    } else {
        pg_->GetIterator(node);
    }

    pg_->StoreAccumulator(node, iterator_);
    pg_->LoadObjByName(node_, iterator_, "next");
    pg_->StoreAccumulator(node_, method_);
    pg_->StoreConst(node_, closed_, Constant::JS_FALSE);
}

void Iterator::GetMethod(util::StringView name) const
{
    pg_->LoadObjByName(node_, iterator_, name);
    pg_->StoreAccumulator(node_, method_);
}

void Iterator::CallMethodWithValue() const
{
    pg_->CallThis(node_, method_, 2);
}

void Iterator::CallMethod() const
{
    pg_->CallThis(node_, method_, 1);
}

void Iterator::Next() const
{
    CallMethod();

    if (type_ == IteratorType::ASYNC) {
        pg_->FuncBuilder()->Await(node_);
    }

    pg_->StoreAccumulator(node_, nextResult_);
    pg_->ThrowIfNotObject(node_, nextResult_);
}

void Iterator::Complete() const
{
    pg_->LoadObjByName(node_, nextResult_, "done");
}

void Iterator::Value() const
{
    pg_->LoadObjByName(node_, nextResult_, "value");
}

bool Iterator::UseCloseSyncOpcode(bool abruptCompletion) const
{
    // Only the abrupt (exception) close path folds into the closeiterator opcode:
    // the original-exception-wins rule (7.4.6 step 6) is enforced by the
    // bytecode-level handler that discards the opcode outcome and rethrows, so
    // this path is observably identical to the inlined sequence regardless of
    // whether the runtime executes accessor 'return' getters per GetV. The
    // non-abrupt (break/continue/return) close stays inlined: its GetMethod runs
    // via ldobjbyname and honors full GetV semantics there. Enabled from target
    // API 26.1.0 on; below that - and for ASYNC iterators - the inlined
    // Iterator::Close sequence is kept.
    return abruptCompletion && type_ == IteratorType::SYNC &&
           util::Helpers::IsSupportThisAndIteratorCloseOpt(pg_->Binder()->Program()->TargetApiVersion(),
               pg_->Binder()->Program()->TargetApiMinorVersion(),
               pg_->Binder()->Program()->TargetApiPatchVersion());
}

void Iterator::Close(bool abruptCompletion) const
{
    if (UseCloseSyncOpcode(abruptCompletion)) {
        CloseSyncOpcode(abruptCompletion);
        return;
    }

    RegScope rs(pg_);
    VReg completion = pg_->AllocReg();
    VReg innerResult = pg_->AllocReg();
    VReg innerException = pg_->AllocReg();
    Label *noReturn = pg_->AllocLabel();

    pg_->StoreAccumulator(node_, completion);
    pg_->LoadAccumulator(node_, closed_);
    pg_->BranchIfTrue(node_, noReturn);

    pg_->StoreConst(node_, closed_, Constant::JS_TRUE);
    pg_->StoreConst(node_, innerResult, Constant::JS_UNDEFINED);
    pg_->StoreConst(node_, innerException, Constant::JS_HOLE);

    TryContext tryCtx(pg_);
    const auto &labelSet = tryCtx.LabelSet();

    pg_->SetLabel(node_, labelSet.TryBegin());

    // 4. Let innerResult be GetMethod(iterator, "return").
    GetMethod("return");

    // 5. If innerResult.[[Type]] is normal, then
    {
        // a. Let return be innerResult.[[Value]].
        // b. If return is undefined, return Completion(completion).
        pg_->BranchIfUndefined(node_, noReturn);
        // c. Set innerResult to Call(return, iterator).
        CallMethod();
        if (type_ == IteratorType::ASYNC) {
            // d. If innerResult.[[Type]] is normal, set innerResult to Await(innerResult.[[Value]]).
            pg_->FuncBuilder()->Await(node_);
        }
        pg_->StoreAccumulator(node_, innerResult);
    }

    pg_->SetLabel(node_, labelSet.TryEnd());
    pg_->Branch(node_, labelSet.CatchEnd());

    pg_->SetLabel(node_, labelSet.CatchBegin());
    pg_->StoreAccumulator(node_, innerException);
    pg_->SetLabel(node_, labelSet.CatchEnd());

    // 6. If completion.[[Type]] is throw, return Completion(completion).
    if (abruptCompletion) {
        pg_->LoadAccumulator(node_, completion);
        pg_->EmitThrow(node_);
    } else {
        // 7. If innerResult.[[Type]] is throw, return Completion(innerResult).
        pg_->LoadAccumulator(node_, innerException);
        pg_->EmitRethrow(node_);
    }

    // 8. If Type(innerResult.[[Value]]) is not Object, throw a TypeError exception.
    pg_->LoadAccumulator(node_, innerResult);
    pg_->ThrowIfNotObject(node_, innerResult);

    pg_->SetLabel(node_, noReturn);
    pg_->LoadAccumulator(node_, completion);
    if (abruptCompletion) {
        pg_->EmitThrow(node_);
    }
}

// ES 7.4.6 IteratorClose via the dedicated closeiterator opcode.
// acc is preserved across the sequence: on the abort path (break/continue/
// return) it carries the value of an enclosing 'return expr', on the abrupt
// path the pending exception, which the EH dispatch has already moved into acc
// and cleared from the thread.
void Iterator::CloseSyncOpcode(bool abruptCompletion) const
{
    RegScope rs(pg_);
    VReg completion = pg_->AllocReg();
    Label *skip = pg_->AllocLabel();

    pg_->StoreAccumulator(node_, completion);
    pg_->LoadAccumulator(node_, closed_);
    pg_->BranchIfTrue(node_, skip);

    pg_->StoreConst(node_, closed_, Constant::JS_TRUE);

    if (abruptCompletion) {
        // A throw completion wins over any error of the close attempt itself
        // (7.4.6: the completion check precedes the innerResult check), so
        // closeiterator runs under a handler that discards its outcome and
        // rethrows the original exception. Every path (close ok, close
        // failed, already closed) converges on the rethrow.
        //
        // The try range must end on a branch and the handler must contain a
        // real instruction: bytecodeopt's IrBuilder suppresses fall-through
        // edges into catch entries, so a try block whose last instruction
        // falls straight into its own handler ends up without successors and
        // crashes ConnectTryCodeBlock.
        TryContext tryCtx(pg_);
        const auto &labelSet = tryCtx.LabelSet();

        pg_->SetLabel(node_, labelSet.TryBegin());
        pg_->CloseIterator(node_, iterator_);
        pg_->SetLabel(node_, labelSet.TryEnd());
        pg_->Branch(node_, skip);

        pg_->SetLabel(node_, labelSet.CatchBegin());
        pg_->Branch(node_, skip);
        pg_->SetLabel(node_, labelSet.CatchEnd());

        pg_->SetLabel(node_, skip);
        pg_->LoadAccumulator(node_, completion);
        pg_->EmitThrow(node_);
        return;
    }

    // A failing close attempt propagates its own error (7.4.6 step 6).
    pg_->CloseIterator(node_, iterator_);
    pg_->SetLabel(node_, skip);
    pg_->LoadAccumulator(node_, completion);
}

DestructuringIterator::DestructuringIterator(PandaGen *pg, const ir::AstNode *node)
    : Iterator(pg, node, IteratorType::SYNC), done_(pg->AllocReg()), result_(pg->AllocReg())
{
    pg_->StoreConst(node, done_, Constant::JS_FALSE);
    pg_->StoreConst(node, result_, Constant::JS_UNDEFINED);
}

void DestructuringIterator::Step(Label *doneTarget) const
{
    TryContext tryCtx(pg_);
    const auto &labelSet = tryCtx.LabelSet();
    Label *normalClose = pg_->AllocLabel();
    Label *noClose = pg_->AllocLabel();

    pg_->SetLabel(node_, labelSet.TryBegin());
    JumpIfDone(noClose);
    Next();
    Complete();
    pg_->StoreAccumulator(node_, done_);
    pg_->BranchIfFalse(node_, normalClose);
    pg_->StoreConst(node_, done_, Constant::JS_TRUE);
    pg_->LoadConst(node_, Constant::JS_UNDEFINED);
    pg_->Branch(node_, noClose);

    pg_->SetLabel(node_, normalClose);
    Value();
    pg_->StoreAccumulator(node_, result_);
    pg_->Branch(node_, labelSet.CatchEnd());
    pg_->SetLabel(node_, noClose);
    OnIterDone(doneTarget);

    pg_->SetLabel(node_, labelSet.TryEnd());
    pg_->Branch(node_, labelSet.CatchEnd());

    pg_->SetLabel(node_, labelSet.CatchBegin());
    pg_->StoreAccumulator(node_, result_);
    pg_->StoreConst(node_, done_, Constant::JS_TRUE);
    pg_->LoadAccumulator(node_, result_);
    pg_->EmitThrow(node_);
    pg_->SetLabel(node_, labelSet.CatchEnd());
}

void DestructuringIterator::OnIterDone([[maybe_unused]] Label *doneTarget) const
{
    pg_->LoadConst(node_, Constant::JS_UNDEFINED);
}

void DestructuringRestIterator::OnIterDone([[maybe_unused]] Label *doneTarget) const
{
    pg_->Branch(node_, doneTarget);
}

void DestructuringIterator::JumpIfDone(Label *noClose) const
{
    pg_->LoadAccumulator(node_, done_);
    pg_->BranchIfTrue(node_, noClose);
}

}  // namespace panda::es2panda::compiler
