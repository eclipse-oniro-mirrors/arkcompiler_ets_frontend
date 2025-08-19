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

#include "ir/statements/annotationUsage.h"

namespace ark::es2panda::ir {

template <typename T>
class AnnotationAllowed : public T {
public:
    AnnotationAllowed() = delete;
    ~AnnotationAllowed() override = default;

    NO_COPY_OPERATOR(AnnotationAllowed);
    NO_MOVE_SEMANTIC(AnnotationAllowed);

    [[nodiscard]] bool HasAnnotations() const noexcept
    {
        return annotations_ != nullptr && !annotations_->empty();
    }

    void EmplaceAnnotation(AnnotationUsage *source)
    {
        auto *node = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ == nullptr) {
            ES2PANDA_ASSERT(allocator_ != nullptr);
            node->annotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
        }
        source->SetParent(this);
        node->annotations_->emplace_back(source);
    }

    void ClearAnnotations()
    {
        auto *node = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ != nullptr) {
            node->annotations_->clear();
        }
    }

    void TransformAnnotations(const NodeTransformer &cb, std::string_view const transformationName)
    {
        auto *node = AstNode::GetHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ != nullptr && !node->annotations_->empty()) {
            auto &annotations = *node->annotations_;
            for (size_t ix = 0; ix < annotations.size(); ix++) {
                if (auto *transformedNode = cb(annotations[ix]); annotations[ix] != transformedNode) {
                    annotations[ix]->SetTransformedNode(transformationName, transformedNode);
                    transformedNode->SetParent(this);
                    annotations[ix] = transformedNode->AsAnnotationUsage();
                }
            }
        }
    }

    void IterateAnnotations(const NodeTraverser &cb) const
    {
        auto *node = AstNode::GetHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ != nullptr && !node->annotations_->empty()) {
            auto &annotations = *node->annotations_;
            for (auto *anno : VectorIterationGuard(annotations)) {
                cb(anno);
            }
        }
    }

    void DumpAnnotations(ir::SrcDumper *dumper) const
    {
        auto *node = AstNode::GetHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ != nullptr && !node->annotations_->empty()) {
            auto &annotations = *node->annotations_;
            for (auto *anno : annotations) {
                anno->Dump(dumper);
            }
        }
    }

    [[nodiscard]] ArenaVector<ir::AnnotationUsage *> &AnnotationsForUpdate()
    {
        auto *node = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>();
        ES2PANDA_ASSERT(node->annotations_ != nullptr);
        return *node->annotations_;
    }

    [[nodiscard]] ArenaVector<ir::AnnotationUsage *> &Annotations()
    {
        auto *node = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ == nullptr) {
            ES2PANDA_ASSERT(allocator_ != nullptr);
            node->annotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
        }
        return *node->annotations_;
    }

    [[nodiscard]] ArenaVector<ir::AnnotationUsage *> const &Annotations() const
    {
        auto *node = AstNode::GetHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ != nullptr) {
            return *node->annotations_;
        }
        ES2PANDA_ASSERT(emptyAnnotations_ != nullptr);
        emptyAnnotations_->clear();
        return *emptyAnnotations_;
    }

    void SetAnnotations(ArenaVector<ir::AnnotationUsage *> &&annotationList)
    {
        auto *node = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>();
        if (!annotationList.empty()) {
            if (node->annotations_ == nullptr) {
                ES2PANDA_ASSERT(allocator_ != nullptr);
                node->annotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
            }
            auto &annotations = *node->annotations_;
            annotations = std::move(annotationList);
            for (auto *annotation : annotations) {
                annotation->SetParent(this);
            }
        } else if (node->annotations_ != nullptr) {
            node->annotations_->clear();
        }
    }

    void SetAnnotations(ArenaVector<ir::AnnotationUsage *> const &annotationList)
    {
        ES2PANDA_ASSERT(!annotationList.empty());
        auto *node = AstNode::GetOrCreateHistoryNodeAs<AnnotationAllowed<T>>();
        if (node->annotations_ == nullptr) {
            ES2PANDA_ASSERT(allocator_ != nullptr);
            node->annotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
        } else {
            node->annotations_->clear();
        }
        for (auto *anno : annotationList) {
            node->annotations_->emplace_back(anno->Clone(allocator_, this));
        }
    }

protected:
    explicit AnnotationAllowed(Expression const &other, ArenaAllocator *allocator) : T(other)
    {
        InitClass(allocator);
    }
    explicit AnnotationAllowed(AstNodeType const type, ArenaAllocator *const allocator) : T(type)
    {
        InitClass(allocator);
    }
    explicit AnnotationAllowed(AstNodeType const type, ArenaVector<AnnotationUsage *> &&annotations,
                               ArenaAllocator *const allocator)
        : T(type)
    {
        annotations_ = allocator->New<ArenaVector<AnnotationUsage *>>(allocator->Adapter());
        *annotations_ = std::move(annotations);
        InitClass(allocator);
    }
    explicit AnnotationAllowed(AstNodeType const type, TypeNode *typeAnnotation, ArenaAllocator *const allocator)
        : T(type, typeAnnotation)
    {
        InitClass(allocator);
    }
    explicit AnnotationAllowed(AstNodeType const type, ModifierFlags const flags, ArenaAllocator *const allocator)
        : T(type, flags)
    {
        InitClass(allocator);
    }
    explicit AnnotationAllowed(AstNodeType const type, Expression *const key, Expression *const value,
                               ModifierFlags const modifiers, ArenaAllocator *const allocator, bool const isComputed)
        : T(type, key, value, modifiers, allocator, isComputed)
    {
        InitClass(allocator);
    }

    explicit AnnotationAllowed(ArenaAllocator *const allocator, ArenaVector<Statement *> &&statementList)
        : T(allocator, std::move(statementList))
    {
        InitClass(allocator);
    }

    AnnotationAllowed(AnnotationAllowed const &other) : T(static_cast<T const &>(other))
    {
        if (other.annotations_ != nullptr && !other.annotations_->empty()) {
            ES2PANDA_ASSERT(allocator_ != nullptr);
            annotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
            for (auto *anno : *other.annotations_) {
                annotations_->emplace_back(anno->Clone(allocator_, this));
            }
        }
    }

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = static_cast<AnnotationAllowed<T> *>(other);

        if (annotations_ != nullptr && !annotations_->empty()) {
            if (otherImpl->annotations_ == nullptr) {
                ES2PANDA_ASSERT(allocator_ != nullptr);
                otherImpl->annotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
            }
            for (auto *anno : *annotations_) {
                otherImpl->annotations_->emplace_back(anno->Clone(allocator_, other));
            }
        } else if (otherImpl->annotations_ != nullptr) {
            otherImpl->annotations_->clear();
        }

        T::CopyTo(other);
    }

private:
    friend class SizeOfNodeTest;
    ArenaVector<AnnotationUsage *> *annotations_ = nullptr;

    static inline ArenaAllocator *allocator_ = nullptr;
    static inline ArenaVector<AnnotationUsage *> *emptyAnnotations_ = nullptr;

    static void InitClass(ArenaAllocator *alloc)
    {
        ES2PANDA_ASSERT(alloc != nullptr);
        if (allocator_ != alloc) {
            allocator_ = alloc;
            emptyAnnotations_ = allocator_->New<ArenaVector<AnnotationUsage *>>(allocator_->Adapter());
        }
    }
};
}  // namespace ark::es2panda::ir

#endif
