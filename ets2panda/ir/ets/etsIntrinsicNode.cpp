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

#include "etsIntrinsicNode.h"

#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {

void ETSIntrinsicNode::Iterate(const NodeTraverser &cb) const
{
    for (auto *arg : arguments_) {
        cb(arg);
    }
}

void ETSIntrinsicNode::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSIntrinsicNode"}, {"arguments", arguments_}});
}

void ETSIntrinsicNode::Dump([[maybe_unused]] ir::SrcDumper *dumper) const
{
    dumper->Add("%%intrin%%");
    dumper->Add(Id().Mutf8());
    dumper->Add("(");
    for (auto arg : arguments_) {
        arg->Dump(dumper);
        if (arg != arguments_.back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add(")");
}

EtsIntrinsicInfo const *GetIntrinsicInfoFor(ETSIntrinsicNode const *node)
{
    return node->info_;
}

class EtsIntrinsicInfo {
public:
    static EtsIntrinsicInfo const *For(util::StringView id)
    {
        if (auto it = INFOS.find(id); it != INFOS.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    static EtsIntrinsicInfo const *For(ETSIntrinsicNode const *intrin)
    {
        return GetIntrinsicInfoFor(intrin);
    }

    virtual util::StringView Name() const = 0;

    virtual checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const = 0;

    virtual checker::Type *ExpectedTypeAt([[maybe_unused]] checker::ETSChecker *checker,
                                          [[maybe_unused]] size_t idx) const
    {
        return nullptr;
    }

    void Compile(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const
    {
        CompileImpl(etsg, intrin);
        etsg->SetAccumulatorType(intrin->TsType());
    }

    EtsIntrinsicInfo() = default;
    virtual ~EtsIntrinsicInfo() = default;
    NO_COPY_SEMANTIC(EtsIntrinsicInfo);
    NO_MOVE_SEMANTIC(EtsIntrinsicInfo);

protected:
    virtual void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const = 0;

    bool CheckParams(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const
    {
        bool hasError = false;
        for (size_t idx = 0; idx < intrin->Arguments().size(); ++idx) {
            auto &arg = intrin->Arguments()[idx];
            arg->SetPreferredType(ExpectedTypeAt(checker, idx));
            hasError |= arg->Check(checker)->IsTypeError();
        }
        return !hasError;
    }

    checker::Type *InvalidateIntrinsic(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const
    {
        checker->LogError(diagnostic::INVALID_INTRINSIC, {}, intrin->Start());
        return intrin->SetTsType(checker->GlobalTypeError());
    }

    template <uint8_t ARGS>
    std::array<ir::Expression const *, ARGS> const &Args(ETSIntrinsicNode const *intrin) const
    {
        ES2PANDA_ASSERT(intrin->Arguments().size() >= ARGS);
        return *reinterpret_cast<std::array<ir::Expression const *, ARGS> const *>(intrin->Arguments().data());
    }

private:
    using InfosMap = std::unordered_map<util::StringView, std::unique_ptr<EtsIntrinsicInfo>>;

    static InfosMap InitIntrinsicInfos();
    static const InfosMap INFOS;
};

void ETSIntrinsicNode::Compile([[maybe_unused]] compiler::PandaGen *pg) const {}

void ETSIntrinsicNode::Compile(compiler::ETSGen *etsg) const
{
    if (auto info = GetIntrinsicInfoFor(this); LIKELY(info != nullptr)) {
        return info->Compile(etsg, this);
    }
    ES2PANDA_UNREACHABLE();
}

ETSIntrinsicNode::ETSIntrinsicNode(ETSIntrinsicNode const &other, ArenaAllocator *const allocator)
    : Expression(static_cast<Expression const &>(other)), info_(other.info_), arguments_(allocator->Adapter())
{
    for (auto *const arg : other.arguments_) {
        arguments_.emplace_back(arg->Clone(allocator, this)->AsExpression());
    }
}

ETSIntrinsicNode::ETSIntrinsicNode(util::StringView id, ArenaVector<ir::Expression *> &&arguments)
    : Expression(AstNodeType::ETS_INTRINSIC_NODE_TYPE),
      info_(EtsIntrinsicInfo::For(id)),
      arguments_(std::move(arguments))
{
}

void ETSIntrinsicNode::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&args : arguments_) {
        if (auto *transformedNode = cb(args); args != transformedNode) {
            args->SetTransformedNode(transformationName, transformedNode);
            args = static_cast<TypeNode *>(transformedNode);
        }
    }
}

util::StringView ETSIntrinsicNode::Id() const
{
    return info_ == nullptr ? "invalid" : info_->Name();
}

checker::Type *ETSIntrinsicNode::ExpectedTypeAt(checker::ETSChecker *checker, size_t idx) const
{
    if (auto info = GetIntrinsicInfoFor(this); LIKELY(info != nullptr)) {
        return info->ExpectedTypeAt(checker, idx);
    }
    return nullptr;
}

checker::Type *ETSIntrinsicNode::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType ETSIntrinsicNode::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    if (auto info = GetIntrinsicInfoFor(this); LIKELY(info != nullptr)) {
        return {this, info->Check(checker, this)};
    }
    checker->LogError(diagnostic::INVALID_INTRINSIC, {}, Start());
    return {this, SetTsType(checker->GlobalTypeError())};
}

ETSIntrinsicNode *ETSIntrinsicNode::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ETSIntrinsicNode *clone = allocator->New<ir::ETSIntrinsicNode>(*this, allocator);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}

class ETSIntrinsicTypeReference final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "typereference";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 1 ||
            (!intrin->Arguments()[0]->IsStringLiteral() && !intrin->Arguments()[0]->IsTypeNode())) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalBuiltinClassType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        if (intrin->Arguments()[0]->IsStringLiteral()) {
            etsg->EmitLdaType(intrin, intrin->Arguments()[0]->AsStringLiteral()->Str());
            return;
        }
        etsg->EmitLdaType(intrin, intrin->Arguments()[0]->TsType()->ToAssemblerTypeWithRankView(etsg->Allocator()));
    }
};

