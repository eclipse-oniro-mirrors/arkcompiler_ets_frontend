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

#ifndef ES2PANDA_IR_OBFUSCATION_NAME_CACHE_H
#define ES2PANDA_IR_OBFUSCATION_NAME_CACHE_H

#include "util/es2pandaMacros.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

namespace ark::es2panda::ir {
class FunctionDeclaration;
class ClassDefinition;
class VariableDeclaration;
class TSInterfaceDeclaration;
class TSEnumDeclaration;
class AnnotationDeclaration;
class Identifier;
class ScriptFunction;
class ClassProperty;
class MethodDefinition;
class TSEnumMember;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::ir {

class ObfuscationNameCache {
public:
    ObfuscationNameCache() = default;
    ~ObfuscationNameCache() = default;

    NO_COPY_SEMANTIC(ObfuscationNameCache);
    NO_MOVE_SEMANTIC(ObfuscationNameCache);

    bool GenerateJsonFile(const std::string &outputPath);
    std::string GetModuleName() const;
    void SetModuleName(const std::string &moduleName);

    void RecordClass(const ClassDefinition *classDef);
    void RecordClassProperty(const ClassProperty *prop);
    void RecordMethod(const MethodDefinition *method);
    void RecordEnum(const TSEnumDeclaration *enumDecl);
    void RecordEnumMember(const TSEnumMember *member);

    void RecordFunction(const FunctionDeclaration *funcDecl);
    void RecordInterface(const TSInterfaceDeclaration *interfaceDecl);

    void RecordAnnotationDeclaration(const AnnotationDeclaration *annotationDecl);

private:
    enum class NameCacheType { CLAZZ, CLAZZ_PROPERTY, CLAZZ_METHOD };
    class NameCacheValue {
        friend class ObfuscationNameCache;

    public:
        NameCacheValue(NameCacheType type, const std::string & /*key*/) : type_(type) {}

        const NameCacheType &GetType() const
        {
            return type_;
        }

        const std::unordered_map<std::string, std::shared_ptr<NameCacheValue>> &GetChildrenNode() const
        {
            return childrenNode_;
        }

    private:
        NameCacheType type_;
        std::unordered_map<std::string, std::shared_ptr<NameCacheValue>> childrenNode_ {};
    };

    std::string moduleName_ {};
    std::unordered_map<std::string, std::shared_ptr<NameCacheValue>> fileCache_ {};

    std::string GetEnumInternalName(const TSEnumDeclaration *enumDecl);
    void AddNameEntry(const std::string &name, const NameCacheType &type);
    void RecordInterfaceAccessorPair(const MethodDefinition *method);

    std::string GetMethodName(const std::string &name);
    std::string GetMethodSignature(const std::string &name);

    void WriteValueToJson(const std::unordered_map<std::string, std::shared_ptr<NameCacheValue>> &dataMap,
                          std::ofstream &ofs, const std::string &obfName);
};

}  // namespace ark::es2panda::ir

#endif  // ES2PANDA_IR_OBFUSCATION_NAME_CACHE_H
