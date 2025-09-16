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

#include "sourceLocation.h"

#include "lexer/token/letters.h"
#include "parser/program/program.h"

#include <cstdint>

namespace ark::es2panda::lexer {
void OffsetEntry::AddCol(size_t offset)
{
    size_t diff = offset - offset_;
    offset_ = offset;

    if (ranges.empty()) {
        ranges.emplace_back(Range {diff});
        return;
    }

    auto &range = ranges.back();

    if (diff == range.byteSize) {
        range.cnt++;
    } else {
        ranges.emplace_back(Range {diff});
    }
}

LineIndex::LineIndex(const util::StringView &source) noexcept
{
    auto iter = util::StringView::Iterator(source);
    entries_.emplace_back(0);

    while (true) {
        switch (iter.Next()) {
            case util::StringView::Iterator::INVALID_CP: {
                return;
            }
            case LEX_CHAR_CR: {
                if (iter.HasNext() && iter.Peek() == LEX_CHAR_LF) {
                    iter.Forward(1);
                }

                [[fallthrough]];
            }
            case LEX_CHAR_LF:
            case LEX_CHAR_PS:
            case LEX_CHAR_LS: {
                entries_.emplace_back(iter.Index());
                break;
            }
            default: {
                entries_.back().AddCol(iter.Index());
            }
        }
    }
}

SourceLocation LineIndex::GetLocation(SourcePosition pos) const noexcept
{
    size_t line = pos.line;

    size_t col = 0;

    // It can occur during stdlib parsing where entries does not uploaded
    if (line > entries_.size()) {
        return SourceLocation(line + 1, col + 1, pos.Program());
    }

    if (line == entries_.size()) {
        --line;
    }

    const auto &entry = entries_[line];
    size_t diff = pos.index - entry.lineStart;

    for (const auto &range : entry.ranges) {
        if (diff < range.cnt) {
            col += diff;
            break;
        }

        diff -= range.cnt * range.byteSize;
        col += range.cnt;
    }

    return SourceLocation(line + 1, col + 1, pos.Program());
}

size_t LineIndex::GetOffset(SourceLocation loc) const noexcept
{
    ES2PANDA_ASSERT(loc.line != 0);
    ES2PANDA_ASSERT(loc.col != 0);
    size_t line = loc.line - 1;
    size_t col = loc.col - 1;

    if (line >= entries_.size()) {
        return 0;
    }

    const auto &entry = entries_[line];
    size_t offset = entry.lineStart;

    for (const auto &range : entry.ranges) {
        if (col < range.cnt) {
            offset += col * range.byteSize;
            break;
        }

        col -= range.cnt;
        offset += range.cnt * range.byteSize;
    }

    return offset;
}

SourceLocation SourcePosition::ToLocation() const
{
    return lexer::LineIndex(Program()->SourceCode()).GetLocation(*this);
}

const parser::Program *SourcePosition::Program() const
{
    if (program_ != nullptr) {
        ES2PANDA_ASSERT(!program_->IsDied());
    }
    return program_;
}

const parser::Program *SourceLocation::Program() const
{
    if (program_ != nullptr) {
        ES2PANDA_ASSERT(!program_->IsDied());
    }
    return program_;
}

void SourcePosition::SetProgram(const parser::Program *program)
{
    program_ = program;
}

void SourceRange::SetProgram(const parser::Program *program)
{
    start.SetProgram(program);
    end.SetProgram(program);
}

}  // namespace ark::es2panda::lexer