class ETSIntrinsicAnyLdByVal final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anyldbyval";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 2U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSAnyType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, prop] = Args<2U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();
        auto const propReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);

        prop->Compile(etsg);
        etsg->StoreAccumulator(intrin, propReg);

        etsg->EmitAnyLdbyval(intrin, objReg, propReg);
    }
};

class ETSIntrinsicAnyStByVal final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anystbyval";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 3U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(intrin->Arguments()[2U]->TsType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, prop, propVal] = Args<3U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();
        auto const propReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);

        prop->Compile(etsg);
        etsg->StoreAccumulator(intrin, propReg);

        propVal->Compile(etsg);

        etsg->EmitAnyStbyval(intrin, objReg, propReg);
    }
};

class ETSIntrinsicAnyLdByIdx final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anyldbyidx";
    }

    checker::Type *ExpectedTypeAt(checker::ETSChecker *checker, [[maybe_unused]] size_t idx) const override
    {
        if (idx == 1U) {
            return checker->GlobalDoubleType();
        }
        return nullptr;
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 2U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSAnyType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, prop] = Args<2U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();
        auto const propReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);

        prop->Compile(etsg);
        etsg->StoreAccumulator(intrin, propReg);

        etsg->EmitAnyLdbyidx(intrin, objReg, propReg);
    }
};

class ETSIntrinsicAnyStByIdx final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anystbyidx";
    }

    checker::Type *ExpectedTypeAt(checker::ETSChecker *checker, [[maybe_unused]] size_t idx) const override
    {
        if (idx == 1U) {
            return checker->GlobalDoubleType();
        }
        return nullptr;
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 3U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(intrin->Arguments()[2U]->TsType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, prop, propVal] = Args<3U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();
        auto const propReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);

        prop->Compile(etsg);
        etsg->StoreAccumulator(intrin, propReg);

        propVal->Compile(etsg);

        etsg->EmitAnyStbyidx(intrin, objReg, propReg);
    }
};

class ETSIntrinsicAnyLdByName final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anyldbyname";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 2U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        if (!intrin->Arguments()[1]->IsStringLiteral()) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSAnyType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, prop] = Args<2U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);
        etsg->EmitAnyLdbyname(intrin, objReg, prop->AsStringLiteral()->Str());
    }
};

