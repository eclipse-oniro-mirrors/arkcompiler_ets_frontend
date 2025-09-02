/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_LSP_CLASS_HIERARCHIES_INFO_H
#define ES2PANDA_LSP_CLASS_HIERARCHIES_INFO_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

enum class ClassRelationKind { UNKNOWN, INTERFACE, CLASS, FIELD, METHOD, PROPERTY };

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct ClassRelationDetails {
    ClassRelationDetails() = default;
    ClassRelationDetails(std::string f, size_t p, ClassRelationKind k) : fileName(std::move(f)), pos(p), kind(k) {}
    std::string fileName;
    size_t pos = 0;
    ClassRelationKind kind = ClassRelationKind::UNKNOWN;
};

struct ClassHierarchyItemInfo {
    ClassHierarchyItemInfo() = default;
    ClassHierarchyItemInfo(std::string d, ClassRelationKind k, size_t p) : pos(p), kind(k), description(std::move(d)) {}
    size_t pos = 0;
    ClassRelationKind kind = ClassRelationKind::UNKNOWN;
    std::vector<ClassRelationDetails> overridden;
    std::vector<ClassRelationDetails> overriding;
    std::vector<ClassRelationDetails> implemented;
    std::vector<ClassRelationDetails> implementing;
    std::string description;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

/**
 * @brief Returns class/interface hierarchys at a specific source position.
 *
 * @param context Parsing context for AST access.
 * @param fileName Source file name.
 * @param pos Character offset in source code.
 */
std::vector<ClassHierarchyItemInfo> GetClassHierarchiesImpl(std::vector<es2panda_Context *> *contextList,
                                                            const std::string &fileName, size_t pos);
}  // namespace ark::es2panda::lsp

#endif