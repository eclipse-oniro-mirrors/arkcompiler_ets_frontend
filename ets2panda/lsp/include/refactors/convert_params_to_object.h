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

#ifndef CONVERT_PARAMS_TO_OBJECT_H
#define CONVERT_PARAMS_TO_OBJECT_H

#include "refactor_types.h"
#include <vector>
#include <string>

namespace ark::es2panda::ir {
class MethodDefinition;
class ScriptFunction;
class ETSParameterExpression;
class TypeNode;
class CallExpression;
class Expression;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::lsp {
class ChangeTracker;

constexpr RefactorActionView CONVERT_PARAMS_TO_OBJECT_ACTION {
    refactor_name::CONVERT_PARAMS_TO_OBJECT, refactor_description::CONVERT_PARAMS_TO_OBJECT_DESC,
    "refactor.rewrite.parameters.toObject.introduceInterface"};

struct RefactorParameterInfo {
private:
    std::string name_;
    ir::TypeNode *typeAnnotation_ {nullptr};
    bool isOptional_ {false};
    ir::Expression *defaultValue_ {nullptr};

public:
    RefactorParameterInfo() = default;

    std::string Name() const
    {
        return name_;
    }
    void SetName(const std::string &name)
    {
        name_ = name;
    }

    ir::TypeNode *TypeAnnotation() const
    {
        return typeAnnotation_;
    }
    void SetTypeAnnotation(ir::TypeNode *typeAnnotation)
    {
        typeAnnotation_ = typeAnnotation;
    }

    bool IsOptional() const
    {
        return isOptional_;
    }
    void SetIsOptional(bool isOptional)
    {
        isOptional_ = isOptional;
    }

    ir::Expression *DefaultValue() const
    {
        return defaultValue_;
    }
    void SetDefaultValue(ir::Expression *defaultValue)
    {
        defaultValue_ = defaultValue;
    }
};

struct RefactorFunctionInfo {
private:
    ir::MethodDefinition *methodDef_ {nullptr};
    std::string functionName_;
    bool hasThisParameter_ {false};
    std::vector<RefactorParameterInfo> parameters_;

public:
    RefactorFunctionInfo() = default;

    ir::MethodDefinition *MethodDef() const
    {
        return methodDef_;
    }
    void SetMethodDef(ir::MethodDefinition *methodDef)
    {
        methodDef_ = methodDef;
    }

    std::string FunctionName() const
    {
        return functionName_;
    }
    void SetFunctionName(const std::string &functionName)
    {
        functionName_ = functionName;
    }

    bool HasThisParameter() const
    {
        return hasThisParameter_;
    }
    void SetHasThisParameter(bool hasThisParameter)
    {
        hasThisParameter_ = hasThisParameter;
    }

    const std::vector<RefactorParameterInfo> &Parameters() const
    {
        return parameters_;
    }
    std::vector<RefactorParameterInfo> &Parameters()
    {
        return parameters_;
    }
    void SetParameters(const std::vector<RefactorParameterInfo> &parameters)
    {
        parameters_ = parameters;
    }
};

class ConvertParamsToObjectRefactor : public Refactor {
public:
    ConvertParamsToObjectRefactor();
    std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};

}  // namespace ark::es2panda::lsp

#endif  // CONVERT_PARAMS_TO_OBJECT_H
