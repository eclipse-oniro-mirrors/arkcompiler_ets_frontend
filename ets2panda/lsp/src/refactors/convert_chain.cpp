/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 */

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>
#include "ir/astNode.h"
#include "ir/expression.h"
#include "lsp/include/internal_api.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/conditionalExpression.h"
#include "lsp/include/refactors/convert_chain.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "refactor_provider.h"

/**
 * @file convert_chain.cpp
 * @brief Implements the ConvertToOptionalChainExpressionRefactor.
 *
 * This refactor identifies `&&` chains and conditional (`?:`) expressions
 * that can be safely transformed into optional chaining (`?.`, `?.()`, `??`).
 * When applied, it replaces the original chain with an equivalent expression
 * using optional chaining operators.
 *
 * ### Supported patterns
 * - `a && a.b && a.b.c` → `a?.b?.c`
 * - `a && a["b"] && a["b"].c` → `a?.["b"]?.c`
 * - `a && a.b && a.b()` → `a?.b?.()`
 * - `a && a.b ? a.b.c : d` → `a?.b?.c ?? d`
 *
 * ### Not supported
 * - Logical `||` chains
 * - Already-optional chains (e.g., `a?.b`)
 * - Complex mixed logical/bitwise chains that don’t fit the access pattern
 *
 * The implementation works by:
 *  - Locating the outermost `&&` or conditional expression covering the cursor
 *  - Collecting repeated occurrences of the access chain
 *  - Building a replacement string that preserves property, call, or computed
 *    access while inserting `?.` or `??`
 *
 * @see ConvertToOptionalChainExpressionRefactor
 * @see ApplicableRefactorInfo
 * @see RefactorEditInfo
 */
