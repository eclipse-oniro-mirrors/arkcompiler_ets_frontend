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
#include "public/public.h"

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

SrcDumper::SrcDumper(const ir::AstNode *node, bool isDeclgen, bool enableJsdocDump) : isDeclgen_(isDeclgen)
{
    if (enableJsdocDump) {
        jsdocGetter_ = std::make_unique<parser::JsdocHelper>(node);
    }
    node->Dump(this);
}

void SrcDumper::IncrIndent()
{
    indent_ += "  ";
}

void SrcDumper::DecrIndent()
{
    ES2PANDA_ASSERT(indent_.size() >= 2U);
    indent_.resize(indent_.size() - 2U);
}

// NOTE: `num` argument is unsed, should be deleted once bindings are no longer hardcoded (never?)
void SrcDumper::Endl([[maybe_unused]] size_t num)
{
    ss_ << std::endl;
    ss_ << indent_;
}

static bool OnlySpaces(const std::string &s)
{
    for (char c : s) {
        if (!std::isspace(c)) {
            return false;
        }
    }
    return true;
}
static std::stringstream NormalizeStream(std::stringstream &iss)
{
    std::stringstream oss;
    std::string line;
    bool lastWasEmpty = false;
    while (std::getline(iss, line)) {
        if (OnlySpaces(line)) {
            if (!lastWasEmpty) {
                oss << std::endl;
                lastWasEmpty = true;
            }
        } else {
            oss << line << std::endl;
            lastWasEmpty = false;
        }
    }
    return oss;
}

std::string SrcDumper::Str() const
{
    // NOTE: at first place we should not print extra indentations and newlines
    // NOTE: should we normalize for all calls?
    if (IsDeclgen()) {
        std::stringstream ss;
        // NOTE: copy stream to save constness of Str() method (used in plugins)
        ss << ss_.rdbuf();
        ss = NormalizeStream(ss);
        return ss.str();
    }
    return ss_.str();
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

void SrcDumper::DumpJsdocBeforeTargetNode(const ir::AstNode *inputNode)
{
    if (!jsdocGetter_) {
        return;
    }
    jsdocGetter_->InitNode(inputNode);
    auto resJsdoc = jsdocGetter_->GetJsdocBackward();
    if (!resJsdoc.Empty()) {
        ss_ << resJsdoc.Mutf8();
        ss_ << std::endl;
        auto parent = inputNode->Parent();
        if (inputNode->IsClassDefinition() || parent->IsClassDefinition() || parent->IsTSInterfaceBody() ||
            parent->IsScriptFunction()) {
            parent = parent->Parent();
        }
        while (!parent->IsETSModule() || (parent->IsETSModule() && parent->AsETSModule()->IsNamespace())) {
            ss_ << "  ";
            parent = parent->Parent();
            if (parent == nullptr) {
                break;
            }
        }
    }
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
