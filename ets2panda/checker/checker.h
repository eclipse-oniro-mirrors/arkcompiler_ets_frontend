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

#ifndef ES2PANDA_CHECKER_CHECKER_H
#define ES2PANDA_CHECKER_CHECKER_H

#include "checker/checkerContext.h"
#include "checker/SemanticAnalyzer.h"
#include "checker/types/globalTypesHolder.h"
#include "util/diagnosticEngine.h"

namespace ark::es2panda::util {
class Options;
}  // namespace ark::es2panda::util

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::ir {
class AstNode;
class Expression;
class BlockStatement;
enum class AstNodeType : uint8_t;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
class VarBinder;
class Decl;
class EnumVariable;
class FunctionDecl;
class LocalVariable;
class Scope;
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::checker {
class ETSChecker;
class InterfaceType;
class GlobalTypesHolder;
class SemanticAnalyzer;

using StringLiteralPool = std::unordered_map<util::StringView, Type *>;
using NumberLiteralPool = std::unordered_map<double, Type *>;
using FunctionParamsResolveResult = std::variant<std::vector<varbinder::LocalVariable *> &, bool>;
using InterfacePropertyMap =
    std::unordered_map<util::StringView, std::pair<varbinder::LocalVariable *, InterfaceType *>>;
using TypeOrNode = std::variant<Type *, ir::AstNode *>;
using IndexInfoTypePair = std::pair<Type *, Type *>;
using PropertyMap = std::unordered_map<util::StringView, varbinder::LocalVariable *>;
using ArgRange = std::pair<uint32_t, uint32_t>;

class Checker {
public:
    explicit Checker(ArenaAllocator *allocator, util::DiagnosticEngine &diagnosticEngine);
    virtual ~Checker() = default;

    NO_COPY_SEMANTIC(Checker);
    NO_MOVE_SEMANTIC(Checker);

    [[nodiscard]] ArenaAllocator *Allocator() noexcept
    {
        return allocator_;
    }

    [[nodiscard]] varbinder::Scope *Scope() const noexcept
    {
        return scope_;
    }

    [[nodiscard]] CheckerContext &Context() noexcept
    {
        return context_;
    }

    [[nodiscard]] bool HasStatus(CheckerStatus status) noexcept
    {
        return (context_.Status() & status) != 0;
    }

    void RemoveStatus(CheckerStatus status) noexcept
    {
        context_.Status() &= ~status;
    }

    void AddStatus(CheckerStatus status) noexcept
    {
        context_.Status() |= status;
    }

    [[nodiscard]] TypeRelation *Relation() const noexcept
    {
        return relation_;
    }

    void InitGlobalTypes()
    {
        globalTypes_ = ProgramAllocator()->New<GlobalTypesHolder>(ProgramAllocator());
    }

    [[nodiscard]] GlobalTypesHolder *GetGlobalTypesHolder() const noexcept
    {
        return globalTypes_;
    }

    void SetGlobalTypesHolder(GlobalTypesHolder *globalTypes)
    {
        globalTypes_ = globalTypes;
    }

    [[nodiscard]] RelationHolder &IdenticalResults() noexcept
    {
        return identicalResults_;
    }

    [[nodiscard]] RelationHolder &AssignableResults() noexcept
    {
        return assignableResults_;
    }

    [[nodiscard]] RelationHolder &ComparableResults() noexcept
    {
        return comparableResults_;
    }

    [[nodiscard]] RelationHolder &UncheckedCastableResult() noexcept
    {
        return uncheckedCastableResults_;
    }

    [[nodiscard]] RelationHolder &SupertypeResults() noexcept
    {
        return supertypeResults_;
    }

    [[nodiscard]] std::unordered_map<const void *, Type *> &TypeStack() noexcept
    {
        return typeStack_;
    }

    [[nodiscard]] std::unordered_set<Type *> &NamedTypeStack() noexcept
    {
        return namedTypeStack_;
    }

    [[nodiscard]] virtual bool IsETSChecker() const noexcept
    {
        return false;
    }

    [[nodiscard]] ETSChecker *AsETSChecker()
    {
        ES2PANDA_ASSERT(IsETSChecker());
        return reinterpret_cast<ETSChecker *>(this);
    }

    [[nodiscard]] const ETSChecker *AsETSChecker() const
    {
        ES2PANDA_ASSERT(IsETSChecker());
        return reinterpret_cast<const ETSChecker *>(this);
    }

    virtual bool StartChecker([[maybe_unused]] varbinder::VarBinder *varbinder, const util::Options &options) = 0;
    virtual Type *CheckTypeCached(ir::Expression *expr) = 0;
    virtual Type *GetTypeOfVariable(varbinder::Variable *var) = 0;
    virtual void ResolveStructuredTypeMembers(Type *type) = 0;

    void LogError(const diagnostic::DiagnosticKind &diagnostic,
                  const util::DiagnosticMessageParams &diagnosticParams = {});
    void LogError(const diagnostic::DiagnosticKind &diagnostic, const util::DiagnosticMessageParams &diagnosticParams,
                  const lexer::SourcePosition &pos);
    void LogError(const diagnostic::DiagnosticKind &diagnostic, const lexer::SourcePosition &pos);
    void LogTypeError(std::string_view message, const lexer::SourcePosition &pos);
    void LogTypeError(const util::DiagnosticMessageParams &list, const lexer::SourcePosition &pos);
    void LogDiagnostic(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &list,
                       const lexer::SourcePosition &pos);
    void LogDiagnostic(const diagnostic::DiagnosticKind &kind, const lexer::SourcePosition &pos)
    {
        LogDiagnostic(kind, {}, pos);
    }

    bool IsTypeIdenticalTo(Type *source, Type *target);
    bool IsTypeIdenticalTo(Type *source, Type *target, const diagnostic::DiagnosticKind &diagKind,
                           const util::DiagnosticMessageParams &diagParams, const lexer::SourcePosition &errPos);
    bool IsTypeIdenticalTo(Type *source, Type *target, const diagnostic::DiagnosticKind &diagKind,
                           const lexer::SourcePosition &errPos);
    bool IsTypeAssignableTo(Type *source, Type *target);
    bool IsTypeAssignableTo(Type *source, Type *target, const diagnostic::DiagnosticKind &diagKind,
                            const util::DiagnosticMessageParams &list, const lexer::SourcePosition &errPos);
    bool IsTypeComparableTo(Type *source, Type *target);
    bool IsTypeComparableTo(Type *source, Type *target, const diagnostic::DiagnosticKind &diagKind,
                            const util::DiagnosticMessageParams &list, const lexer::SourcePosition &errPos);
    bool AreTypesComparable(Type *source, Type *target);
    bool IsTypeEqualityComparableTo(Type *source, Type *target);
    bool IsAllTypesAssignableTo(Type *source, Type *target);
    void SetAnalyzer(SemanticAnalyzer *analyzer);
    checker::SemanticAnalyzer *GetAnalyzer() const;

    friend class ScopeContext;
    friend class TypeStackElement;
    friend class NamedTypeStackElement;
    friend class SavedCheckerContext;
    friend class NamedTypeStackElement;

    varbinder::VarBinder *VarBinder() const;

    util::DiagnosticEngine &DiagnosticEngine()
    {
        return diagnosticEngine_;
    }

    lexer::SourcePosition GetPositionForDiagnostic() const
    {
        return lexer::SourcePosition {Program()};
    }

    // NOTE: required only for evaluate.
    void Initialize(varbinder::VarBinder *varbinder);

    [[nodiscard]] bool IsAnyError();

    virtual void CleanUp();

    // legacy API, use Allocator() instead
    [[nodiscard]] ArenaAllocator *ProgramAllocator()
    {
        return allocator_;
    }

    bool IsDeclForDynamicStaticInterop() const;

protected:
    parser::Program *Program() const;
    void SetProgram(parser::Program *program);

private:
    ArenaAllocator *allocator_;
    CheckerContext context_;
    GlobalTypesHolder *globalTypes_ {nullptr};
    TypeRelation *relation_;
    SemanticAnalyzer *analyzer_ {};
    varbinder::VarBinder *varbinder_ {};
    parser::Program *program_ {};
    varbinder::Scope *scope_ {};
    util::DiagnosticEngine &diagnosticEngine_;

    RelationHolder identicalResults_ {Allocator()};
    RelationHolder assignableResults_ {Allocator()};
    RelationHolder comparableResults_ {Allocator()};
    RelationHolder uncheckedCastableResults_ {Allocator()};
    RelationHolder supertypeResults_ {Allocator()};

    std::unordered_map<const void *, Type *> typeStack_;
    std::unordered_set<Type *> namedTypeStack_;
};

class NamedTypeStackElement {
public:
    explicit NamedTypeStackElement(Checker *checker, Type *element) : checker_(checker), element_(element)
    {
        checker_->namedTypeStack_.insert(element);
    }

    ~NamedTypeStackElement()
    {
        checker_->namedTypeStack_.erase(element_);
    }
    NO_COPY_SEMANTIC(NamedTypeStackElement);
    NO_MOVE_SEMANTIC(NamedTypeStackElement);

private:
    Checker *checker_;
    Type *element_;
};

class TypeStackElement {
public:
    explicit TypeStackElement(Checker *checker, const void *element,
                              const std::optional<util::DiagnosticWithParams> &diag, const lexer::SourcePosition &pos,
                              bool isRecursive = false)
        : checker_(checker), element_(element), isRecursive_(isRecursive)
    {
        if (!checker->typeStack_.insert({element, nullptr}).second) {
            if (isRecursive_) {
                cleanup_ = false;
            } else {
                checker_->LogError(diag->kind, diag->params, pos);
                element_ = nullptr;
            }
        }
    }

    bool HasTypeError()
    {
        hasErrorChecker_ = true;
        return element_ == nullptr;
    }

    Type *GetElementType()
    {
        auto recursiveType = checker_->typeStack_.find(element_);
        if (recursiveType != checker_->typeStack_.end()) {
            return recursiveType->second;
        }
        return nullptr;
    }

    void SetElementType(Type *type)
    {
        checker_->typeStack_[element_] = type;
    }

    ~TypeStackElement()
    {
        ES2PANDA_ASSERT(hasErrorChecker_);
        if (element_ != nullptr && cleanup_) {
            checker_->typeStack_.erase(element_);
        }
    }

    NO_COPY_SEMANTIC(TypeStackElement);
    NO_MOVE_SEMANTIC(TypeStackElement);

private:
    Checker *checker_;
    const void *element_;
    bool hasErrorChecker_ {false};
    bool isRecursive_;
    bool cleanup_ {true};
};

template <typename T>
class RecursionPreserver {
public:
    explicit RecursionPreserver(std::unordered_set<T *> &elementStack, T *element)
        : elementStack_(elementStack), element_(element)
    {
        recursion_ = !elementStack_.insert(element_).second;
    }

    bool &operator*()
    {
        return recursion_;
    }

    ~RecursionPreserver()
    {
        if (!recursion_) {
            elementStack_.erase(element_);
        }
    }

    NO_COPY_SEMANTIC(RecursionPreserver);
    NO_MOVE_SEMANTIC(RecursionPreserver);

private:
    std::unordered_set<T *> &elementStack_;
    T *element_;
    bool recursion_;
};

class ScopeContext {
public:
    explicit ScopeContext(Checker *checker, varbinder::Scope *newScope);

    ~ScopeContext()
    {
        checker_->scope_ = prevScope_;
        checker_->SetProgram(prevProgram_);
    }

    NO_COPY_SEMANTIC(ScopeContext);
    NO_MOVE_SEMANTIC(ScopeContext);

private:
    Checker *checker_;
    varbinder::Scope *prevScope_;
    parser::Program *prevProgram_;
};

class SavedCheckerContext {
public:
    explicit SavedCheckerContext(Checker *checker, CheckerStatus newStatus)
        : SavedCheckerContext(checker, newStatus, nullptr)
    {
    }

    explicit SavedCheckerContext(Checker *checker, CheckerStatus newStatus, const ETSObjectType *containingClass)
        : SavedCheckerContext(checker, newStatus, containingClass, nullptr)
    {
    }

    explicit SavedCheckerContext(Checker *checker, CheckerStatus newStatus, const ETSObjectType *containingClass,
                                 Signature *containingSignature)
        : checker_(checker), prev_(checker->context_)
    {
        const bool inExternal = checker->HasStatus(CheckerStatus::IN_EXTERNAL);
        checker_->context_ = CheckerContext(checker, newStatus, containingClass, containingSignature);
        if (inExternal) {
            // handled here instead of at call sites to make things more foolproof
            checker_->context_.Status() |= CheckerStatus::IN_EXTERNAL;
        }
    }

    NO_COPY_SEMANTIC(SavedCheckerContext);
    DEFAULT_MOVE_SEMANTIC(SavedCheckerContext);

    ~SavedCheckerContext()
    {
        checker_->context_ = prev_;
    }

private:
    Checker *checker_;
    CheckerContext prev_;
};

// VerifiedType is used to check return type from Expresssion not equal nullptr
class VerifiedType {
public:
    VerifiedType() = delete;
    ~VerifiedType() = default;

    DEFAULT_MOVE_SEMANTIC(VerifiedType);
    DEFAULT_COPY_SEMANTIC(VerifiedType);

    VerifiedType([[maybe_unused]] const ir::AstNode *const node, Type *type) : type_(type)
    {
        ES2PANDA_ASSERT(type != nullptr || !node->IsExpression());
    };

    Type *operator*() const
    {
        return type_;
    }

    // NOLINTNEXTLINE(*-explicit-constructor)
    operator Type *() const
    {
        return type_;
    }

    Type *operator->() const
    {
        return type_;
    }

    friend bool operator==(const VerifiedType &l, const std::nullptr_t &r)
    {
        return l.type_ == r;
    }

    friend bool operator==(const VerifiedType &l, const VerifiedType &r)
    {
        return l.type_ == r.type_;
    }

    friend bool operator!=(const VerifiedType &l, const std::nullptr_t &r)
    {
        return !(l == r);
    }

    friend bool operator!=(const VerifiedType &l, const VerifiedType &r)
    {
        return !(l == r);
    }

    // Other conparison is should't used with VerificationType

private:
    Type *type_ {};
};

class SignatureMatchContext {
public:
    explicit SignatureMatchContext(Checker *checker, util::DiagnosticType diagnosticKind, bool isLogError = true)
        : diagnosticEngine_(checker->DiagnosticEngine()),
          diagnosticCheckpoint_(),
          diagnosticKind_(diagnosticKind),
          isLogError_(isLogError)
    {
        diagnosticCheckpoint_ = diagnosticEngine_.Save();
    }

    bool ValidSignatureMatchStatus()
    {
        std::array<size_t, util::DiagnosticType::COUNT> diagnosticCheckpoint = diagnosticEngine_.Save();
        return diagnosticCheckpoint_[diagnosticKind_] == diagnosticCheckpoint[diagnosticKind_];
    }

    ~SignatureMatchContext()
    {
        if (isLogError_) {
            return;
        }

        diagnosticEngine_.Rollback(diagnosticCheckpoint_);
    }

    NO_COPY_SEMANTIC(SignatureMatchContext);
    NO_MOVE_SEMANTIC(SignatureMatchContext);

private:
    util::DiagnosticEngine &diagnosticEngine_;
    std::array<size_t, util::DiagnosticType::COUNT> diagnosticCheckpoint_;
    util::DiagnosticType diagnosticKind_;
    bool isLogError_;
};

class SignatureCollectContext {
public:
    explicit SignatureCollectContext(Checker *checker, varbinder::LocalVariable *overloadDeclaration,
                                     bool isCreateSyntheticVar = false)
        : checker_(checker), isCreateSyntheticVar_(isCreateSyntheticVar)
    {
        preOverloadDeclaration_ = checker_->Context().ContainingOverloadDeclaration() != nullptr
                                      ? checker_->Context().ContainingOverloadDeclaration()->AsLocalVariable()
                                      : nullptr;
        checker_->Context().SetContainingOverloadDeclaration(overloadDeclaration);
    }

    bool IsOverloadDeclarationCall()
    {
        return checker_->Context().ContainingOverloadDeclaration() != nullptr;
    }

    varbinder::LocalVariable *CreateSyntheticVar(ArenaAllocator *const allocator)
    {
        varbinder::VariableFlags variableFlags =
            IsOverloadDeclarationCall() ? varbinder::VariableFlags::SYNTHETIC | varbinder::VariableFlags::METHOD |
                                              varbinder::VariableFlags::OVERLOAD
                                        : varbinder::VariableFlags::SYNTHETIC | varbinder::VariableFlags::METHOD;
        varbinder::LocalVariable *syntheticVar = allocator->New<varbinder::LocalVariable>(variableFlags);
        return syntheticVar;
    }

    ~SignatureCollectContext()
    {
        if (isCreateSyntheticVar_ && syntheticVar_ != nullptr) {
            syntheticVar_->Reset(checker_->Context().ContainingOverloadDeclaration()->Declaration(),
                                 syntheticVar_->Flags());
            checker_->Context().SetContainingOverloadDeclaration(preOverloadDeclaration_);
        }
    }

    NO_COPY_SEMANTIC(SignatureCollectContext);
    NO_MOVE_SEMANTIC(SignatureCollectContext);

private:
    Checker *checker_;
    bool isCreateSyntheticVar_;
    varbinder::LocalVariable *preOverloadDeclaration_;
    varbinder::LocalVariable *syntheticVar_ = nullptr;
};

}  // namespace ark::es2panda::checker

#endif /* CHECKER_H */
