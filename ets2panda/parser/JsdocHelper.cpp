/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "JsdocHelper.h"
#include <algorithm>
#include <ir/ets/etsModule.h>
#include <limits>
#include "lexer/lexer.h"
#include "ir/ets/etsTuple.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/statements/expressionStatement.h"

namespace ark::es2panda::parser {
static constexpr std::string_view JSDOC_END = "*/";

static constexpr size_t START_POS = 0;
static constexpr size_t COLLECT_CURRENT_POS = 1;

// NOLINTBEGIN(modernize-avoid-c-arrays)
static constexpr std::string_view POTENTIAL_PREFIX[] = {
    "@",        "get",    "set",    "let",    "const",   "overload", "async",   "readonly",
    "abstract", "native", "static", "public", "private", "declare",  "default", "export"};
// NOLINTEND(modernize-avoid-c-arrays)

// Note: Potential annotation allowed node need to collect jsdoc.
// NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
static const std::unordered_set<ir::AstNodeType> ANNOTATION_ALLOWED_NODE = {
    ir::AstNodeType::METHOD_DEFINITION,         ir::AstNodeType::CLASS_DECLARATION,
    ir::AstNodeType::STRUCT_DECLARATION,        ir::AstNodeType::FUNCTION_DECLARATION,
    ir::AstNodeType::TS_INTERFACE_DECLARATION,  ir::AstNodeType::CLASS_PROPERTY,
    ir::AstNodeType::VARIABLE_DECLARATION,      ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION,
    ir::AstNodeType::ARROW_FUNCTION_EXPRESSION, ir::AstNodeType::ANNOTATION_DECLARATION};
// NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)

static const ArenaVector<ir::AnnotationUsage *> &GetAstAnnotationUsage(const ir::AstNode *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::METHOD_DEFINITION: {
            auto *func = node->AsMethodDefinition()->Function();
            ES2PANDA_ASSERT(func != nullptr);
            return func->Annotations();
        }
        case ir::AstNodeType::CLASS_DECLARATION:
            return node->AsClassDeclaration()->Definition()->Annotations();
        case ir::AstNodeType::FUNCTION_DECLARATION:
            return node->AsFunctionDeclaration()->Annotations();
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            return node->AsTSInterfaceDeclaration()->Annotations();
        case ir::AstNodeType::CLASS_PROPERTY:
            return node->AsClassProperty()->Annotations();
        case ir::AstNodeType::VARIABLE_DECLARATION:
            return node->AsVariableDeclaration()->Annotations();
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
            return node->AsTSTypeAliasDeclaration()->Annotations();
        case ir::AstNodeType::ETS_PARAMETER_EXPRESSION:
            return node->AsETSParameterExpression()->Annotations();
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            return node->AsArrowFunctionExpression()->Annotations();
        case ir::AstNodeType::ANNOTATION_DECLARATION:
            return node->AsAnnotationDeclaration()->Annotations();
        case ir::AstNodeType::STRUCT_DECLARATION:
            return node->AsETSStructDeclaration()->Definition()->Annotations();
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static void HandlePotentialPrefix(parser::JsdocHelper *jsdocHelper)
{
    jsdocHelper->Iterator().Reset(jsdocHelper->Node()->Start().index);
    if (jsdocHelper->Iterator().Index() != START_POS) {
        jsdocHelper->BackwardAndSkipSpace(1U);
    }
    for (auto prefix : POTENTIAL_PREFIX) {
        auto currentSv = jsdocHelper->SourceView(START_POS, jsdocHelper->Iterator().Index() + COLLECT_CURRENT_POS);
        if (currentSv.EndsWith(prefix)) {
            jsdocHelper->BackwardAndSkipSpace(prefix.length());
        }
    }
}

static void HandlePotentialPrefixOrAnnotationUsage(parser::JsdocHelper *jsdocHelper)
{
    if (ANNOTATION_ALLOWED_NODE.count(jsdocHelper->Node()->Type()) == 0) {
        HandlePotentialPrefix(jsdocHelper);
        return;
    }

    auto const &annoUsage = GetAstAnnotationUsage(jsdocHelper->Node());
    if (annoUsage.empty()) {
        HandlePotentialPrefix(jsdocHelper);
        return;
    }

    const auto annoStartIndex = annoUsage[0]->Range().start.index;
    if (annoStartIndex == START_POS) {
        jsdocHelper->Iterator().Reset(START_POS);
        return;
    }

    // Note: eat current iter.
    jsdocHelper->Iterator().Reset(annoStartIndex - 1U);
    if (jsdocHelper->Iterator().Index() != START_POS) {
        // Note: eat token `@`
        jsdocHelper->BackwardAndSkipSpace(1U);
    }
}

void JsdocHelper::InitNode(const ir::AstNode *input)
{
    auto root = input;
    while (root->Parent() != nullptr) {
        root = root->Parent();
    }
    root_ = root;
    program_ = root_->AsETSModule()->Program();
    sourceCode_ = program_->SourceCode();
    iter_ = util::StringView::Iterator(sourceCode_);
    if (input->IsClassDefinition()) {
        node_ = input->Parent();
    } else {
        node_ = input;
    }
}

bool JsdocHelper::BackWardUntilJsdocStart()
{
    while (true) {
        const char32_t cp = Iterator().Index() == START_POS ? util::StringView::Iterator::INVALID_CP : PeekBackWard();
        switch (cp) {
            case util::StringView::Iterator::INVALID_CP: {
                break;
            }
            case lexer::LEX_CHAR_ASTERISK: {
                Backward(1);
                if (PeekBackWard() == lexer::LEX_CHAR_SLASH) {
                    // Note: found `/*` here, it is only the common start of comments, not jsdoc.
                    return false;
                }
                if (PeekBackWard() != lexer::LEX_CHAR_ASTERISK) {
                    continue;
                }

                if (Iterator().Index() == START_POS) {
                    break;
                }

                Backward(1);
                if (PeekBackWard() == lexer::LEX_CHAR_SLASH) {
                    return true;
                }
                continue;
            }
            default: {
                SkipCpBackward();
                continue;
            }
        }
        return false;
    }
}

util::StringView JsdocHelper::GetJsdocBackward()
{
    HandlePotentialPrefixOrAnnotationUsage(this);
    size_t jsdocEndPos = Iterator().Index() + COLLECT_CURRENT_POS;
    size_t backwardPos = jsdocEndPos;
    auto currentSourceView = SourceView(START_POS, jsdocEndPos);
    while (currentSourceView.EndsWith(JSDOC_END)) {
        BackwardAndSkipSpace(JSDOC_END.length());
        if (!BackWardUntilJsdocStart()) {
            break;
        }
        backwardPos = Iterator().Index();
        BackwardAndSkipSpace(1);
        currentSourceView = SourceView(START_POS, Iterator().Index() + COLLECT_CURRENT_POS);
    }

    if (backwardPos == jsdocEndPos) {
        return "";
    }
    return SourceView(backwardPos, jsdocEndPos);
}

// Note: Return first matched string that starts with `/*` or `/**` and ends with `*/`
util::StringView JsdocHelper::GetLicenseStringFromStart()
{
    Iterator().Reset(START_POS);
    auto licenseStart = START_POS;
    do {
        const char32_t cp = Iterator().Peek();
        switch (cp) {
            case util::StringView::Iterator::INVALID_CP: {
                break;
            }
            case lexer::LEX_CHAR_ASTERISK: {
                Forward(1);
                if (Iterator().Peek() == lexer::LEX_CHAR_SLASH) {
                    Forward(1);
                    break;
                }
                continue;
            }
            case lexer::LEX_CHAR_SLASH: {
                Forward(1);
                if (Iterator().Peek() == lexer::LEX_CHAR_ASTERISK) {
                    licenseStart = Iterator().Index() - 1;
                }
                continue;
            }
            default: {
                Iterator().SkipCp();
                continue;
            }
        }
        break;
    } while (true);

    return SourceView(licenseStart, Iterator().Index());
}

static constexpr size_t INVALID_SCAN_POS = std::numeric_limits<size_t>::max();

// Note: lexical states of the forward line scan below. JsdocHelper scans source text
// backwards without the lexer token stream, so `//`-like or `*/`-like sequences inside
// string literals or other comments must not be mistaken for real comment delimiters
// (e.g. the `//` in "https://example.cn").
enum class LineTriviaScanState {
    CODE,
    LINE_COMMENT,
    BLOCK_COMMENT,
    DOUBLE_QUOTED,
    SINGLE_QUOTED,
    TEMPLATE,
};

struct LineTriviaScanResult {
    LineTriviaScanState state {LineTriviaScanState::CODE};
    // Absolute source index of the `//` when a line comment starts within [begin, end).
    size_t lineCommentStart {INVALID_SCAN_POS};
};

static size_t FindLineStartIndex(const parser::JsdocHelper *jsdocHelper, size_t pos)
{
    size_t lineStart = pos;
    while (lineStart > START_POS) {
        const auto prevCp = jsdocHelper->SourceView(lineStart - 1U, lineStart);
        if (prevCp.Is("\n") || prevCp.Is("\r")) {
            break;
        }
        lineStart--;
    }
    return lineStart;
}

// Note: single-step handlers of ScanLineTrivia below, extracted to keep the state
// dispatch loop flat (codecheck G.FUN.01-CPP limits block nesting to four levels).

// Note: keep scanning past the line comment (it ends at the newline) so multi-line
// ranges are validated with the same state machine; keep the first `//` seen.
static void HandleLineCommentStart(LineTriviaScanResult &result, const util::StringView::Iterator &iter, size_t begin)
{
    if (result.lineCommentStart == INVALID_SCAN_POS) {
        result.lineCommentStart = begin + iter.Index() - 1U;
    }
}

static void HandleCodeCp(LineTriviaScanResult &result, util::StringView::Iterator &iter, char32_t cp, size_t begin)
{
    if (cp == lexer::LEX_CHAR_DOUBLE_QUOTE) {
        result.state = LineTriviaScanState::DOUBLE_QUOTED;
        return;
    }
    if (cp == lexer::LEX_CHAR_SINGLE_QUOTE) {
        result.state = LineTriviaScanState::SINGLE_QUOTED;
        return;
    }
    if (cp == lexer::LEX_CHAR_BACK_TICK) {
        result.state = LineTriviaScanState::TEMPLATE;
        return;
    }
    if (cp == lexer::LEX_CHAR_SLASH) {
        const char32_t nextCp = iter.Peek();
        if (nextCp == lexer::LEX_CHAR_ASTERISK) {
            (void)iter.Next();
            result.state = LineTriviaScanState::BLOCK_COMMENT;
            return;
        }
        if (nextCp == lexer::LEX_CHAR_SLASH) {
            HandleLineCommentStart(result, iter, begin);
            result.state = LineTriviaScanState::LINE_COMMENT;
            (void)iter.Next();
        }
        return;
    }
    if (cp == lexer::LEX_CHAR_ASTERISK && iter.Peek() == lexer::LEX_CHAR_SLASH) {
        // Note: `*/` in code state closes a block comment opened on an earlier line.
        (void)iter.Next();
    }
}

static void HandleQuotedCp(LineTriviaScanResult &result, util::StringView::Iterator &iter, char32_t cp)
{
    if (cp == lexer::LEX_CHAR_BACKSLASH) {
        if (iter.HasNext()) {
            (void)iter.Next();
        }
        return;
    }
    if (result.state == LineTriviaScanState::DOUBLE_QUOTED && cp == lexer::LEX_CHAR_DOUBLE_QUOTE) {
        result.state = LineTriviaScanState::CODE;
        return;
    }
    if (result.state == LineTriviaScanState::SINGLE_QUOTED && cp == lexer::LEX_CHAR_SINGLE_QUOTE) {
        result.state = LineTriviaScanState::CODE;
        return;
    }
    if (result.state == LineTriviaScanState::TEMPLATE && cp == lexer::LEX_CHAR_BACK_TICK) {
        result.state = LineTriviaScanState::CODE;
    }
}

static void HandleBlockCommentCp(LineTriviaScanResult &result, util::StringView::Iterator &iter, char32_t cp)
{
    if (cp == lexer::LEX_CHAR_ASTERISK && iter.Peek() == lexer::LEX_CHAR_SLASH) {
        (void)iter.Next();
        result.state = LineTriviaScanState::CODE;
    }
}

// Note: line comments end at the newline; scanning continues on the next line so
// multi-line ranges can be validated with the same state machine.
static void HandleLineCommentCp(LineTriviaScanResult &result, char32_t cp)
{
    if (cp == lexer::LEX_CHAR_LF || cp == lexer::LEX_CHAR_CR) {
        result.state = LineTriviaScanState::CODE;
    }
}

// Note: forward lexical scan of [begin, end) tracking strings and comments, used to
// validate comment candidates produced by the backward scan. Only ASCII delimiters
// matter; multi-byte UTF-8 content never matches them. Returns an empty CODE result
// when the range is empty or reversed.
static LineTriviaScanResult ScanLineTrivia(const parser::JsdocHelper *jsdocHelper, size_t begin, size_t end)
{
    LineTriviaScanResult result;
    if (begin >= end) {
        return result;
    }
    const auto rangeView = jsdocHelper->SourceView(begin, end);
    util::StringView::Iterator iter(rangeView);

    while (iter.HasNext()) {
        const char32_t cp = iter.Next();
        switch (result.state) {
            case LineTriviaScanState::CODE:
                HandleCodeCp(result, iter, cp, begin);
                break;
            case LineTriviaScanState::DOUBLE_QUOTED:
            case LineTriviaScanState::SINGLE_QUOTED:
            case LineTriviaScanState::TEMPLATE:
                HandleQuotedCp(result, iter, cp);
                break;
            case LineTriviaScanState::BLOCK_COMMENT:
                HandleBlockCommentCp(result, iter, cp);
                break;
            case LineTriviaScanState::LINE_COMMENT:
                HandleLineCommentCp(result, cp);
                break;
            default:
                return result;
        }
    }
    return result;
}

// Note: the trailing `*/` before the node is a real block-comment end only when it is not
// inside a string literal or a line comment (e.g. `// note */`); otherwise the backward
// block-comment scan would run through unrelated code and produce a bogus comment range.
static bool IsRealBlockCommentEnd(const parser::JsdocHelper *jsdocHelper, size_t slashPos)
{
    // Note: defensive guard, callers pass the index of `/` in a `*/` suffix, which is >= 2;
    // reject degenerate input so `slashPos - 1U` can never underflow.
    if (slashPos == 0U || slashPos > jsdocHelper->SourceLength()) {
        return false;
    }
    const size_t lineStart = FindLineStartIndex(jsdocHelper, slashPos - 1U);
    const auto scan = ScanLineTrivia(jsdocHelper, lineStart, slashPos - 1U);
    // CODE: closes a block comment opened on an earlier line; BLOCK_COMMENT: closes a block
    // comment opened on the same line. Both are real comment terminators.
    return scan.state == LineTriviaScanState::CODE || scan.state == LineTriviaScanState::BLOCK_COMMENT;
}

// Note: check whether `/*` at [slashPos, slashPos + 1) opens a real block comment: the
// delimiter must not sit inside a string literal or inside another comment (a `/*` inside
// a block comment is inert text, and `// x /* y */` keeps scanning in LINE_COMMENT state).
// Scanning from the source start tracks the full lexical context, so the inner `/*` of
// `/* a /* b */` is rejected while the outer one is accepted.
static bool IsRealBlockCommentStart(const parser::JsdocHelper *jsdocHelper, size_t slashPos)
{
    if (slashPos >= jsdocHelper->SourceLength()) {
        return false;
    }
    const auto scan = ScanLineTrivia(jsdocHelper, START_POS, slashPos);
    return scan.state == LineTriviaScanState::CODE;
}

// Note: one backward step after peeking a `*` (see BackWardUntilBlockCommentStart):
// `PeekBackWard` reads the character at the current index; after `Backward(1)` it reads
// the candidate `*`'s predecessor. Returns true when a real block-comment start is
// reached (iterator stops on its `/`); false keeps the scan going (inert candidate,
// string/comment content, or `/**` double-star probing).
bool JsdocHelper::StepBackwardFromAsterisk()
{
    Backward(1);
    if (PeekBackWard() == lexer::LEX_CHAR_SLASH) {
        // Note: found `/*`; accept it only when it opens a real block comment,
        // i.e. not string content and not inert text inside another comment
        // (`/* a /* b */` keeps the outermost `/*`).
        if (IsRealBlockCommentStart(this, Iterator().Index())) {
            return true;
        }
        return false;
    }
    if (PeekBackWard() != lexer::LEX_CHAR_ASTERISK) {
        return false;
    }

    if (Iterator().Index() == START_POS) {
        return false;
    }

    Backward(1);
    if (PeekBackWard() == lexer::LEX_CHAR_SLASH) {
        // Note: `/**` start; its `/` gets validated on the next loop pass
        // (`PeekBackWard` is now the first `*` of `/**`).
        Forward(1);
    }
    return false;
}

bool JsdocHelper::BackWardUntilBlockCommentStart()
{
    while (Iterator().Index() != START_POS) {
        if (PeekBackWard() != lexer::LEX_CHAR_ASTERISK) {
            SkipCpBackward();
            continue;
        }
        if (StepBackwardFromAsterisk()) {
            return true;
        }
        if (Iterator().Index() == START_POS) {
            break;
        }
    }
    return false;
}

bool JsdocHelper::TryConsumeLineCommentBackward()
{
    const size_t savedIndex = Iterator().Index();
    SkipWhiteSpacesBackward();
    // Note: the iterator now points at the last non-whitespace character before the node,
    // so a valid line comment must cover that character. Scan the whole line forward so
    // that `//`-like content inside strings is rejected and a comment containing `//`
    // (e.g. an URL) is consumed from its real start.
    const size_t contentEnd = Iterator().Index() + COLLECT_CURRENT_POS;
    const size_t lineStart = FindLineStartIndex(this, Iterator().Index());
    const auto scan = ScanLineTrivia(this, lineStart, contentEnd);
    if (scan.state != LineTriviaScanState::LINE_COMMENT || scan.lineCommentStart == INVALID_SCAN_POS) {
        Iterator().Reset(savedIndex);
        return false;
    }
    Iterator().Reset(scan.lineCommentStart);
    return true;
}

// Note: Property-access MemberExpression / CallExpression ranges start at the chain head
// (e.g. `this` in `this.a().b()`), so comments between chain links sit after Node()->Start().
// After OptionalLowering, Object() ranges may no longer bracket the source trivia; locate `.` /
// `?.` from the property's source position instead.
static const ir::MemberExpression *GetPropertyAccessMemberForComments(const ir::AstNode *node)
{
    if (node->IsExpressionStatement()) {
        return GetPropertyAccessMemberForComments(node->AsExpressionStatement()->GetExpression());
    }
    if (node->IsChainExpression()) {
        return GetPropertyAccessMemberForComments(node->AsChainExpression()->GetExpression());
    }
    if (node->IsCallExpression()) {
        const auto *callee = node->AsCallExpression()->Callee();
        if (callee != nullptr && callee->IsChainExpression()) {
            callee = callee->AsChainExpression()->GetExpression();
        }
        if (callee != nullptr && callee->IsMemberExpression()) {
            const auto *member = callee->AsMemberExpression();
            if (member->HasMemberKind(ir::MemberExpressionKind::PROPERTY_ACCESS)) {
                return member;
            }
        }
        return nullptr;
    }
    if (node->IsMemberExpression()) {
        const auto *member = node->AsMemberExpression();
        if (member->HasMemberKind(ir::MemberExpressionKind::PROPERTY_ACCESS)) {
            return member;
        }
        return nullptr;
    }
    if (node->IsIdentifier() && node->Parent() != nullptr && node->Parent()->IsMemberExpression()) {
        const auto *member = node->Parent()->AsMemberExpression();
        if (member->Property() == node && member->HasMemberKind(ir::MemberExpressionKind::PROPERTY_ACCESS)) {
            return member;
        }
    }
    return nullptr;
}

static constexpr size_t OPTIONAL_CHAIN_LEN = 2;

static size_t FindPropertyAccessPunctuatorStart(parser::JsdocHelper *jsdocHelper, size_t propStart)
{
    if (propStart >= OPTIONAL_CHAIN_LEN) {
        const auto beforeProp = jsdocHelper->SourceView(propStart - OPTIONAL_CHAIN_LEN, propStart);
        if (beforeProp.Is("?.")) {
            return propStart - OPTIONAL_CHAIN_LEN;
        }
    }
    if (propStart > START_POS) {
        const auto beforeProp = jsdocHelper->SourceView(propStart - 1, propStart);
        if (beforeProp.Is(".")) {
            return propStart - 1;
        }
    }
    // Property range may already include `.` / `?.`. Clamp the right bound: `Substr` is
    // unchecked, and the source may end right at the property (no trailing newline/semicolon).
    const size_t sourceLength = jsdocHelper->SourceLength();
    if (propStart >= sourceLength) {
        return INVALID_SCAN_POS;
    }
    const auto atProp = jsdocHelper->SourceView(propStart, std::min(propStart + OPTIONAL_CHAIN_LEN, sourceLength));
    if (atProp.StartsWith("?.") || atProp.StartsWith(".")) {
        return propStart;
    }
    return INVALID_SCAN_POS;
}

static size_t ResolveCommentsFallbackStartIndex(parser::JsdocHelper *jsdocHelper)
{
    const ir::AstNode *node = jsdocHelper->Node();
    const ir::MemberExpression *member = GetPropertyAccessMemberForComments(node);
    if (member != nullptr) {
        return member->Start().index;
    }
    if (node->IsExpressionStatement()) {
        return node->AsExpressionStatement()->GetExpression()->Start().index;
    }
    return node->Start().index;
}

static size_t ResolveCommentsPunctuatorStartIndex(parser::JsdocHelper *jsdocHelper)
{
    const ir::MemberExpression *member = GetPropertyAccessMemberForComments(jsdocHelper->Node());
    if (member == nullptr || member->Property() == nullptr) {
        return INVALID_SCAN_POS;
    }
    return FindPropertyAccessPunctuatorStart(jsdocHelper, member->Property()->Start().index);
}

static void PositionCommentsScanStart(parser::JsdocHelper *jsdocHelper, size_t startIndex)
{
    jsdocHelper->Iterator().Reset(startIndex);
    if (jsdocHelper->Iterator().Index() != START_POS) {
        jsdocHelper->BackwardAndSkipSpace(1U);
    }
    for (auto prefix : POTENTIAL_PREFIX) {
        auto currentSv = jsdocHelper->SourceView(START_POS, jsdocHelper->Iterator().Index() + COLLECT_CURRENT_POS);
        if (currentSv.EndsWith(prefix)) {
            jsdocHelper->BackwardAndSkipSpace(prefix.length());
        }
    }
}

// Note: collect contiguous comments (block/line) ending at the current iterator
// position; returns "" when no comment directly precedes it.
util::StringView JsdocHelper::CollectCommentsFromCurrent()
{
    const size_t commentsEndPos = Iterator().Index() + COLLECT_CURRENT_POS;
    size_t backwardPos = commentsEndPos;

    while (true) {
        auto currentSourceView = SourceView(START_POS, Iterator().Index() + COLLECT_CURRENT_POS);
        if (currentSourceView.EndsWith(JSDOC_END) && IsRealBlockCommentEnd(this, Iterator().Index())) {
            BackwardAndSkipSpace(JSDOC_END.length());
            if (!BackWardUntilBlockCommentStart()) {
                break;
            }
            backwardPos = Iterator().Index();
            BackwardAndSkipSpace(1);
            continue;
        }

        if (TryConsumeLineCommentBackward()) {
            backwardPos = Iterator().Index();
            BackwardAndSkipSpace(1);
            continue;
        }

        break;
    }

    if (backwardPos == commentsEndPos) {
        return "";
    }
    return SourceView(backwardPos, commentsEndPos);
}

util::StringView JsdocHelper::GetCommentsBackward()
{
    if (ANNOTATION_ALLOWED_NODE.count(Node()->Type()) != 0) {
        HandlePotentialPrefixOrAnnotationUsage(this);
        return CollectCommentsFromCurrent();
    }

    // Prefer `.` / `?.` before the property so mid-chain comments work even after
    // OptionalLowering rewrites Object() ranges. Fall back to the expression start for
    // cases like `arg.method()` whose leading comments sit before the whole expression.
    const size_t punctuatorStart = ResolveCommentsPunctuatorStartIndex(this);
    const size_t fallbackStart = ResolveCommentsFallbackStartIndex(this);
    if (punctuatorStart != INVALID_SCAN_POS && punctuatorStart != fallbackStart) {
        PositionCommentsScanStart(this, punctuatorStart);
        auto comments = CollectCommentsFromCurrent();
        if (!comments.Empty()) {
            return comments;
        }
    }

    PositionCommentsScanStart(this, fallbackStart);
    return CollectCommentsFromCurrent();
}
}  // namespace ark::es2panda::parser