namespace ark::es2panda::lsp {
namespace {
constexpr const char *K_ACTIONKIND_CSTR = "refactor.rewrite.expression.optionalChain";

struct TargetInfo {
    ir::Expression *expr {nullptr};
    ir::Expression *finalExpr {nullptr};
    std::vector<ir::Expression *> occurrences;
};

template <class Sv>
inline std::string ToStdString(const Sv &v)
{
    return std::string(v.data(), v.size());
}

inline const SourceFile *GetSourceFile(const RefactorContext &ctx)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(ctx.context);
    return (pub != nullptr) ? pub->sourceFile : nullptr;
}

inline ir::Expression *SkipParens(ir::Expression *e)
{
    return e;
}

inline ir::Expression *GetObjectOf(ir::Expression *e)
{
    if (e == nullptr) {
        return nullptr;
    }
    if (e->IsMemberExpression()) {
        return e->AsMemberExpression()->Object();
    }
    if (e->IsCallExpression()) {
        return e->AsCallExpression()->Callee();
    }
    return nullptr;
}

inline bool IsOptionalChainNode(const ir::Expression *e)
{
    if (e == nullptr) {
        return false;
    }
    if (e->IsMemberExpression()) {
        return e->AsMemberExpression()->IsOptional();
    }
    if (e->IsCallExpression()) {
        return e->AsCallExpression()->IsOptional();
    }
    return false;
}

static bool IsLogicalAndBetween(const RefactorContext &ctx, const ir::AstNode *l, const ir::AstNode *r)
{
    const SourceFile *sf = GetSourceFile(ctx);
    if (sf == nullptr) {
        return false;
    }

    const size_t a = l->End().index;
    const size_t b = r->Start().index;
    if (a >= b || b > sf->source.size()) {
        return false;
    }

    const std::string_view mid = sf->source.substr(a, b - a);
    return mid.find("&&") != std::string_view::npos;
}

ir::Expression *OutermostAccessOrCall(ir::Expression *e)
{
    if (e == nullptr) {
        return nullptr;
    }

    ir::AstNode *n = e;
    for (ir::AstNode *p = n->Parent(); p != nullptr; p = n->Parent()) {
        if (p->IsMemberExpression() && p->AsMemberExpression()->Object() == n) {
            n = p;
            continue;
        }

        if (p->IsCallExpression() && p->AsCallExpression()->Callee() == n) {
            n = p;
            continue;
        }

        break;
    }

    return n->AsExpression();
}

bool IsCandidateRoot(const ir::AstNode *n)
{
    if (n == nullptr) {
        return false;
    }
    if (n->IsBinaryExpression()) {
        return true;
    }
    if (n->IsConditionalExpression()) {
        return true;
    }
    return false;
}

ir::AstNode *LeastAncestorRootCovering(ir::AstNode *tok, const TextRange &span)
{
    ir::AstNode *nearest = nullptr;

    for (ir::AstNode *cur = tok; cur != nullptr; cur = cur->Parent()) {
        if (!IsCandidateRoot(cur)) {
            continue;
        }

        const size_t s = cur->Start().index;
        const size_t e = cur->End().index;
        if (s <= span.pos && span.end <= e) {
            nearest = cur;
        }
    }

    return nearest;
}

ir::BinaryExpression *OutermostAndChain(const RefactorContext &ctx, ir::BinaryExpression *be)
{
    if (be == nullptr) {
        return nullptr;
    }

    ir::BinaryExpression *cur = be;
    ir::AstNode *parent = cur->Parent();
    while (parent != nullptr) {
        if (!parent->IsBinaryExpression()) {
            break;
        }

        ir::BinaryExpression *pb = parent->AsBinaryExpression();
        if (!IsLogicalAndBetween(ctx, pb->Left(), pb->Right())) {
            break;
        }

        cur = pb;
        parent = cur->Parent();
    }

    return cur;
}

bool ChainStartsWith(ir::Expression *chain, ir::Expression *sub)
{
    ir::Expression *c = chain;
    while (c != nullptr && (c->IsCallExpression() || c->IsMemberExpression())) {
        if (c->DumpEtsSrc() == sub->DumpEtsSrc()) {
            break;
        }
        c = GetObjectOf(c);
    }
    ir::Expression *s = sub;
    while (c != nullptr && s != nullptr && c->IsMemberExpression() && s->IsMemberExpression()) {
        if (c->DumpEtsSrc() != s->DumpEtsSrc()) {
            return false;
        }
        c = GetObjectOf(c);
        s = GetObjectOf(s);
    }
    if (c == nullptr || s == nullptr) {
        return false;
    }
    if (!c->IsIdentifier() || !s->IsIdentifier()) {
        return false;
    }
    return c->DumpEtsSrc() == s->DumpEtsSrc();
}

static ir::Expression *GetMatchingStart(ir::Expression *chain, ir::Expression *sub)
{
    if (sub == nullptr) {
        return nullptr;
    }
    if (!(sub->IsIdentifier() || sub->IsMemberExpression())) {
        return nullptr;
    }
    return ChainStartsWith(SkipParens(chain), SkipParens(sub)) ? sub : nullptr;
}

static ir::Expression *FinalAccessInChain(ir::Expression *node)
{
    ir::Expression *cur = SkipParens(node);
    if (cur == nullptr) {
        return nullptr;
    }
    if (cur->IsBinaryExpression()) {
        return FinalAccessInChain(cur->AsBinaryExpression()->Left());
    }
    if ((cur->IsMemberExpression() || cur->IsCallExpression()) && !IsOptionalChainNode(cur)) {
        return OutermostAccessOrCall(cur);
    }
    return nullptr;
}

std::vector<ir::Expression *> CollectOccurrences(const RefactorContext &ctx, ir::Expression *matchTo,
                                                 ir::Expression *expr)
{
    std::vector<ir::Expression *> occ;
    ir::Expression *lhs = expr;
    while (lhs != nullptr && lhs->IsBinaryExpression()) {
        auto *be = lhs->AsBinaryExpression();
        if (!IsLogicalAndBetween(ctx, be->Left(), be->Right())) {
            break;
        }
        ir::Expression *right = SkipParens(be->Right());
        ir::Expression *m = GetMatchingStart(SkipParens(matchTo), right);
        if (!m) {
            break;
        }
        occ.push_back(m);
        matchTo = m;
        lhs = be->Left();
    }
    ir::Expression *finalMatch = GetMatchingStart(matchTo, lhs);
    if (finalMatch != nullptr) {
        occ.push_back(finalMatch);
    }
    return occ;
}

std::string ArgumentsToSrc(const ir::CallExpression *call)
{
    std::string s;
    const auto &args = call->Arguments();
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) {
            s += ", ";
        }
        s += args[i]->DumpEtsSrc();
    }
    return s;
}

