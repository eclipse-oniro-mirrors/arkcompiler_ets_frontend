/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "ETSNolintParser.h"
#include "public/public.h"

namespace ark::es2panda::parser {
ETSNolintParser::ETSNolintParser(ParserImpl *mainParser) : parser_(mainParser)
{
    line_ = parser_->Lexer()->Line();
}

static bool WarningCollectionEnabled(public_lib::Context *ctx)
{
    return !ctx->config->options->GetEtsWarningCollection().empty();
}

void ETSNolintParser::SetStartPos()
{
    line_ = parser_->Lexer()->Pos().Line();
    startPos_ = parser_->Lexer()->Pos().Iterator().Index();
    posOffset_ = startPos_;

    BackwardSymbol(startPos_);
}

void ETSNolintParser::CollectETSNolints()
{
    if (!WarningCollectionEnabled(parser_->ctx_)) {
        return;
    }

    SetStartPos();
    char32_t cp = PeekSymbol();

    while (cp != lexer::LEX_CHAR_EOS && cp != lexer::UNICODE_CODE_POINT_MAX && cp != lexer::UNICODE_INVALID_CP) {
        if (!IsEtsNolint()) {
            NextSymbol();
            cp = PeekSymbol();
            continue;
        }
        std::size_t line = line_;
        std::set<ETSWarnings> collection;
        if (IsNextLine()) {
            collection = ParseETSNolintArgs();
            line += 1;
        } else if (IsBegin()) {
            collection = ParseETSNolintArgs();
            for (const auto it : collection) {
                applyingCollection_.insert(it);
            }
        } else if (IsEnd()) {
            collection = ParseETSNolintArgs();

            for (const auto it : collection) {
                applyingCollection_.erase(it);
            }
            cp = PeekSymbol();
            continue;
        } else {
            collection = ParseETSNolintArgs();
        }
        AddToETSNolintLinesCollection(line, collection);
        cp = PeekSymbol();
    }

    RewindToStart();
}

void ETSNolintParser::ApplyETSNolintsToStatements(ArenaVector<ir::Statement *> &statements) const
{
    if (!WarningCollectionEnabled(parser_->ctx_)) {
        return;
    }

    for (auto *it : statements) {
        ApplyETSNolintsToNodesRecursively(it);
    }
}

void ETSNolintParser::NextSymbol()
{
    if (PeekSymbol() == lexer::LEX_CHAR_LF) {
        if (!applyingCollection_.empty()) {
            AddToETSNolintLinesCollection(line_, applyingCollection_);
        }
        line_++;
    }

    posOffset_++;
    parser_->Lexer()->Pos().Iterator().Forward(1);
}

void ETSNolintParser::BackwardSymbol()
{
    posOffset_--;
    parser_->Lexer()->Pos().Iterator().Backward(1);

    if (PeekSymbol() == lexer::LEX_CHAR_LF) {
        line_--;
    }
}

void ETSNolintParser::NextSymbol(std::size_t i)
{
    for (; i > 0; --i) {
        NextSymbol();
    }
}

void ETSNolintParser::BackwardSymbol(std::size_t i)
{
    for (; i > 0; --i) {
        BackwardSymbol();
    }
}

void ETSNolintParser::RewindToStart() const
{
    parser_->Lexer()->Pos().Iterator().Backward(posOffset_ - startPos_);
}

void ETSNolintParser::AddToETSNolintLinesCollection(std::size_t line, const std::set<ETSWarnings> &collection)
{
    const auto search = linesCollection_.find(line);
    if (search != linesCollection_.end()) {
        search->second.insert(collection.begin(), collection.end());
        return;
    }

    linesCollection_.insert({line, collection});
}

char32_t ETSNolintParser::PeekSymbol() const
{
    return parser_->Lexer()->Pos().Iterator().Peek();
}

bool ETSNolintParser::TryPeekU32String(const std::u32string &u32str)
{
    std::size_t localPosOffset = 0;
    char32_t cp;

    for (const char32_t i : u32str) {
        cp = PeekSymbol();
        if (i != cp) {
            BackwardSymbol(localPosOffset);
            return false;
        }
        NextSymbol();
        localPosOffset++;
    }
    return true;
}

bool ETSNolintParser::IsEtsNolint()
{
    static const std::u32string ETSNOLINT_CHAR32T = {
        lexer::LEX_CHAR_UPPERCASE_E, lexer::LEX_CHAR_UPPERCASE_T, lexer::LEX_CHAR_UPPERCASE_S,
        lexer::LEX_CHAR_UPPERCASE_N, lexer::LEX_CHAR_UPPERCASE_O, lexer::LEX_CHAR_UPPERCASE_L,
        lexer::LEX_CHAR_UPPERCASE_I, lexer::LEX_CHAR_UPPERCASE_N, lexer::LEX_CHAR_UPPERCASE_T};

    char32_t cp;

    for (unsigned long i = 0; i < ETSNOLINT_CHAR32T.length(); i++) {
        cp = PeekSymbol();
        if (ETSNOLINT_CHAR32T[i] != cp) {
            return false;
        }

        NextSymbol();
    }

    return true;
}

bool ETSNolintParser::IsNextLine()
{
    static const std::u32string NEXTLINE_CHAR32T = {
        lexer::LEX_CHAR_MINUS,       lexer::LEX_CHAR_UPPERCASE_N, lexer::LEX_CHAR_UPPERCASE_E,
        lexer::LEX_CHAR_UPPERCASE_X, lexer::LEX_CHAR_UPPERCASE_T, lexer::LEX_CHAR_UPPERCASE_L,
        lexer::LEX_CHAR_UPPERCASE_I, lexer::LEX_CHAR_UPPERCASE_N, lexer::LEX_CHAR_UPPERCASE_E};

    return TryPeekU32String(NEXTLINE_CHAR32T);
}

bool ETSNolintParser::IsBegin()
{
    static const std::u32string BEGIN_CHAR32T = {lexer::LEX_CHAR_MINUS,       lexer::LEX_CHAR_UPPERCASE_B,
                                                 lexer::LEX_CHAR_UPPERCASE_E, lexer::LEX_CHAR_UPPERCASE_G,
                                                 lexer::LEX_CHAR_UPPERCASE_I, lexer::LEX_CHAR_UPPERCASE_N};

    return TryPeekU32String(BEGIN_CHAR32T);
}

bool ETSNolintParser::IsEnd()
{
    static const std::u32string END_CHAR32T = {lexer::LEX_CHAR_MINUS, lexer::LEX_CHAR_UPPERCASE_E,
                                               lexer::LEX_CHAR_UPPERCASE_N, lexer::LEX_CHAR_UPPERCASE_D};

    return TryPeekU32String(END_CHAR32T);
}

ETSWarnings ETSNolintParser::MapETSNolintArg(const std::string &warningName) const
{
    return util::gen::ets_warnings::FromString(warningName);
}

bool ETSNolintParser::ValidETSNolintArg(const std::string &warningName) const
{
    return util::gen::ets_warnings::FromString(warningName) != ETSWarnings::INVALID;
}

std::set<ETSWarnings> ETSNolintParser::ParseETSNolintArgs()
{
    std::set<ETSWarnings> warningsCollection;

    if (PeekSymbol() != lexer::LEX_CHAR_LEFT_PAREN) {
        for (std::underlying_type_t<ETSWarnings> wid = ETSWarnings::FIRST; wid <= ETSWarnings::LAST; wid++) {
            warningsCollection.insert(ETSWarnings {wid});
        }
        return warningsCollection;
    }

    NextSymbol();
    char32_t cp = 0;
    std::string warningName;

    while (cp != lexer::LEX_CHAR_SP && cp != lexer::LEX_CHAR_LF && cp != lexer::LEX_CHAR_EOS) {
        cp = PeekSymbol();
        const lexer::SourcePosition sPos {line_ + 1, 0, parser_->GetProgram()};
        if (cp != lexer::LEX_CHAR_MINUS && cp != lexer::LEX_CHAR_COMMA && cp != lexer::LEX_CHAR_RIGHT_PAREN &&
            (cp < lexer::LEX_CHAR_LOWERCASE_A || cp > lexer::LEX_CHAR_LOWERCASE_Z)) {
            parser_->DiagnosticEngine().LogDiagnostic(diagnostic::UNEXPECTED_CHAR_ETSNOLINT,
                                                      util::DiagnosticMessageParams {}, sPos);
        }
        if ((cp == lexer::LEX_CHAR_COMMA || cp == lexer::LEX_CHAR_RIGHT_PAREN) && !ValidETSNolintArg(warningName)) {
            parser_->DiagnosticEngine().LogDiagnostic(diagnostic::INVALID_ARGUMENT_ETSNOLINT,
                                                      util::DiagnosticMessageParams {}, sPos);
        }
        if ((cp == lexer::LEX_CHAR_COMMA || cp == lexer::LEX_CHAR_RIGHT_PAREN) && ValidETSNolintArg(warningName)) {
            warningsCollection.insert(MapETSNolintArg(warningName));
            warningName.clear();
        } else {
            ES2PANDA_ASSERT(cp <= std::numeric_limits<unsigned char>::max());
            warningName += cp;
        }
        if (cp == lexer::LEX_CHAR_RIGHT_PAREN) {
            break;
        }

        NextSymbol();
    }

    return warningsCollection;
}

bool ETSNolintParser::IsLineWithETSNolint(const std::size_t line) const
{
    return linesCollection_.find(line) != linesCollection_.end();
}

std::set<ETSWarnings> ETSNolintParser::GetWarningsCollectionByLine(std::size_t line) const
{
    const auto search = linesCollection_.find(line);
    return search == linesCollection_.end() ? std::set<ETSWarnings> {} : search->second;
}

void ETSNolintParser::ApplyETSNolintsToNodesRecursively(ir::AstNode *node) const
{
    if (node == nullptr) {
        return;
    }
    const std::size_t line = node->Start().line;
    if (IsLineWithETSNolint(line)) {
        const std::set<ETSWarnings> warningsCollection = GetWarningsCollectionByLine(line);

        parser_->GetProgram()->AddNodeToETSNolintCollection(node, warningsCollection);
    }
    node->Iterate([&](auto *childNode) { ApplyETSNolintsToNodesRecursively(childNode); });
}
}  // namespace ark::es2panda::parser
