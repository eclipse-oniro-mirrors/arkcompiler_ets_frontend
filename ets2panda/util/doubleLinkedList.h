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

#ifndef ES2PANDA_UTIL_DOUBLE_LINKED_LIST_H
#define ES2PANDA_UTIL_DOUBLE_LINKED_LIST_H

#include "libarkbase/mem/arena_allocator.h"

namespace ark::es2panda::util {

template <typename T>
class ArenaDoubleLinkedList {
public:
    explicit ArenaDoubleLinkedList(ArenaAllocator *allocator) : allocator_ {allocator} {}

    struct Item {
        T data {};
        Item *next {nullptr};
        Item *prev {nullptr};
    };

    Item *Head()
    {
        return head_;
    }

    Item *Tail()
    {
        return tail_;
    }

    bool Empty()
    {
        return head_ == nullptr;
    };

    ArenaAllocator *Allocator()
    {
        return allocator_;
    }

    Item *Append(const T &data)
    {
        auto item = allocator_->New<Item>();
        ES2PANDA_ASSERT(item != nullptr);
        item->data = data;
        Append(item);
        return item;
    }

    void Append(Item *item)
    {
        item->next = nullptr;
        item->prev = nullptr;

        if (Empty()) {
            head_ = item;
            tail_ = item;
        } else {
            tail_->next = item;
            item->prev = tail_;
            tail_ = item;
        }
    }

    Item *Prepend(const T &data)
    {
        auto item = allocator_->New<Item>();
        ES2PANDA_ASSERT(item != nullptr);
        item->data = data;
        Prepend(item);
        return item;
    }

    void Prepend(Item *item)
    {
        item->next = nullptr;
        item->prev = nullptr;

        if (Empty()) {
            head_ = item;
            tail_ = item;
        } else {
            head_->prev = item;
            item->next = head_;
            head_ = item;
        }
    }

    Item *Insert(Item *after, const T &data)
    {
        auto item = allocator_->New<Item>();
        ES2PANDA_ASSERT(item != nullptr);
        item->data = data;
        Insert(after, item);
        return item;
    }

    void Insert(Item *after, Item *item)
    {
        ES2PANDA_ASSERT(!Empty());
        ES2PANDA_ASSERT(after != nullptr);
        ES2PANDA_ASSERT(item != nullptr);

        auto afterNext = after->next;

        after->next = item;
        item->prev = after;

        item->next = afterNext;
        if (afterNext != nullptr) {
            afterNext->prev = item;
        }

        if (after == tail_) {
            tail_ = item;
        }
    }

    void Erase(Item *item)
    {
        ES2PANDA_ASSERT(!Empty());
        ES2PANDA_ASSERT(item != nullptr);

        if (item == head_ && item == tail_) {
            // Item is the only element in list
            head_ = nullptr;
            tail_ = nullptr;
        } else if (item == head_) {
            head_ = head_->next;
            if (head_ != nullptr) {
                head_->prev = nullptr;
            }
        } else if (item == tail_) {
            tail_ = tail_->prev;
            if (tail_ != nullptr) {
                tail_->next = nullptr;
            }
        } else {
            // Item is not a head or tail element of list
            auto prev = item->prev;
            auto next = item->next;

            ES2PANDA_ASSERT(prev != nullptr && next != nullptr);
            prev->next = next;
            next->prev = prev;
        }

        item->next = nullptr;
        item->prev = nullptr;
    }

private:
    Item *head_ {nullptr};
    Item *tail_ {nullptr};
    ArenaAllocator *allocator_;
};
}  // namespace ark::es2panda::util
#endif
