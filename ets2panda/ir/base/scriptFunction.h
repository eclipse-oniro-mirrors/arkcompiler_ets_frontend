/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_PARSER_INCLUDE_AST_SCRIPT_FUNCTION_H
#define ES2PANDA_PARSER_INCLUDE_AST_SCRIPT_FUNCTION_H

#include "checker/types/signature.h"
#include "ir/annotationAllowed.h"
#include "ir/statements/annotationUsage.h"
#include "ir/statements/returnStatement.h"
#include "ir/astNode.h"
#include "scriptFunctionSignature.h"
#include "util/enumbitops.h"
#include "util/language.h"
#include "varbinder/scope.h"

namespace ark::es2panda::checker {
class Signature;

}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class TSTypeParameterDeclaration;
class TypeNode;
class AnnotationUsage;
class FunctionSignature;

class ScriptFunction : public AnnotationAllowed<AstNode> {
public:
    // Need to reduce the number of constructor parameters to pass OHOS CI code check
    struct ScriptFunctionData {
        AstNode *body = nullptr;
        FunctionSignature &&signature;
        ir::ScriptFunctionFlags funcFlags = ir::ScriptFunctionFlags::NONE;
        ir::ModifierFlags flags = ir::ModifierFlags::NONE;
        ark::es2panda::Language lang {Language::Id::ETS};
    };

    ScriptFunction() = delete;
    ~ScriptFunction() override = default;

    NO_COPY_SEMANTIC(ScriptFunction);
    NO_MOVE_SEMANTIC(ScriptFunction);

    explicit ScriptFunction(ArenaAllocator *allocator, ScriptFunctionData &&data);

    explicit ScriptFunction(ArenaAllocator *allocator, ScriptFunctionData &&data, AstNodeHistory *history);

