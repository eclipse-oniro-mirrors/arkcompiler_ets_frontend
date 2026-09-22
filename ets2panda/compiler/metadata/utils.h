/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_METADATA_UTILS_H
#define ES2PANDA_COMPILER_METADATA_UTILS_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

inline std::string IrDeclToString(const ir::AstNode *node)
{
    ir::SrcDumper dumper;
    node->Dump(&dumper);
    return dumper.Str();
}

template <typename T>
std::string IrDeclVectorToString(const ArenaVector<T *> &nodes)
{
    std::string str;
    size_t i = 0;
    for (const auto &node : nodes) {
        str += IrDeclToString(node);
        if (++i != nodes.size()) {
            str += ", ";
        }
    }
    return str;
}

inline std::string GetDeclKindToLog(const Metadata::ClassDecl *fbClassDecl)
{
    if (fbClassDecl->enum_kind() != Metadata::EnumKind_NONE) {
        return "enum ";
    }
    if (fbClassDecl->is_namespace()) {
        return "namespace ";
    }
    return "class ";
}

inline std::string GetDeclKindToLog(const ir::ClassDefinition *astDecl)
{
    if (astDecl->IsEnumTransformed()) {
        return "enum ";
    }
    if (astDecl->IsNamespaceTransformed()) {
        return "namespace ";
    }
    return "class ";
}

inline size_t CalculateMetadataSize(panda_file::MetadataByModules *metadata)
{
    size_t totalSize = 0;
    for (const auto &[moduleName, moduleMetadata] : *metadata) {
        totalSize += moduleMetadata.size();
    }
    return totalSize;
}

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_METADATA_UTILS_H