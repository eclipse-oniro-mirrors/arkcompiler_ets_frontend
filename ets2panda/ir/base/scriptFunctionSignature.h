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

#ifndef ES2PANDA_COMPILER_CORE_SCRIPT_FUNCTION_SIGNATURE_H
#define ES2PANDA_COMPILER_CORE_SCRIPT_FUNCTION_SIGNATURE_H

#include "ir/expression.h"

#include <ir/ets/etsParameterExpression.h>

namespace ark::es2panda::ir {
class TSTypeParameterDeclaration;
class TypeNode;
class ScriptFunction;
class ETSFunctionType;

class FunctionSignature {
public:
    explicit FunctionSignature(TSTypeParameterDeclaration *typeParams, ArenaVector<ir::Expression *> &&params,
                               TypeNode *returnTypeAnnotation, bool hasReceiver = false)
        : typeParams_(typeParams),
          params_(std::move(params)),
          returnTypeAnnotation_(returnTypeAnnotation),
          hasReceiver_(hasReceiver)
    {
    }

    FunctionSignature() = delete;
    ~FunctionSignature() = default;
    NO_COPY_SEMANTIC(FunctionSignature);
    DEFAULT_MOVE_SEMANTIC(FunctionSignature);

    [[nodiscard]] const ArenaVector<ir::Expression *> &Params() const noexcept
    {
        return params_;
    }

    [[nodiscard]] ArenaVector<ir::Expression *> &Params() noexcept
    {
        return params_;
    }

    [[nodiscard]] TSTypeParameterDeclaration *TypeParams() noexcept
    {
        return typeParams_;
    }

    [[nodiscard]] const TSTypeParameterDeclaration *TypeParams() const noexcept
    {
        return typeParams_;
    }

    void SetTypeParams(TSTypeParameterDeclaration *typeParams) noexcept
    {
        typeParams_ = typeParams;
    }

    [[nodiscard]] TypeNode *ReturnType() noexcept
    {
        return returnTypeAnnotation_;
    }

    void SetReturnType(TypeNode *type) noexcept
    {
        returnTypeAnnotation_ = type;
    }

    [[nodiscard]] const TypeNode *ReturnType() const noexcept
    {
        return returnTypeAnnotation_;
    }

    void Iterate(const NodeTraverser &cb) const;
    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName);

    [[nodiscard]] FunctionSignature Clone(ArenaAllocator *allocator);

    [[nodiscard]] bool HasReceiver() const noexcept
    {
        return hasReceiver_;
    }

private:
    TSTypeParameterDeclaration *typeParams_;
    ArenaVector<ir::Expression *> params_;
    TypeNode *returnTypeAnnotation_;
    bool hasReceiver_;

    friend class ScriptFunction;
    friend class ETSFunctionType;
    void CopyFrom(const FunctionSignature &other)
    {
        typeParams_ = other.typeParams_;
        params_ = other.params_;
        returnTypeAnnotation_ = other.returnTypeAnnotation_;
        hasReceiver_ = other.hasReceiver_;
    }
};

}  // namespace ark::es2panda::ir

#endif
