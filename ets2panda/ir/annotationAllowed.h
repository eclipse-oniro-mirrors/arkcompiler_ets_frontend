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

#ifndef ES2PANDA_IR_ANNOTATION_ALLOWED_H
#define ES2PANDA_IR_ANNOTATION_ALLOWED_H

#include "ir/astNode.h"
#include "ir/statement.h"
#include "ir/statements/annotationUsage.h"
#include "util/es2pandaMacros.h"

namespace ark::es2panda::ir {

template <typename T>
class AnnotationAllowed : public T {
public:
    AnnotationAllowed() = delete;
    ~AnnotationAllowed() override = default;

    NO_COPY_OPERATOR(AnnotationAllowed);
    NO_MOVE_SEMANTIC(AnnotationAllowed);

    void EmplaceAnnotations(AnnotationUsage *source)
    {
        auto newNode = reinterpret_cast<AnnotationAllowed<T> *>(this->GetOrCreateHistoryNode());
        newNode->annotations_.emplace_back(source);
    }

    void ClearAnnotations()
    {
        auto newNode = reinterpret_cast<AnnotationAllowed<T> *>(this->GetOrCreateHistoryNode());
        newNode->annotations_.clear();
    }

    void SetValueAnnotations(AnnotationUsage *source, size_t index)
    {
        auto newNode = reinterpret_cast<AnnotationAllowed<T> *>(this->GetOrCreateHistoryNode());
        auto &arenaVector = newNode->annotations_;
        ES2PANDA_ASSERT(arenaVector.size() > index);
        arenaVector[index] = source;
    };

    void TransformAnnotations(const NodeTransformer &cb, std::string_view const transformationName)
    {
        auto &annotations = Annotations();
        for (size_t ix = 0; ix < annotations.size(); ix++) {
            if (auto *transformedNode = cb(annotations[ix]); annotations[ix] != transformedNode) {
                annotations[ix]->SetTransformedNode(transformationName, transformedNode);
                SetValueAnnotations(transformedNode->AsAnnotationUsage(), ix);
            }
        }
    }

    ArenaVector<ir::AnnotationUsage *> &AnnotationsForUpdate()
    {
        return AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>()->annotations_;
    }

    const ArenaVector<ir::AnnotationUsage *> &Annotations()
    {
        return AstNode::GetHistoryNodeAs<AnnotationAllowed<T>>()->annotations_;
    }

    [[nodiscard]] const ArenaVector<ir::AnnotationUsage *> &Annotations() const noexcept
    {
        return AstNode::GetHistoryNodeAs<AnnotationAllowed<T>>()->annotations_;
    }

    void SetAnnotations(const ArenaVector<ir::AnnotationUsage *> &&annotationList)
    {
        auto &annotations = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>()->annotations_;
        annotations = ArenaVector<AnnotationUsage *> {annotationList};

        for (auto annotation : Annotations()) {
            annotation->SetParent(this);
        }
    }

    void SetAnnotations(const ArenaVector<ir::AnnotationUsage *> &annotationList)
    {
        auto &annotations = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>()->annotations_;
        annotations = annotationList;

        for (auto annotation : Annotations()) {
            annotation->SetParent(this);
        }
    }

    void AddAnnotations(AnnotationUsage *annotations)
    {
        AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>()->annotations_.emplace_back(annotations);
    }

protected:
    explicit AnnotationAllowed(Expression const &other, ArenaAllocator *allocator)
        : T(other), annotations_(allocator->Adapter())
    {
    }
    explicit AnnotationAllowed(AstNodeType const type, ArenaVector<AnnotationUsage *> &&annotations)
        : T(type), annotations_(std::move(annotations))
    {
    }
    explicit AnnotationAllowed(AstNodeType const type, ModifierFlags const flags,
                               ArenaVector<AnnotationUsage *> &&annotations)
        : T(type, flags), annotations_(std::move(annotations))
    {
    }
    explicit AnnotationAllowed(AstNodeType const type, ArenaAllocator *const allocator)
        : T(type), annotations_(allocator->Adapter())
    {
    }
    explicit AnnotationAllowed(AstNodeType const type, ModifierFlags const flags, ArenaAllocator *const allocator)
        : T(type, flags), annotations_(allocator->Adapter())
    {
    }
    explicit AnnotationAllowed(AstNodeType const type, Expression *const key, Expression *const value,
                               ModifierFlags const modifiers, ArenaAllocator *const allocator, bool const isComputed)
        : T(type, key, value, modifiers, allocator, isComputed), annotations_(allocator->Adapter())
    {
    }

    explicit AnnotationAllowed(ArenaAllocator *const allocator, ArenaVector<Statement *> &&statementList)
        : T(allocator, std::move(statementList)), annotations_(allocator->Adapter())
    {
    }

    AnnotationAllowed(AnnotationAllowed const &other)
        : T(static_cast<T const &>(other)), annotations_(other.annotations_.get_allocator())
    {
    }

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = static_cast<AnnotationAllowed<T> *>(other);
        otherImpl->annotations_ = annotations_;
        T::CopyTo(other);
    }

private:
    friend class SizeOfNodeTest;
    ArenaVector<AnnotationUsage *> annotations_;
};
}  // namespace ark::es2panda::ir

#endif
