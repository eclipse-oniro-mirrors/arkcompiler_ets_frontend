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

#ifndef RECURSIVE_GUARD_H
#define RECURSIVE_GUARD_H

namespace ark::es2panda::parser {

constexpr unsigned int MAX_RECURSION_DEPTH = 1024;

struct RecursiveContext {
    unsigned depth = 0;
};

class TrackRecursive {
public:
    explicit TrackRecursive(RecursiveContext &recursivecontext) : recursivecontext_(recursivecontext)
    {
        ++recursivecontext_.depth;
        valid_ = recursivecontext_.depth <= MAX_RECURSION_DEPTH;
    };

    ~TrackRecursive()
    {
        --recursivecontext_.depth;
    }

    TrackRecursive(const TrackRecursive &) = delete;
    TrackRecursive(TrackRecursive &&) = delete;
    TrackRecursive &operator=(const TrackRecursive &) = delete;
    TrackRecursive &operator=(TrackRecursive &&) = delete;

    explicit operator bool() const
    {
        return valid_;
    }

private:
    RecursiveContext &recursivecontext_;
    bool valid_ = true;
};

class WhileLoopGuard {
public:
    explicit WhileLoopGuard(unsigned maxIterations = MAX_RECURSION_DEPTH) : maxIterations_(maxIterations) {}

    bool ShouldContinue()
    {
        return ++currentIterations_ <= maxIterations_;
    }

    bool IsLimitReached() const
    {
        return currentIterations_ >= maxIterations_;
    }

    unsigned GetCurrentIterations() const
    {
        return currentIterations_;
    }

    unsigned GetMaxIterations() const
    {
        return maxIterations_;
    }

    void Reset()
    {
        currentIterations_ = 0;
    }

    void SetMaxIterations(unsigned maxIterations)
    {
        maxIterations_ = maxIterations;
    }

private:
    unsigned maxIterations_;
    unsigned currentIterations_ {0};
};

}  // namespace ark::es2panda::parser

#endif  // UTIL_GUARD_H