class ETSIntrinsicAnyStByName final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anystbyname";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 3U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        if (!intrin->Arguments()[1]->IsStringLiteral()) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(intrin->Arguments()[2U]->TsType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, prop, propVal] = Args<3U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);

        propVal->Compile(etsg);

        etsg->EmitAnyStbyname(intrin, objReg, prop->AsStringLiteral()->Str());
    }
};

class ETSIntrinsicAnyCall final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anycall";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().empty()) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSAnyType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [callee] = Args<1U>(intrin);
        auto args =
            Span<ir::Expression const *const> {intrin->Arguments().data(), intrin->Arguments().size()}.SubSpan(1);

        compiler::RegScope rs(etsg);
        auto const calleeReg = etsg->AllocReg();

        callee->Compile(etsg);
        etsg->StoreAccumulator(intrin, calleeReg);

        etsg->CallAny(intrin, args, calleeReg);
    }
};

class ETSIntrinsicAnyCallNew final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anycallnew";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().empty()) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSAnyType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [callee] = Args<1U>(intrin);
        auto args =
            Span<ir::Expression const *const> {intrin->Arguments().data(), intrin->Arguments().size()}.SubSpan(1);

        compiler::RegScope rs(etsg);
        auto const calleeReg = etsg->AllocReg();

        callee->Compile(etsg);
        etsg->StoreAccumulator(intrin, calleeReg);

        etsg->CallAnyNew(intrin, args, calleeReg);
    }
};

class ETSIntrinsicAnyCallThis final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anycallthis";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() < 2U || !intrin->Arguments()[1]->IsStringLiteral()) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSAnyType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [callee, prop] = Args<2U>(intrin);
        auto args =
            Span<ir::Expression const *const> {intrin->Arguments().data(), intrin->Arguments().size()}.SubSpan(2);

        compiler::RegScope rs(etsg);
        auto const calleeReg = etsg->AllocReg();

        callee->Compile(etsg);
        etsg->StoreAccumulator(intrin, calleeReg);

        etsg->CallAnyThis(intrin, prop->AsStringLiteral()->Str(), args, calleeReg);
    }
};

class ETSIntrinsicAnyIsinstance final : public EtsIntrinsicInfo {
public:
    util::StringView Name() const override
    {
        return "anyisinstance";
    }

    checker::Type *Check(checker::ETSChecker *checker, ETSIntrinsicNode *intrin) const override
    {
        CheckParams(checker, intrin);
        if (intrin->Arguments().size() != 2U) {
            return InvalidateIntrinsic(checker, intrin);
        }
        return intrin->SetTsType(checker->GlobalETSBooleanType());
    }

    void CompileImpl(compiler::ETSGen *etsg, ETSIntrinsicNode const *intrin) const override
    {
        auto const [obj, type] = Args<2U>(intrin);

        compiler::RegScope rs(etsg);
        auto const objReg = etsg->AllocReg();
        auto const typeReg = etsg->AllocReg();

        obj->Compile(etsg);
        etsg->StoreAccumulator(intrin, objReg);

        type->Compile(etsg);
        etsg->StoreAccumulator(intrin, typeReg);

        etsg->LoadAccumulator(intrin, objReg);
        etsg->EmitAnyIsinstance(intrin, typeReg);
    }
};
// NOLINTNEXTLINE (cert-err58-cpp)
const EtsIntrinsicInfo::InfosMap EtsIntrinsicInfo::INFOS = EtsIntrinsicInfo::InitIntrinsicInfos();

EtsIntrinsicInfo::InfosMap EtsIntrinsicInfo::InitIntrinsicInfos()
{
    EtsIntrinsicInfo::InfosMap infos;
    auto const registerIntrin = [&infos](std::unique_ptr<EtsIntrinsicInfo> &&data) {
        auto name = data->Name();
        [[maybe_unused]] auto res = infos.emplace(name, std::move(data));
        ES2PANDA_ASSERT(res.second);
    };

    registerIntrin(std::make_unique<ETSIntrinsicTypeReference>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyLdByVal>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyStByVal>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyLdByIdx>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyStByIdx>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyLdByName>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyStByName>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyCall>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyCallNew>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyCallThis>());
    registerIntrin(std::make_unique<ETSIntrinsicAnyIsinstance>());
    return infos;
}

}  // namespace ark::es2panda::ir
