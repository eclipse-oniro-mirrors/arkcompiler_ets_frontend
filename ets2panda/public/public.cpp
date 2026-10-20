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

#include "public/public.h"
#include "compiler/lowering/phase.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ark::es2panda::public_lib {

static constexpr size_t POINTER_HASH_MULTIPLIER = 31U;
static constexpr size_t EMPTY_ARRAY_CAPACITY = 1U;

template <typename T>
struct ApiResultPointerSnapshot {
    size_t count;
    std::vector<T> values;
};

template <typename T>
using ApiResultPointerSnapshots = std::unordered_multimap<size_t, ApiResultPointerSnapshot<T>>;

template <typename T>
static T *FindOrCopyPointers(ApiResultPointerSnapshots<T> &arrays, ApiResultStorage::PointerHashFn hashFn, size_t count,
                             const void *source, ApiResultStorage::PointerReader<T> read)
{
    size_t hash = count;
    for (size_t i = 0; i < count; ++i) {
        hash = hash * POINTER_HASH_MULTIPLIER + std::hash<T> {}(read(source, i));
    }
    if (hashFn != nullptr) {
        hash = hashFn(hash);
    }
    auto [begin, end] = arrays.equal_range(hash);
    for (auto it = begin; it != end; ++it) {
        auto &candidate = it->second;
        // Logical length is separate from the nonempty backing storage used
        // for empty results. An empty array must not match {nullptr}.
        if (candidate.count != count) {
            continue;
        }
        bool same = true;
        for (size_t i = 0; i < count; ++i) {
            if (candidate.values[i] != read(source, i)) {
                same = false;
                break;
            }
        }
        if (same) {
            return candidate.values.data();
        }
    }
    std::vector<T> result(count == 0 ? EMPTY_ARRAY_CAPACITY : count);
    for (size_t i = 0; i < count; ++i) {
        result[i] = read(source, i);
    }
    return arrays.emplace(hash, ApiResultPointerSnapshot<T> {count, std::move(result)})->second.values.data();
}

struct ApiResultStorage::Impl {
    std::unordered_multimap<size_t, std::string> strings;
    ApiResultPointerSnapshots<char *> stringsArray;
    ApiResultPointerSnapshots<es2panda_AstNode *> nodes;
    ApiResultPointerSnapshots<es2panda_Type *> types;
    ApiResultPointerSnapshots<es2panda_Signature *> signatures;
    ApiResultPointerSnapshots<es2panda_Variable *> variables;
    ApiResultPointerSnapshots<es2panda_Declaration *> declarations;
    ApiResultPointerSnapshots<es2panda_Scope *> scopes;
};

ApiResultStorage::ApiResultStorage(StringHashFn stringHash, PointerHashFn pointerHash)
    : stringHash_(stringHash), pointerHash_(pointerHash)
{
}

ApiResultStorage::~ApiResultStorage() = default;

ApiResultStorage::Impl &ApiResultStorage::GetImpl()
{
    if (impl_ == nullptr) {
        impl_ = std::make_unique<Impl>();
    }
    return *impl_;
}

char *ApiResultStorage::String(std::string_view value)
{
    auto &storage = GetImpl();
    const auto hash = stringHash_ != nullptr ? stringHash_(value) : std::hash<std::string_view> {}(value);
    auto [begin, end] = storage.strings.equal_range(hash);
    for (auto it = begin; it != end; ++it) {
        if (std::string_view(it->second) == value) {
            return it->second.data();
        }
    }
    return storage.strings.emplace(hash, value)->second.data();
}

char **ApiResultStorage::Pointers(size_t count, const void *source, PointerReader<char *> read)
{
    return FindOrCopyPointers(GetImpl().stringsArray, pointerHash_, count, source, read);
}

es2panda_AstNode **ApiResultStorage::Pointers(size_t count, const void *source, PointerReader<es2panda_AstNode *> read)
{
    return FindOrCopyPointers(GetImpl().nodes, pointerHash_, count, source, read);
}

es2panda_Type **ApiResultStorage::Pointers(size_t count, const void *source, PointerReader<es2panda_Type *> read)
{
    return FindOrCopyPointers(GetImpl().types, pointerHash_, count, source, read);
}

es2panda_Signature **ApiResultStorage::Pointers(size_t count, const void *source,
                                                PointerReader<es2panda_Signature *> read)
{
    return FindOrCopyPointers(GetImpl().signatures, pointerHash_, count, source, read);
}

es2panda_Variable **ApiResultStorage::Pointers(size_t count, const void *source,
                                               PointerReader<es2panda_Variable *> read)
{
    return FindOrCopyPointers(GetImpl().variables, pointerHash_, count, source, read);
}

es2panda_Declaration **ApiResultStorage::Pointers(size_t count, const void *source,
                                                  PointerReader<es2panda_Declaration *> read)
{
    return FindOrCopyPointers(GetImpl().declarations, pointerHash_, count, source, read);
}

es2panda_Scope **ApiResultStorage::Pointers(size_t count, const void *source, PointerReader<es2panda_Scope *> read)
{
    return FindOrCopyPointers(GetImpl().scopes, pointerHash_, count, source, read);
}

void ApiResultStorage::Clear()
{
    impl_.reset();
}

// Out-of-line: keeps apiResults construction/destruction inside es2panda-lib.
Context::Context() = default;
Context::~Context() = default;

checker::Checker *Context::GetChecker() const
{
    return checkers_[compiler::GetPhaseManager()->GetCurrentMajor()];
}

checker::SemanticAnalyzer *Context::GetAnalyzer() const
{
    return analyzers_[compiler::GetPhaseManager()->GetCurrentMajor()];
}

}  // namespace ark::es2panda::public_lib
