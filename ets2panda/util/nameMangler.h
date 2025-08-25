/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NAMEMANGLER_H
#define NAMEMANGLER_H

#include <string>
#include "ustring.h"
#include <mutex>

namespace ark::es2panda::util {
class NameMangler {
public:
    enum LangFeatureType {
        ASYNC,
        GET,
        PARTIAL,
        PROPERTY,
        SET,
    };

    static NameMangler *GetInstance()
    {
        static NameMangler *manglerInstance;
        if (manglerInstance == nullptr) {
            manglerInstance = new NameMangler();
        }

        return manglerInstance;
    };

    std::string CreateMangledNameByTypeAndName(LangFeatureType type, const util::StringView &nodeName);
    std::string CreateMangledNameForLambdaInvoke(size_t invokeCounter);
    std::string CreateMangledNameForLambdaObject(const util::StringView &lambdaInvokeName);
    std::string CreateMangledNameForUnionProperty(const std::string &propTypeName);
    std::string CreateMangledNameForAnnotation(const std::string &baseName, const std::string &annotationName);
    std::string AppendToAnnotationName(const std::string &annotationName, const std::string &secondPart);

    std::string GetOriginalClassNameFromPartial(const std::string &partialName);

private:
    NameMangler() = default;
};
}  // namespace ark::es2panda::util

#endif  // NAMEMANGLER_H