std::vector<ir::Expression *> BuildChainFromFinal(ir::Expression *finalExpr)
{
    std::vector<ir::Expression *> chain;
    for (ir::Expression *cur = finalExpr; cur; cur = GetObjectOf(cur)) {
        chain.push_back(cur);
        if (cur->IsIdentifier()) {
            break;
        }
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

void EmitMemberHop(std::string &out, const ir::MemberExpression *m)
{
    if (m->IsComputed()) {
        out += "?[";
        out += ((m->Property() != nullptr) ? m->Property()->DumpEtsSrc() : "?");
        out += "]";
        return;
    }
    out += "?.";
    out += ((m->Property() != nullptr) ? m->Property()->DumpEtsSrc() : "?");
}

void EmitCallHop(std::string &out, const ir::CallExpression *c)
{
    out += "?.(";
    out += ArgumentsToSrc(c);
    out += ")";
}

static std::string ConvertChainToOptional(ir::Expression *finalExpr)
{
    std::string out;
    std::vector<ir::Expression *> chain = BuildChainFromFinal(finalExpr);
    if (chain.empty()) {
        return out;
    }

    out += chain.front()->DumpEtsSrc();
    for (size_t i = 1; i < chain.size(); ++i) {
        ir::Expression *node = chain[i];
        if (node->IsMemberExpression()) {
            EmitMemberHop(out, node->AsMemberExpression());
            continue;
        }
        if (node->IsCallExpression()) {
            EmitCallHop(out, node->AsCallExpression());
            continue;
        }
        out += node->DumpEtsSrc();
    }
    return out;
}

static TargetInfo ResolveBinaryTarget(const RefactorContext &ctx, ir::BinaryExpression *rootBe)
{
    TargetInfo out {};
    if (rootBe == nullptr) {
        return out;
    }
    if (!IsLogicalAndBetween(ctx, rootBe->Left(), rootBe->Right())) {
        return out;
    }

    ir::BinaryExpression *be = OutermostAndChain(ctx, rootBe);
    ir::Expression *finalExpr = FinalAccessInChain(be->Right());
    if (finalExpr == nullptr) {
        return out;
    }

    auto occ = CollectOccurrences(ctx, finalExpr, be->Left());
    if (occ.empty()) {
        return out;
    }

    out.expr = be->AsExpression();
    out.finalExpr = finalExpr;
    out.occurrences = std::move(occ);
    return out;
}

static TargetInfo ResolveConditionalTarget(const RefactorContext &ctx, ir::ConditionalExpression *ce)
{
    TargetInfo out {};
    if (ce == nullptr) {
        return out;
    }

    ir::Expression *finalExpr = FinalAccessInChain(ce->Consequent());
    if (!finalExpr) {
        return out;
    }

    ir::Expression *test = ce->Test()->AsExpression();
    if (test != nullptr && (test->IsIdentifier() || test->IsMemberExpression())) {
        if (GetMatchingStart(finalExpr, test)) {
            out.expr = ce->AsExpression();
            out.finalExpr = finalExpr;
            out.occurrences.push_back(test);
            return out;
        }
    }

    if (ce->Test()->IsBinaryExpression()) {
        auto occ = CollectOccurrences(ctx, finalExpr, ce->Test()->AsBinaryExpression()->AsExpression());
        if (!occ.empty()) {
            out.expr = ce->AsExpression();
            out.finalExpr = finalExpr;
            out.occurrences = std::move(occ);
            return out;
        }
    }
    return out;
}

TargetInfo ResolveTarget(const RefactorContext &ctx)
{
    TargetInfo out {};
    ir::AstNode *tok = GetTouchingToken(ctx.context, ctx.span.pos, false);
    if (tok == nullptr) {
        return out;
    }

    ir::AstNode *root = LeastAncestorRootCovering(tok, ctx.span);
    if (root == nullptr) {
        return out;
    }

    if (root->IsBinaryExpression()) {
        return ResolveBinaryTarget(ctx, root->AsBinaryExpression());
    }
    if (root->IsConditionalExpression()) {
        return ResolveConditionalTarget(ctx, root->AsConditionalExpression());
    }
    return out;
}

std::string BuildReplacement([[maybe_unused]] const RefactorContext &ctx, const TargetInfo &tgt)
{
    std::string converted = ConvertChainToOptional(tgt.finalExpr);
    if (tgt.expr->IsBinaryExpression()) {
        return converted;
    }
    if (tgt.expr->IsConditionalExpression()) {
        auto *ce = tgt.expr->AsConditionalExpression();
        std::string rhs = (ce->Alternate() != nullptr) ? ce->Alternate()->DumpEtsSrc() : "undefined";
        return converted + " ?? " + rhs;
    }
    return converted;
}

std::vector<FileTextChanges> ReplaceWholeNode(ir::AstNode *node, const std::string &text)
{
    std::vector<FileTextChanges> out;
    if (node == nullptr) {
        return out;
    }

    ir::AstNode *boundary = ChangeTracker::ToEditBoundary(node);
    if (boundary == nullptr) {
        boundary = node;
    }

    const size_t start = boundary->Start().index;
    const size_t end = boundary->End().index;
    const size_t len = (end > start) ? (end - start) : 0;
    FileTextChanges ftc;
    ftc.textChanges.emplace_back(TextSpan(start, len), text);
    out.emplace_back(std::move(ftc));
    return out;
}

std::vector<FileTextChanges> DoConvertToOptionalChainInternal(const RefactorContext &ctx, const TargetInfo &target)
{
    std::vector<FileTextChanges> empty;
    if (target.expr == nullptr || target.finalExpr == nullptr) {
        return empty;
    }
    const std::string repl = BuildReplacement(ctx, target);
    return ReplaceWholeNode(target.expr, repl);
}
}  // namespace

ConvertToOptionalChainExpressionRefactor::ConvertToOptionalChainExpressionRefactor()
{
    AddKind(std::string(K_ACTIONKIND_CSTR));
}

std::vector<ApplicableRefactorInfo> ConvertToOptionalChainExpressionRefactor::GetAvailableActions(
    const RefactorContext &refContext) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;
    if (!refContext.kind.empty() && !this->IsKind(refContext.kind)) {
        return res;
    }
    TargetInfo target = ResolveTarget(refContext);
    if (target.expr == nullptr) {
        return res;
    }
    applicableRef.name = ToStdString(refactor_name::CONVERT_CHAIN_REFACTOR_NAME);
    applicableRef.description = ToStdString(refactor_description::CONVERT_CHAIN_REFACTOR_DESC);
    applicableRef.action.name = ToStdString(TO_OPTIONAL_CHAIN_ACTION.name);
    applicableRef.action.description = ToStdString(TO_OPTIONAL_CHAIN_ACTION.description);
    applicableRef.action.kind = std::string(TO_OPTIONAL_CHAIN_ACTION.kind);
    res.push_back(applicableRef);
    return res;
}

std::unique_ptr<RefactorEditInfo> ConvertToOptionalChainExpressionRefactor::GetEditsForAction(
    const RefactorContext &context, const std::string &actionName) const
{
    if (!actionName.empty() && actionName != ToStdString(TO_OPTIONAL_CHAIN_ACTION.name)) {
        return nullptr;
    }

    TargetInfo target = ResolveTarget(context);
    if (target.expr == nullptr) {
        return nullptr;
    }

    auto edits = DoConvertToOptionalChainInternal(context, target);
    if (edits.empty()) {
        return nullptr;
    }
    return std::make_unique<RefactorEditInfo>(std::move(edits));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertToOptionalChainExpressionRefactor> g_convChainRefReg(
    "ConvertToOptionalChainExpressionRefactor");
}  // namespace ark::es2panda::lsp