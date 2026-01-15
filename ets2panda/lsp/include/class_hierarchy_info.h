/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_LSP_CLASS_HIERARCHY_INFO_H
#define ES2PANDA_LSP_CLASS_HIERARCHY_INFO_H

#include <memory>
#include <unordered_map>
#include <vector>
#include "class_hierarchy_item.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
class FunctionParamStyle {
public:
    FunctionParamStyle(std::string paramName, std::string paramKind)
        : name_(std::move(paramName)), kind_(std::move(paramKind))
    {
    }

    std::string GetParamName() const
    {
        return name_;
    }

    std::string GetParamKind() const
    {
        return kind_;
    }

private:
    std::string name_;
    std::string kind_;
};

class ClassHierarchyInfo {
public:
    ClassHierarchyInfo() = default;

    virtual ~ClassHierarchyInfo() = default;

    ClassHierarchyInfo(const ClassHierarchyInfo &other) = default;
    ClassHierarchyInfo(ClassHierarchyInfo &&other) = default;
    ClassHierarchyInfo &operator=(const ClassHierarchyInfo &other) = default;
    ClassHierarchyInfo &operator=(ClassHierarchyInfo &&other) = default;

    void SetClassName(const std::string &className)
    {
        if (className.empty()) {
            return;
        }
        className_ = className;
    }

    const std::string &GetName() const
    {
        return className_;
    }

    const std::unordered_map<std::string, std::shared_ptr<ClassMethodItem>> &GetMethodItemList() const
    {
        return methodItems_;
    }

    const std::unordered_map<std::string, std::shared_ptr<ClassPropertyItem>> &GetPropertyItemList() const
    {
        return propertyItems_;
    }

    bool AddItemToMethodList(const std::shared_ptr<ClassMethodItem> &item)
    {
        if (item == nullptr) {
            return false;
        }
        auto detail = item->GetDetail();
        auto result = methodItems_.try_emplace(detail, item);
        return result.second;
    }

    bool AddItemToPropertyList(const std::shared_ptr<ClassPropertyItem> &item)
    {
        if (item == nullptr) {
            return false;
        }
        auto detail = item->GetDetail();
        auto result = propertyItems_.try_emplace(detail, item);
        return result.second;
    }

    void DeleteTargetItemInMethodList(const std::shared_ptr<ClassMethodItem> &item)
    {
        if (item == nullptr) {
            return;
        }
        auto detail = item->GetDetail();
        methodItems_.erase(detail);
    }

    void DeleteTargetItemInPropertyList(const std::shared_ptr<ClassPropertyItem> &item)
    {
        if (item == nullptr) {
            return;
        }
        auto detail = item->GetDetail();
        propertyItems_.erase(detail);
    }

    void DeleteAllItemsInMethodList()
    {
        methodItems_.clear();
    }

    void DeleteAllItemsInPropertyList()
    {
        propertyItems_.clear();
    }

    bool IsItemExistInMethodList(const std::shared_ptr<ClassMethodItem> &item) const
    {
        if (item == nullptr) {
            return false;
        }
        auto detail = item->GetDetail();
        auto iter = methodItems_.find(detail);
        return iter != methodItems_.end();
    }

    bool IsItemExistInPropertyList(const std::shared_ptr<ClassPropertyItem> &item) const
    {
        if (item == nullptr) {
            return false;
        }
        auto detail = item->GetDetail();
        auto iter = propertyItems_.find(detail);
        return iter != propertyItems_.end();
    }

private:
    std::string className_;
    std::unordered_map<std::string, std::shared_ptr<ClassMethodItem>> methodItems_;
    std::unordered_map<std::string, std::shared_ptr<ClassPropertyItem>> propertyItems_;
};

using ClassHierarchy = std::vector<ClassHierarchyInfo>;

/**
 * Retrieve the list of undefined virtual functions and properties in the parent class.
 *
 * such as ets:
 * class Animal {
 *   public body_: string = '';
 *
 *   public action(): void {
 *       console.log("need Animal action");
 *   }
 *   public sleep(): void {
 *       console.log("need sleep");
 *   }
 * }
 *
 * class Bird extends Animal {
 *   action(): void {
 *       console.log("need Bird action");
 *   }
 *
 *   Drink(): void {
 *       console.log("need Bird Drink");
 *   }
 * }
 * when clicking 'Bird'. ClassHierarchy is :
 *  [ { "Animal", { detail: body_: string, ClassDefinitionStyle FIELD, AccessModifierStyle: PUBLIC } },
 *    { "Animal", { detail: sleep(): void, ClassDefinitionStyle: METHOD, AccessModifierStyle: PUBLIC } } ].
 */
ClassHierarchy GetClassHierarchyInfoImpl(es2panda_Context *context, size_t position);
}  // namespace ark::es2panda::lsp

#endif
