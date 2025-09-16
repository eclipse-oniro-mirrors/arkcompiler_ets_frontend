/*
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

#include "srcDump.h"

#include <ir/astNode.h>
#include <ir/base/classDefinition.h>
#include <ir/base/classProperty.h>
#include <ir/ts/tsEnumDeclaration.h>
#include <ir/ts/tsInterfaceDeclaration.h>
#include <ir/ts/tsTypeAliasDeclaration.h>
#include <cmath>
#include <iostream>

namespace ark::es2panda::ir {

SrcDumper::SrcDumper(const ir::AstNode *node)
{
    node->Dump(this);
}

SrcDumper::SrcDumper(const ir::AstNode *node, bool isDeclgen) : isDeclgen_(isDeclgen)
{
    node->Dump(this);
}

void SrcDumper::IncrIndent()
{
    indent_.push_back(' ');
    indent_.push_back(' ');
}

void SrcDumper::DecrIndent()
{
    if (indent_.size() >= 2U) {
        indent_.pop_back();
        indent_.pop_back();
    }
}

void SrcDumper::Endl(size_t num)
{
    while (num != 0U) {
        ss_ << std::endl;
        --num;
    }
    ss_ << indent_;
}

void SrcDumper::Add(const std::string &str)
{
    ss_ << str;
}

void SrcDumper::Add(int8_t i)
{
    ss_ << static_cast<int32_t>(i);
}

void SrcDumper::Add(int16_t i)
{
    ss_ << i;
}

void SrcDumper::Add(int32_t i)
{
    ss_ << i;
}

void SrcDumper::Add(int64_t l)
{
    ss_ << l;
}

void SrcDumper::Add(float f)
{
    ss_ << f;
}

void SrcDumper::Add(double d)
{
    ss_ << d;
}

bool SrcDumper::IsDeclgen() const
{
    return isDeclgen_;
}

void SrcDumper::DumpVariant(NodeVariant &node)
{
    std::visit(
        [this](auto &&value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                if constexpr (std::is_pointer_v<T>) {
                    DumpNodeIfPointer(value);
                }
            }
        },
        node);
}

template <typename T>
void SrcDumper::DumpNodeIfPointer(T *value)
{
    if (value) {
        isIndirectDepPhase_ = true;
        value->Dump(this);
        isIndirectDepPhase_ = false;
    }
}

void SrcDumper::DumpNode(const std::string &key)
{
    auto it = unExportNode_.find(key);
    if (it == unExportNode_.end()) {
        return;
    }

    NodeVariant node = it->second;
    unExportNode_.erase(it);

    DumpVariant(node);
}

void SrcDumper::Run()
{
    while (!taskQueue_.empty()) {
        auto task = std::move(taskQueue_.front());
        taskQueue_.pop();
        task();
    }
}

}  // namespace ark::es2panda::ir