    [[nodiscard]] const Identifier *Id() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->id_;
    }

    [[nodiscard]] Identifier *Id() noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->id_;
    }

    [[nodiscard]] const checker::Signature *Signature() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->signature_;
    }

    [[nodiscard]] checker::Signature *Signature() noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->signature_;
    }

    [[nodiscard]] const ArenaVector<ir::Expression *> &Params() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->irSignature_.Params();
    }

    [[nodiscard]] const ArenaVector<ir::Expression *> &Params();

    const ArenaVector<ReturnStatement *> &ReturnStatements() const
    {
        return GetHistoryNodeAs<ScriptFunction>()->returnStatements_;
    }

    [[nodiscard]] const ArenaVector<ReturnStatement *> &ReturnStatements();
    [[nodiscard]] ArenaVector<ReturnStatement *> &ReturnStatementsForUpdate();

    [[nodiscard]] const TSTypeParameterDeclaration *TypeParams() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->irSignature_.TypeParams();
    }

    [[nodiscard]] TSTypeParameterDeclaration *TypeParams() noexcept
    {
        return GetHistoryNode()->AsScriptFunction()->irSignature_.TypeParams();
    }

    [[nodiscard]] const AstNode *Body() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->body_;
    }

    [[nodiscard]] AstNode *Body() noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->body_;
    }

    void AddReturnStatement(ReturnStatement *returnStatement)
    {
        EmplaceReturnStatements(returnStatement);
    }

    void SetBody(AstNode *body);

    [[nodiscard]] const TypeNode *ReturnTypeAnnotation() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->irSignature_.ReturnType();
    }

    [[nodiscard]] TypeNode *ReturnTypeAnnotation() noexcept
    {
        return GetHistoryNode()->AsScriptFunction()->irSignature_.ReturnType();
    }

    void SetReturnTypeAnnotation(TypeNode *node) noexcept;

    [[nodiscard]] bool IsEntryPoint() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::ENTRY_POINT) != 0;
    }

    [[nodiscard]] bool IsGenerator() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::GENERATOR) != 0;
    }

    [[nodiscard]] bool IsAsyncFunc() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::ASYNC) != 0;
    }

    [[nodiscard]] bool IsAsyncImplFunc() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::ASYNC_IMPL) != 0;
    }

    [[nodiscard]] bool IsArrow() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::ARROW) != 0;
    }

    [[nodiscard]] bool IsOverload() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::OVERLOAD) != 0;
    }

    [[nodiscard]] bool IsExternalOverload() const
    {
        return (Flags() & ir::ScriptFunctionFlags::EXTERNAL_OVERLOAD) != 0;
    }

    [[nodiscard]] bool IsConstructor() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::CONSTRUCTOR) != 0;
    }

    [[nodiscard]] bool IsGetter() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::GETTER) != 0;
    }

    [[nodiscard]] bool IsSetter() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::SETTER) != 0;
    }

    [[nodiscard]] bool IsExtensionAccessor() const noexcept
    {
        return IsExtensionMethod() && (IsGetter() || IsSetter());
    }

    [[nodiscard]] bool IsMethod() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::METHOD) != 0;
    }

    [[nodiscard]] bool IsProxy() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::PROXY) != 0;
    }

    [[nodiscard]] bool IsStaticBlock() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::STATIC_BLOCK) != 0;
    }

    [[nodiscard]] bool IsEnum() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::ENUM) != 0;
    }

    [[nodiscard]] bool IsHidden() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::HIDDEN) != 0;
    }

    [[nodiscard]] bool IsExternal() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::EXTERNAL) != 0;
    }

    [[nodiscard]] bool IsImplicitSuperCallNeeded() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::IMPLICIT_SUPER_CALL_NEEDED) != 0;
    }

    [[nodiscard]] bool HasBody() const noexcept
    {
        return Body() != nullptr;
    }

    [[nodiscard]] bool HasRestParameter() const noexcept
    {
        return Signature()->RestVar() != nullptr;
    }

    [[nodiscard]] bool HasReturnStatement() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::HAS_RETURN) != 0;
    }

    [[nodiscard]] bool HasThrowStatement() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::HAS_THROW) != 0;
    }

    [[nodiscard]] bool IsTrailingLambda() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::TRAILING_LAMBDA) != 0;
    }

    [[nodiscard]] bool IsSynthetic() const noexcept
    {
        return (Flags() & ir::ScriptFunctionFlags::SYNTHETIC) != 0;
    }

    [[nodiscard]] bool IsDynamic() const noexcept
    {
        return Language().IsDynamic();
    }

    // Note: This method has been written into CAPI, cannot remove it simply.
    [[nodiscard]] bool IsExtensionMethod() const noexcept
    {
        return HasReceiver();
    }

    [[nodiscard]] ir::ScriptFunctionFlags Flags() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->funcFlags_;
    }

    [[nodiscard]] bool HasReceiver() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->irSignature_.HasReceiver();
    }

    void SetIdent(Identifier *id) noexcept;

    void SetSignature(checker::Signature *signature);

    void AddFlag(ir::ScriptFunctionFlags flags) noexcept
    {
        if (!All(Flags(), flags)) {
            GetOrCreateHistoryNode()->AsScriptFunction()->funcFlags_ |= flags;
        }
    }

    void ClearFlag(ir::ScriptFunctionFlags flags) noexcept
    {
        if (Any(Flags(), flags)) {
            GetOrCreateHistoryNode()->AsScriptFunction()->funcFlags_ &= ~flags;
        }
    }

    [[nodiscard]] std::size_t FormalParamsLength() const noexcept;

    [[nodiscard]] bool IsScopeBearer() const noexcept override
    {
        return true;
    }

    [[nodiscard]] varbinder::FunctionScope *Scope() const noexcept override
    {
        return GetHistoryNodeAs<ScriptFunction>()->scope_;
    }

    void SetScope(varbinder::FunctionScope *scope);

    void ClearScope() noexcept override
    {
        SetScope(nullptr);
    }

    [[nodiscard]] es2panda::Language Language() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->lang_;
    }

    void SetPreferredReturnType(checker::Type *preferredReturnType);

    [[nodiscard]] checker::Type *GetPreferredReturnType() noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->preferredReturnType_;
    }

    [[nodiscard]] checker::Type const *GetPreferredReturnType() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->preferredReturnType_;
    }

    void SetAsyncPairMethod(ScriptFunction *asyncPairFunction)
    {
        this->GetOrCreateHistoryNodeAs<ScriptFunction>()->asyncPairFunction_ = asyncPairFunction;
    }

    [[nodiscard]] const ScriptFunction *AsyncPairMethod() const noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->asyncPairFunction_;
    }

    [[nodiscard]] ScriptFunction *AsyncPairMethod() noexcept
    {
        return GetHistoryNodeAs<ScriptFunction>()->asyncPairFunction_;
    }

    [[nodiscard]] ScriptFunction *Clone(ArenaAllocator *allocator, AstNode *parent) override;

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

    void CleanUp() override
    {
        AstNode::CleanUp();
        SetSignature(nullptr);
        SetPreferredReturnType(nullptr);
    }
    void EmplaceReturnStatements(ReturnStatement *returnStatements);
    void ClearReturnStatements();
    void SetValueReturnStatements(ReturnStatement *returnStatements, size_t index);

    void EmplaceParams(Expression *params);
    void SetParams(ArenaVector<Expression *> &&paramsList);
    void ClearParams();
    void SetValueParams(Expression *params, size_t index);
    ArenaVector<Expression *> &ParamsForUpdate();

protected:
    ScriptFunction *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    void DumpBody(ir::SrcDumper *dumper) const;
    void DumpCheckerTypeForDeclGen(ir::SrcDumper *dumper) const;
    friend class SizeOfNodeTest;
    Identifier *id_ {};
    FunctionSignature irSignature_;
    AstNode *body_;
    varbinder::FunctionScope *scope_ {nullptr};
    ir::ScriptFunctionFlags funcFlags_;
    checker::Signature *signature_ {};
    checker::Type *preferredReturnType_ {};
    es2panda::Language lang_;
    ArenaVector<ReturnStatement *> returnStatements_;
    ScriptFunction *asyncPairFunction_;
};
}  // namespace ark::es2panda::ir

#endif
