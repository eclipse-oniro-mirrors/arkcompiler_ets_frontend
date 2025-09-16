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

#ifndef ES2PANDA_LEXER_TOKEN_SOURCE_LOCATION_H
#define ES2PANDA_LEXER_TOKEN_SOURCE_LOCATION_H

#include "macros.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::util {
class StringView;
}  // namespace ark::es2panda::util

namespace ark::es2panda::lexer {

class SourceLocation;
class SourcePosition {
public:
    explicit SourcePosition() noexcept = default;
    explicit SourcePosition(const parser::Program *prog) noexcept : program_(prog) {}
    explicit SourcePosition(size_t i, size_t l, const parser::Program *prog) noexcept
        : index(i), line(l), program_(prog)
    {
    }

    DEFAULT_COPY_SEMANTIC(SourcePosition);
    DEFAULT_MOVE_SEMANTIC(SourcePosition);
    ~SourcePosition() = default;
    SourceLocation ToLocation() const;
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    size_t index {};
    size_t line {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    const parser::Program *Program() const;
    void SetProgram(const parser::Program *program);

    bool operator!=(const SourcePosition &other) const
    {
        return index != other.index || line != other.line || program_ != other.program_;
    }

private:
    const parser::Program *program_ {};
};

class SourceRange {
public:
    explicit SourceRange() noexcept = default;
    SourceRange(SourcePosition s, SourcePosition e) noexcept : start(s), end(e) {}
    DEFAULT_COPY_SEMANTIC(SourceRange);
    DEFAULT_MOVE_SEMANTIC(SourceRange);
    ~SourceRange() = default;

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    SourcePosition start {};
    SourcePosition end {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    bool operator!=(const SourceRange &other) const
    {
        return start != other.start || end != other.end;
    }

    void SetProgram(const parser::Program *program);
};

class CompressedSourceRange {
public:
    explicit CompressedSourceRange() = default;
    DEFAULT_COPY_SEMANTIC(CompressedSourceRange);
    DEFAULT_MOVE_SEMANTIC(CompressedSourceRange);
    ~CompressedSourceRange() = default;

    void SetStart(SourcePosition const &s)
    {
        startLine_ = Limit<uint32_t>(s.line);
        startIndex_ = Limit<uint32_t>(s.index);
        program_ = s.Program();
    }

    void SetEnd(SourcePosition const &e)
    {
        endLine_ = Limit<uint32_t>(e.line);
        endIndex_ = Limit<uint32_t>(e.index);
        program_ = e.Program();
    }

    SourcePosition GetStart() const
    {
        return SourcePosition(startIndex_, startLine_, program_);
    }

    SourcePosition GetEnd() const
    {
        return SourcePosition(endIndex_, endLine_, program_);
    }

    void SetRange(SourceRange const &r)
    {
        SetStart(r.start);
        SetEnd(r.end);
    }

    void SetProgram(const parser::Program *program)
    {
        program_ = program;
    }

    SourceRange GetRange() const
    {
        return SourceRange(GetStart(), GetEnd());
    }

private:
    template <typename T>
    static T Limit(uint64_t val)
    {
        return val > std::numeric_limits<T>::max() ? std::numeric_limits<T>::max() : val;
    }

    parser::Program const *program_ {};
    uint32_t startLine_ {};
    uint32_t endLine_ {};
    uint32_t startIndex_ {};
    uint32_t endIndex_ {};
};

class SourceLocation {
public:
    explicit SourceLocation() noexcept = default;
    explicit SourceLocation(size_t l, size_t c, const parser::Program *prog) noexcept : line(l), col(c), program_(prog)
    {
    }
    DEFAULT_COPY_SEMANTIC(SourceLocation);
    DEFAULT_MOVE_SEMANTIC(SourceLocation);
    ~SourceLocation() = default;

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    size_t line {};
    size_t col {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    const parser::Program *Program() const;

private:
    const parser::Program *program_ {};
};

class Range {
public:
    explicit Range(size_t bS) noexcept : byteSize(bS) {}

    DEFAULT_COPY_SEMANTIC(Range);
    DEFAULT_MOVE_SEMANTIC(Range);
    ~Range() = default;

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    size_t byteSize {};
    size_t cnt {1};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class OffsetEntry {
public:
    explicit OffsetEntry(size_t l) : lineStart(l), offset_(l) {};

    DEFAULT_COPY_SEMANTIC(OffsetEntry);
    DEFAULT_MOVE_SEMANTIC(OffsetEntry);
    ~OffsetEntry() = default;

    void AddCol(size_t offset);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::vector<Range> ranges {};
    size_t lineStart {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    size_t offset_ {};
};

class LineIndex {
public:
    explicit LineIndex(const util::StringView &source) noexcept;
    NO_COPY_SEMANTIC(LineIndex);
    NO_MOVE_SEMANTIC(LineIndex);
    ~LineIndex() = default;

    SourceLocation GetLocation(SourcePosition pos) const noexcept;
    size_t GetOffset(SourceLocation loc) const noexcept;

private:
    std::vector<OffsetEntry> entries_;
};
}  // namespace ark::es2panda::lexer

#endif
