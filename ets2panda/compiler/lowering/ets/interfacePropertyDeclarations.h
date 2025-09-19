/*
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

#ifndef ES2PANDA_COMPILER_LOWERING_INTERFACE_PROP_DECL_H
#define ES2PANDA_COMPILER_LOWERING_INTERFACE_PROP_DECL_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

using InterfacePropertyType = std::unordered_set<std::string>;
using InterfacePropertyMapType = std::unordered_map<std::string, InterfacePropertyType>;
using VisitedInterfacesOfClass = std::unordered_set<std::string>;

class OptionalInterfacePropertyCollector {
public:
    OptionalInterfacePropertyCollector() = default;
    ~OptionalInterfacePropertyCollector() = default;

    NO_MOVE_SEMANTIC(OptionalInterfacePropertyCollector);
    NO_COPY_SEMANTIC(OptionalInterfacePropertyCollector);

    std::string &GetInterfaceId()
    {
        return interfaceId_;
    }

    void SetInterfaceId(std::string id)
    {
        interfaceId_ = std::move(id);
    }

    InterfacePropertyType &GetInterfaceProperty(const std::string &id)
    {
        ES2PANDA_ASSERT(interfaceProperties_.count(id) != 0);
        return interfaceProperties_[id];
    }

    void InsertInterfaceProperty(const std::string &property)
    {
        ES2PANDA_ASSERT(!interfaceId_.empty() && (interfaceProperties_.count(interfaceId_) != 0U));
        interfaceProperties_[interfaceId_].insert(property);
    }

    void InitInterfacePropertyMap()
    {
        ES2PANDA_ASSERT(!interfaceId_.empty());
        interfaceProperties_.insert({interfaceId_, {}});
    }

    bool IsInterfaceHasProperty(const std::string &interId)
    {
        return interfaceProperties_.count(interId) != 0U;
    }

    InterfacePropertyType &GetInterfaceParent(const std::string &id)
    {
        ES2PANDA_ASSERT(interfaceParents_.count(id) != 0);
        return interfaceParents_[id];
    }

    void InsertInterfaceParent(const std::string &parent)
    {
        ES2PANDA_ASSERT(!interfaceId_.empty() && (interfaceParents_.count(interfaceId_) != 0U));
        interfaceParents_[interfaceId_].insert(parent);
    }

    void InitInterfaceParentMap()
    {
        ES2PANDA_ASSERT(!interfaceId_.empty());
        interfaceParents_.insert({interfaceId_, {}});
    }

    bool IsParentExists(const std::string &interId)
    {
        return interfaceParents_.count(interId) != 0U;
    }

    bool IsVisitedInterface(const std::string &interId)
    {
        return !visitedInterfaces_.insert(interId).second;
    }

    void InitVisitedInterfaces()
    {
        visitedInterfaces_.clear();
    }

private:
    std::string interfaceId_ {};
    InterfacePropertyMapType interfaceProperties_ {};
    InterfacePropertyMapType interfaceParents_ {};
    VisitedInterfacesOfClass visitedInterfaces_ {};
};

class InterfacePropertyDeclarationsPhase : public PhaseForDeclarations {
public:
    std::string_view Name() const override
    {
        return "InterfacePropertyDeclarationsPhase";
    }

    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;

private:
    OptionalInterfacePropertyCollector &GetPropCollector()
    {
        return propCollector_;
    }

    void TransformOptionalFieldTypeAnnotation(public_lib::Context *ctx, ir::ClassProperty *const field,
                                              bool isInterface = false);

    ir::FunctionSignature GenerateGetterOrSetterSignature(public_lib::Context *ctx, varbinder::ETSBinder *varbinder,
                                                          ir::ClassProperty *const field, bool isSetter,
                                                          varbinder::FunctionParamScope *paramScope);

    ir::MethodDefinition *GenerateGetterOrSetter(public_lib::Context *ctx, varbinder::ETSBinder *varbinder,
                                                 ir::ClassProperty *const field, bool isSetter, bool isOptional);

    void CollectPropertiesAndSuperInterfaces(ir::TSInterfaceBody *const interface);

    void HandleInternalGetterOrSetterMethod(ir::AstNode *const ast);

    ir::Expression *UpdateInterfaceProperties(public_lib::Context *ctx, varbinder::ETSBinder *varbinder,
                                              ir::TSInterfaceBody *const interface);

    void CollectSuperInterfaceProperties(InterfacePropertyType &implInterfaceProperties, const std::string &interId);

    void UpdateClassProperties(public_lib::Context *ctx, ir::ClassDefinition *const klass);

    ir::AstNode *GenerateGetterOrSetterBodyForOptional(public_lib::Context *ctx, bool isSetter, bool isOptional);

private:
    OptionalInterfacePropertyCollector propCollector_ {};
};

}  // namespace ark::es2panda::compiler

#endif
