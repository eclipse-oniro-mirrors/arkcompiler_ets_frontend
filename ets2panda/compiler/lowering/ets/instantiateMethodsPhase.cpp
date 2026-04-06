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

/*
  Before UnboxLowering modifies method signatures, we need to ensure that
  all classes have their methods instantiated; otherwise overriding checks
  may fail when comparing methods with modified signatures to original ones.
 */

#include "compiler/lowering/ets/instantiateMethodsPhase.h"

namespace ark::es2panda::compiler {

static void InstantiateTypeHierarchyMembers(std::unordered_set<uint64_t> *alreadySeen,
                                            checker::ETSObjectType const *objTp)
{
    // Avoid cycles: if we've already normalized this type's hierarchy, skip it
    if (alreadySeen->find(objTp->Id()) != alreadySeen->end()) {
        return;
    }
    alreadySeen->insert(objTp->Id());

    // First, recursively instantiate superclass members
    if (auto const *superType = objTp->SuperType(); superType != nullptr) {
        InstantiateTypeHierarchyMembers(alreadySeen, superType);
    }

    // Then instantiate interface members
    for (auto const *interface : objTp->Interfaces()) {
        InstantiateTypeHierarchyMembers(alreadySeen, interface);
    }

    // This ensures that CheckAndInstantiateProperties is called
    (void)objTp->InstanceMethods();
}

void InstantiatePropertiesReferredFromChild(checker::ETSChecker *checker, std::unordered_set<uint64_t> *alreadySeen,
                                            ir::AstNode *child)
{
    if (child->IsExpression() && child->AsExpression()->IsTypeNode()) {
        // Avoid dealing with annotation usages.
        // ETSTypeReferenceParts only appear within ETSTypeReference, the only way to get one is
        // again through AnnotationUsage (since it wasn't replaced by OpaqueTypeNode).
        if (child->Parent()->IsAnnotationUsage() || child->IsETSTypeReferencePart()) {
            return;
        }

        auto typeNodeType = child->AsExpression()->AsTypeNode()->GetType(checker);
        ES2PANDA_ASSERT(typeNodeType != nullptr);
        if (typeNodeType == nullptr) {
            return;
        }

        typeNodeType->IterateRecursively([alreadySeen](checker::Type const *tp) {
            if (tp->IsETSObjectType()) {
                InstantiateTypeHierarchyMembers(alreadySeen, tp->AsETSObjectType());
            }
        });
    } else if (child->IsTyped()) {
        auto const *type = child->AsTyped()->TsType();
        if (type == nullptr) {
            return;
        }
        type->IterateRecursively([alreadySeen](checker::Type const *tp) {
            if (tp->IsETSObjectType()) {
                InstantiateTypeHierarchyMembers(alreadySeen, tp->AsETSObjectType());
            }
        });
    }
}

// Make sure that all properties of all mentioned types are instantiated, across the whole type hierarchy.
// This ensures that we don't run afoul of the overriding checks.
static void InstantiateAllProperties(checker::ETSChecker *checker, ir::AstNode *ast)
{
    std::unordered_set<uint64_t> alreadySeen;
    ast->IterateRecursively([checker, &alreadySeen](ir::AstNode *child) {
        InstantiatePropertiesReferredFromChild(checker, &alreadySeen, child);
    });
}

bool InstantiateMethodsPhase::PerformForProgram(parser::Program *program)
{
    InstantiateAllProperties(Context()->GetChecker()->AsETSChecker(), program->Ast());
    return true;
}

}  // namespace ark::es2panda::compiler
