/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd. All rights reserved.
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

#ifndef ES2PANDA_IR_SRCDUMP_H
#define ES2PANDA_IR_SRCDUMP_H

#include <ir/astNode.h>
#include <lexer/token/sourceLocation.h>
#include "generated/tokenType.h"
#include <util/ustring.h>

#include <sstream>
#include <variant>
#include <future>

namespace ark::es2panda::ir {

// Forward declarations
class ClassDefinition;
class TSTypeAliasDeclaration;
class ClassProperty;
class TSInterfaceDeclaration;
class TSEnumDeclaration;

using NodeVariant =
    std::variant<std::monostate, const ark::es2panda::ir::ClassDefinition *,
                 const ark::es2panda::ir::TSTypeAliasDeclaration *, const ark::es2panda::ir::ClassProperty *,
                 const ark::es2panda::ir::TSInterfaceDeclaration *, const ark::es2panda::ir::TSEnumDeclaration *>;

class SrcDumper {
public:
    explicit SrcDumper(const ir::AstNode *node);
    explicit SrcDumper(const ir::AstNode *node, bool isDeclgen);

    void Add(const std::string &str);
    void Add(int8_t i);
    void Add(int16_t i);
    void Add(int32_t i);
    void Add(int64_t l);
    void Add(float f);
    void Add(double d);

    std::string Str() const
    {
        return ss_.str();
    }

    void IncrIndent();
    void DecrIndent();
    void Endl(size_t num = 1);

    bool IsDeclgen() const;
    void DumpVariant(NodeVariant &node);
    void DumpNode(const std::string &key);

    template <typename T>
    void DumpNodeIfPointer(T *value);

    template <typename T>
    void AddNode(const std::string &key, T *value)
    {
        unExportNode_[key] = NodeVariant(value);
    }

    void RemoveNode(const std::string &key)
    {
        unExportNode_.erase(key);
    }

    bool IsIndirectDepPhase() const
    {
        return isIndirectDepPhase_;
    }

    template <typename T>
    void PushTask(T &&task)
    {
        taskQueue_.emplace(std::forward<T>(task));
    }

    void Run();

private:
    std::stringstream ss_;
    std::string indent_;
    bool isDeclgen_ = false;
    bool isIndirectDepPhase_ = false;
    std::unordered_map<std::string, NodeVariant> unExportNode_;

    std::queue<std::function<void()>> taskQueue_;
};
}  // namespace ark::es2panda::ir

#endif  // ES2PANDA_IR_SRCDUMP_H
