/*
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

#include "enumPropertiesInAnnotationsLowering.h"
#include "checker/types/ets/etsArrayType.h"
#include "checker/types/ets/etsEnumType.h"
#include "ir/base/classProperty.h"

namespace ark::es2panda::compiler {

static void TransformEnumArrayRecursively(checker::ETSArrayType *propType)
{
    if (propType->ElementType()->IsETSEnumType()) {
        auto newElemType = propType->ElementType()->AsETSEnumType()->Underlying();
        propType->SetElementType(newElemType);
        return;
    }
    if (propType->ElementType()->IsETSArrayType()) {
        TransformEnumArrayRecursively(propType->ElementType()->AsETSArrayType());
    }
}

static void SetValueType(ir::Expression *value, checker::Type *newType)
{
    if (value->Variable() != nullptr) {
        value->Variable()->SetTsType(newType);
    }
    value->SetTsType(newType);
    if (newType->IsETSArrayType() && newType->AsETSArrayType()->ElementType()->IsETSArrayType()) {
        for (auto elem : value->AsArrayExpression()->Elements()) {
            SetValueType(elem, newType->AsETSArrayType()->ElementType());
        }
    }
}

static void TransformEnumToUnderlying(ir::ClassProperty *prop, checker::Checker *checker)
{
    checker::Type *propType = prop->TsType();
    checker::Type *newPropType {};
    if (propType->IsETSEnumType()) {
        prop->SetTypeAnnotation(nullptr);
        newPropType = propType->AsETSEnumType()->Underlying();
    } else if (propType->IsETSArrayType()) {
        prop->SetTypeAnnotation(nullptr);
        newPropType = propType->Clone(checker);
        TransformEnumArrayRecursively(newPropType->AsETSArrayType());
    } else {
        return;
    }
    prop->SetTsType(newPropType);
    prop->Key()->SetTsType(newPropType);
    prop->Key()->Variable()->SetTsType(newPropType);
    if (prop->Value() != nullptr) {
        SetValueType(prop->Value(), newPropType);
    }
}

bool EnumPropertiesInAnnotationsLoweringPhase::PerformForModule([[maybe_unused]] public_lib::Context *ctx,
                                                                parser::Program *program)
{
    auto *checker = ctx->GetChecker();
    program->Ast()->IterateRecursively([checker](auto *node) {
        if (node->IsAnnotationDeclaration() || node->IsAnnotationUsage()) {
            node->Iterate([checker](auto *child) {
                child->IsClassProperty() ? TransformEnumToUnderlying(child->AsClassProperty(), checker) : void();
            });
        }
    });
    return true;
}

}  // namespace ark::es2panda::compiler
