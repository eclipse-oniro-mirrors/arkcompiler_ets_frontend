/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "get_class_property_info.h"
#include "compiler/lowering/util.h"
#include "libarkbase/macros.h"

namespace ark::es2panda::lsp {

std::vector<std::string> GeneratePropertyModifiers(const ir::ClassProperty *property)
{
    std::vector<std::string> modifiers;

    if (property->IsPublic()) {
        modifiers.emplace_back("public");
    } else if (property->IsPrivate()) {
        modifiers.emplace_back("private");
    } else if (property->IsProtected()) {
        modifiers.emplace_back("protected");
    }

    if (property->IsStatic()) {
        modifiers.emplace_back("static");
    }

    if (property->IsReadonly()) {
        modifiers.emplace_back("readonly");
    }

    return modifiers;
}

void CollectClassProperties(const ir::AstNode *classNode, std::vector<FieldsInfo> &result)
{
    if (classNode == nullptr || !classNode->IsClassDeclaration()) {
        return;
    }

    FieldsInfo classInfo;
    classInfo.name = GetIdentifierName(const_cast<ir::AstNode *>(classNode));

    auto classBody = classNode->AsClassDeclaration()->Definition()->Body();
    for (auto node : classBody) {
        if (!node->IsClassProperty()) {
            continue;
        }

        auto property = node->AsClassProperty();
        auto modifiers = GeneratePropertyModifiers(property);
        std::optional<std::vector<std::string>> modifiersOpt(modifiers);

        std::string name = GetIdentifierName(property);

        constexpr auto K_PROPERTY_PREFIX = "<property>";
        constexpr std::size_t K_PROPERTY_PREFIX_LENGTH = std::char_traits<char>::length(K_PROPERTY_PREFIX);
        if (name.size() >= K_PROPERTY_PREFIX_LENGTH &&
            name.compare(0, K_PROPERTY_PREFIX_LENGTH, K_PROPERTY_PREFIX) == 0) {
            name.erase(0, K_PROPERTY_PREFIX_LENGTH);
        }

        FieldListProperty propertyInfo("classField", std::move(modifiersOpt), name, property->Start().index,
                                       property->End().index);

        classInfo.properties.push_back(propertyInfo);
    }

    result.push_back(classInfo);
}

void CollectInheritedProperties(const ir::AstNode *classNode, std::vector<FieldsInfo> &result)
{
    while (classNode != nullptr) {
        auto superClass = ark::es2panda::lsp::GetEffectiveBaseTypeNode(classNode);
        if (superClass != nullptr) {
            CollectClassProperties(superClass, result);
        }
        classNode = superClass;
    }
}

std::vector<FieldsInfo> GetClassPropertyInfo(es2panda_Context *context, size_t pos, bool shouldCollectInherited)
{
    std::vector<FieldsInfo> result;
    auto classNode = ark::es2panda::lsp::GetTargetDeclarationNodeByPosition(context, pos);
    if (classNode == nullptr) {
        return result;
    }
    CollectClassProperties(classNode, result);
    if (shouldCollectInherited) {
        CollectInheritedProperties(classNode, result);
    }
    return result;
}
}  // namespace ark::es2panda::lsp