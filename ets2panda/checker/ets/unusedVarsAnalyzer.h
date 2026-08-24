/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_UNUSED_VARS_ANALYZER_H
#define ES2PANDA_COMPILER_CHECKER_ETS_UNUSED_VARS_ANALYZER_H

#include "lexer/token/sourceLocation.h"
#include "util/diagnosticEngine.h"
#include "util/ustring.h"

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace ark::es2panda::ir {
class AstNode;
class Identifier;
class MemberExpression;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::checker {

class UnusedVarsAnalyzer {
public:
    explicit UnusedVarsAnalyzer(util::DiagnosticEngine &diagnosticEngine);
    void Analyze(const ir::AstNode *node);

private:
    enum class DeclarationKind {
        IMPORT,
        CLASS,
        FUNCTION,
        ENUM,
        INTERFACE,
        TYPE_ALIAS,
        VARIABLE,
        GLOBAL_VARIABLE,
        PARAMETER,
        PRIVATE_MEMBER,
        NAMESPACE_MEMBER,
    };

    struct DeclarationInfo {
        const varbinder::Variable *variable {};
        util::StringView name {};
        lexer::SourcePosition position {};
        DeclarationKind kind {};
        const ir::AstNode *declNode {};
    };

    struct NamedDeclarationKey {
        const varbinder::Variable *variable {};
        const ir::AstNode *declNode {};
        DeclarationKind kind {};
        std::string name {};

        bool operator==(const NamedDeclarationKey &other) const
        {
            return variable == other.variable && declNode == other.declNode && kind == other.kind && name == other.name;
        }
    };

    // Boost-style hash combine: mix each field with a golden-ratio constant and seed bit shifts.
    static void HashCombine(size_t &seed, const size_t value)
    {
        constexpr size_t HASH_COMBINE_MAGIC = 0x9e3779b9U;
        seed ^= value + HASH_COMBINE_MAGIC + (seed << 6U) + (seed >> 2U);
    }

    struct NamedDeclarationKeyHash {
        size_t operator()(const NamedDeclarationKey &key) const
        {
            auto hash = std::hash<const varbinder::Variable *> {}(key.variable);
            HashCombine(hash, std::hash<const ir::AstNode *> {}(key.declNode));
            HashCombine(hash, std::hash<size_t> {}(static_cast<size_t>(key.kind)));
            HashCombine(hash, std::hash<std::string> {}(key.name));
            return hash;
        }
    };

    using ScopedMemberKey = std::pair<const ir::AstNode *, std::string>;

    struct ScopedMemberKeyHash {
        size_t operator()(const ScopedMemberKey &key) const
        {
            auto hash = std::hash<const ir::AstNode *> {}(key.first);
            HashCombine(hash, std::hash<std::string> {}(key.second));
            return hash;
        }
    };

    void CollectDeclarations(const ir::AstNode *node);
    void MarkReferences(const ir::AstNode *node);
    void EmitWarnings() const;

    void CollectCurrentNodeDeclaration(const ir::AstNode *node, bool &skipChildren);
    void CollectImportDeclaration(const ir::AstNode *node);
    void CollectClassOrTypeDeclaration(const ir::AstNode *node, bool &skipChildren);
    void CollectFunctionOrMemberDeclaration(const ir::AstNode *node);
    void CollectLocalDeclaration(const ir::AstNode *node);
    void CollectBindingIdentifiers(const ir::AstNode *node, DeclarationKind kind, const ir::AstNode *declNode);
    void TryCollectIdentifier(const ir::Identifier *ident, DeclarationKind kind, const ir::AstNode *declNode);
    void TryMarkReference(const ir::Identifier *ident);
    void TryMarkPrivateMemberReference(const ir::MemberExpression *memberExpr);
    void RegisterDeclaration(const varbinder::Variable *variable, util::StringView name,
                             const lexer::SourcePosition &position, DeclarationKind kind, const ir::AstNode *declNode);
    void RegisterNamedDeclaration(const varbinder::Variable *variable, util::StringView name,
                                  const lexer::SourcePosition &position, DeclarationKind kind,
                                  const ir::AstNode *declNode);
    void RegisterScopedMemberDeclaration(const ir::Identifier *ident, const ir::AstNode *declNode,
                                         DeclarationKind kind);
    bool ShouldSkipDeclaration(const ir::AstNode *node) const;
    bool IsReadReference(const ir::AstNode *node) const;
    bool HasUserVisibleDeclarationSource(const DeclarationInfo &info) const;
    bool IsArkUIComponentImport(const DeclarationInfo &info) const;
    bool IsArkUIDslCallReference(const DeclarationInfo &info, const ir::Identifier *ident) const;

    util::DiagnosticEngine &diagnosticEngine_;
    std::unordered_map<const varbinder::Variable *, DeclarationInfo> declarations_ {};
    std::unordered_set<const varbinder::Variable *> references_ {};
    std::vector<DeclarationInfo> namedDeclarations_ {};
    std::unordered_set<NamedDeclarationKey, NamedDeclarationKeyHash> namedReferences_ {};
    std::unordered_map<ScopedMemberKey, DeclarationInfo, ScopedMemberKeyHash> scopedMemberDeclarations_ {};
    std::unordered_set<ScopedMemberKey, ScopedMemberKeyHash> scopedMemberReferences_ {};
    std::unordered_set<const ir::AstNode *> declarationNameNodes_ {};
};

}  // namespace ark::es2panda::checker

#endif
