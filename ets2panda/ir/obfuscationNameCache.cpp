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

#include <fstream>
#include <algorithm>
#include <cctype>
#include <string_view>
#include <vector>
#include "ir/statements/functionDeclaration.h"
#include "ir/base/classDefinition.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/identifier.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/typeNode.h"
#include "ir/ts/tsEnumMember.h"
#include "ir/srcDump.h"
#include "public/public.h"
#include "checker/ETSchecker.h"
#include "generated/signatures.h"

#include "obfuscationNameCache.h"

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace {
constexpr std::string_view NAME_CACHE_OBF_NAME = "#obfName";

struct TypeNameCacheMapping {
    std::string_view from;
    std::string_view to;
};

// Boxed std.core.* wrappers → bytecode primitive short names used in nameCache signatures.
// Longer keys are listed first so overlapping prefixes among map keys stay deterministic.
constexpr TypeNameCacheMapping TYPE_TO_NAME_CACHE_MAP[] = {
    {"std.core.Boolean", "u1"}, {"std.core.Double", "f64"}, {"std.core.Float", "f32"}, {"std.core.Short", "i16"},
    {"std.core.Byte", "i8"},    {"std.core.Char", "u16"},   {"std.core.Long", "i64"},  {"std.core.Int", "i32"},
};

bool IsTypeNameChar(unsigned char c)
{
    return std::isalnum(c) != 0 || c == '.' || c == '_';
}

// True when `pos` starts a type token (not a longer identifier that merely contains the mapped name).
bool IsMappedTypeStartBoundary(const std::string &signature, size_t pos)
{
    if (pos == 0) {
        return true;
    }
    const unsigned char prev = static_cast<unsigned char>(signature[pos - 1]);
    if (!IsTypeNameChar(prev)) {
        return true;
    }
    // Union encoding prefixes the first alternative with 'U': "{Ustd.core.Int,...}".
    static constexpr std::string_view UNION_TYPE_PREFIX = "{U";
    return pos >= UNION_TYPE_PREFIX.size() &&
           signature.compare(pos - UNION_TYPE_PREFIX.size(), UNION_TYPE_PREFIX.size(), UNION_TYPE_PREFIX) == 0;
}

bool IsMappedTypeEndBoundary(const std::string &signature, size_t end)
{
    // Allow suffixes such as "[]" so "std.core.Int[]" becomes "i32[]".
    return end == signature.size() || !IsTypeNameChar(static_cast<unsigned char>(signature[end]));
}

std::string ReplaceBoxedTypesForNameCache(std::string signature)
{
    for (const auto &mapping : TYPE_TO_NAME_CACHE_MAP) {
        size_t startPos = 0;
        while ((startPos = signature.find(mapping.from, startPos)) != std::string::npos) {
            const size_t endPos = startPos + mapping.from.size();
            if (IsMappedTypeStartBoundary(signature, startPos) && IsMappedTypeEndBoundary(signature, endPos)) {
                signature.replace(startPos, mapping.from.size(), mapping.to);
                startPos += mapping.to.size();
            } else {
                startPos = endPos;
            }
        }
    }
    return signature;
}

// Resolves the declaring class/interface name for a non-static method.
std::string GetOwnerClassName(const ark::es2panda::ir::MethodDefinition *method)
{
    for (const auto *parent = method->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsClassDefinition()) {
            return parent->AsClassDefinition()->InternalName().Mutf8();
        }
        if (parent->IsTSInterfaceBody()) {
            return parent->Parent()->AsTSInterfaceDeclaration()->InternalName().Mutf8();
        }
    }
    return {};
}

std::string ReplaceReturnTypeWithVoid(std::string name)
{
    const std::string sep {ark::es2panda::compiler::Signatures::MANGLE_SEPARATOR};
    const auto colonPos = name.find(ark::es2panda::compiler::Signatures::MANGLE_BEGIN);
    if (colonPos == std::string::npos) {
        return name;
    }
    if (name.empty() || name.back() != sep[0]) {
        return name;
    }
    // Prefer the last ';' before the terminating ';'; fall back to ':' for
    // parameter-less signatures such as "fun1003:std.core.Object;".
    const std::string withoutTerminator = name.substr(0, name.size() - sep.size());
    auto cutPos = withoutTerminator.rfind(sep[0]);
    if (cutPos == std::string::npos || cutPos < colonPos) {
        cutPos = colonPos;
    }
    return name.substr(0, cutPos + sep.size()) + std::string(ark::es2panda::compiler::Signatures::PRIMITIVE_VOID) + sep;
}

bool IsVoidLikeReturnType(const ark::es2panda::checker::Type *returnType)
{
    return returnType != nullptr && (returnType->IsETSUndefinedType() || returnType->IsETSVoidType());
}

std::string NormalizeNameCacheMethodName(std::string name, const ark::es2panda::checker::Signature *sig, bool isSetter)
{
    if (isSetter) {
        return ReplaceReturnTypeWithVoid(std::move(name));
    }
    if (sig != nullptr && IsVoidLikeReturnType(sig->ReturnType())) {
        return ReplaceReturnTypeWithVoid(std::move(name));
    }
    return name;
}

std::string StripEtsExtension(std::string name)
{
    static constexpr std::string_view D_ETS_SUFFIX = ".d.ets";
    static constexpr std::string_view ETS_SUFFIX = ".ets";
    if (name.size() >= D_ETS_SUFFIX.size() &&
        name.compare(name.size() - D_ETS_SUFFIX.size(), D_ETS_SUFFIX.size(), D_ETS_SUFFIX) == 0) {
        name.resize(name.size() - D_ETS_SUFFIX.size());
    } else if (name.size() >= ETS_SUFFIX.size() &&
               name.compare(name.size() - ETS_SUFFIX.size(), ETS_SUFFIX.size(), ETS_SUFFIX) == 0) {
        name.resize(name.size() - ETS_SUFFIX.size());
    }
    return name;
}

std::string NormalizeCacheSignature(const std::string &moduleName, std::string signature)
{
    const std::string separator {ark::es2panda::compiler::Signatures::METHOD_SEPARATOR};
    const std::string modulePrefix = moduleName + separator;
    const std::string cachePrefix = StripEtsExtension(moduleName) + separator;
    if (moduleName.empty() || modulePrefix == cachePrefix) {
        return signature;
    }
    size_t pos = 0;
    while ((pos = signature.find(modulePrefix, pos)) != std::string::npos) {
        signature.replace(pos, modulePrefix.size(), cachePrefix);
        pos += cachePrefix.size();
    }
    return signature;
}
}  // namespace

namespace ark::es2panda::ir {

std::string ObfuscationNameCache::GetModuleName() const
{
    return moduleName_;
}

void ObfuscationNameCache::SetModuleName(const std::string &moduleName)
{
    moduleName_ = moduleName;
}

bool ObfuscationNameCache::GenerateJsonFile(const std::string &outputPath)
{
    if (fileCache_.empty()) {
        return true;
    }
    fs::path dir = outputPath;
    std::string baseName = StripEtsExtension(moduleName_.empty() ? fileCache_.begin()->first : moduleName_);
    fs::path file = baseName + ".json";
    fs::path jsonPath = dir / file;
    std::ofstream ofs(jsonPath.string());
    if (!ofs.is_open()) {
        return false;
    }
    WriteValueToJson(fileCache_, ofs, std::string());
    ofs.close();
    return ofs.good();
}

void ObfuscationNameCache::RecordClass(const ClassDefinition *classDef)
{
    std::string name = classDef->InternalName().Mutf8();
    AddNameEntry(name, NameCacheType::CLAZZ);
}

void ObfuscationNameCache::RecordClassProperty(const ClassProperty *prop)
{
    if (prop->Parent()->IsClassDefinition()) {
        std::string name = prop->Parent()->AsClassDefinition()->InternalName().Mutf8() +
                           std::string(compiler::Signatures::METHOD_SEPARATOR) + prop->Id()->Name().Mutf8();
        AddNameEntry(name, NameCacheType::CLAZZ_PROPERTY);
    }
}

void ObfuscationNameCache::RecordInterfaceAccessorPair(const MethodDefinition *method)
{
    std::string className = method->Parent()->Parent()->AsTSInterfaceDeclaration()->InternalName().Mutf8();

    std::string internalName = method->Function()->Signature()->InternalName().Mutf8();
    std::string fieldType;
    size_t colonPos = internalName.find(compiler::Signatures::MANGLE_BEGIN);
    if (colonPos != std::string::npos) {
        fieldType = internalName.substr(colonPos + compiler::Signatures::MANGLE_BEGIN.size());
    }

    std::string propName = method->OriginalNode()->AsClassProperty()->Key()->AsIdentifier()->Name().Mutf8();
    std::string mangledName = propName + std::string(compiler::Signatures::MANGLE_BEGIN) + className +
                              std::string(compiler::Signatures::MANGLE_SEPARATOR) + fieldType;
    std::string sep = std::string(compiler::Signatures::METHOD_SEPARATOR);

    AddNameEntry(className + sep + std::string(compiler::Signatures::GETTER_METHOD_BEGIN) + mangledName,
                 NameCacheType::CLAZZ_METHOD);
    AddNameEntry(className + sep + std::string(compiler::Signatures::SETTER_METHOD_BEGIN) + mangledName + "void;",
                 NameCacheType::CLAZZ_METHOD);
}

void ObfuscationNameCache::RecordMethod(const MethodDefinition *method)
{
    if (method->IsConstructor()) {
        return;
    }
    if ((method->IsGetter() || method->IsSetter()) && method->IsOptionalDeclaration() &&
        method->Parent()->IsTSInterfaceBody() && method->OriginalNode() != nullptr &&
        method->OriginalNode()->IsClassProperty()) {
        RecordInterfaceAccessorPair(method);
        return;
    }

    auto *signature = method->Function()->Signature();
    if (signature == nullptr) {
        return;
    }
    std::string name = signature->InternalName().Mutf8();
    if (!method->IsStatic()) {
        std::string className = GetOwnerClassName(method);
        size_t pos = name.find(compiler::Signatures::MANGLE_BEGIN);
        if (pos != std::string::npos && !className.empty()) {
            name.insert(pos + compiler::Signatures::MANGLE_BEGIN.size(),
                        className + std::string(compiler::Signatures::MANGLE_SEPARATOR));
        }
    }
    name = NormalizeNameCacheMethodName(std::move(name), signature, method->IsSetter());
    AddNameEntry(name, NameCacheType::CLAZZ_METHOD);
}

void ObfuscationNameCache::RecordEnum(const TSEnumDeclaration *enumDecl)
{
    std::string name = GetEnumInternalName(enumDecl);
    AddNameEntry(name, NameCacheType::CLAZZ);
}

void ObfuscationNameCache::RecordEnumMember(const TSEnumMember *member)
{
    std::string name = GetEnumInternalName(member->Parent()->AsTSEnumDeclaration()) +
                       std::string(compiler::Signatures::METHOD_SEPARATOR) +
                       member->Key()->AsIdentifier()->Name().Mutf8();
    AddNameEntry(name, NameCacheType::CLAZZ_PROPERTY);
}

void ObfuscationNameCache::RecordFunction(const FunctionDeclaration *funcDecl)
{
    auto *signature = funcDecl->Function()->Signature();
    if (signature == nullptr) {
        return;
    }
    std::string name = signature->InternalName().Mutf8();
    name = NormalizeNameCacheMethodName(std::move(name), signature, false);
    AddNameEntry(name, NameCacheType::CLAZZ_METHOD);
}

void ObfuscationNameCache::RecordInterface(const TSInterfaceDeclaration *interfaceDecl)
{
    std::string name = interfaceDecl->InternalName().Mutf8();
    AddNameEntry(name, NameCacheType::CLAZZ);
}

void ObfuscationNameCache::RecordAnnotationDeclaration(const AnnotationDeclaration *annotationDecl)
{
    std::string name = annotationDecl->InternalName().Mutf8();
    AddNameEntry(name, NameCacheType::CLAZZ);
}

std::string ObfuscationNameCache::GetEnumInternalName(const TSEnumDeclaration *enumDecl)
{
    std::string name;
    if (enumDecl->Parent()->IsProgram()) {
        name = std::string(enumDecl->Parent()->AsETSModule()->Program()->ModuleName());
    } else if (enumDecl->Parent()->IsClassDefinition()) {
        name = enumDecl->Parent()->AsClassDefinition()->InternalName().Mutf8();
    }
    return name + std::string(compiler::Signatures::METHOD_SEPARATOR) + enumDecl->Key()->Name().Mutf8();
}

void ObfuscationNameCache::AddNameEntry(const std::string &allName, const NameCacheType &type)
{
    if (allName.empty()) {
        return;
    }
    std::vector<std::string> names;
    std::string separator = std::string(compiler::Signatures::METHOD_SEPARATOR);
    size_t pos = 0;

    std::string tmpName = allName;
    const std::string modulePrefix = moduleName_ + separator;
    if (!moduleName_.empty() && tmpName.compare(0, modulePrefix.size(), modulePrefix) == 0) {
        tmpName.erase(0, modulePrefix.size());
        names.push_back(StripEtsExtension(moduleName_));
    }
    std::string globalTag =
        std::string(compiler::Signatures::ETS_GLOBAL) + std::string(compiler::Signatures::METHOD_SEPARATOR);
    if (tmpName.length() >= globalTag.size() && tmpName.substr(0, globalTag.size()) == globalTag) {
        tmpName.erase(0, globalTag.size());
    }
    std::string signature;
    if (type == NameCacheType::CLAZZ_METHOD) {
        signature = GetMethodSignature(tmpName);
        tmpName = GetMethodName(tmpName);
    }
    while ((pos = tmpName.find(separator)) != std::string::npos) {
        names.push_back(tmpName.substr(0, pos));
        tmpName.erase(0, pos + separator.size());
    }
    names.push_back(tmpName + signature);
    std::unordered_map<std::string, std::shared_ptr<NameCacheValue>> *map = &fileCache_;
    for (std::size_t i = 0; i < names.size(); ++i) {
        std::string name = names.at(i);
        NameCacheType currType = (i == (names.size() - 1)) ? type : NameCacheType::CLAZZ;
        const auto &iter = map->find(name);
        if (iter != map->end()) {
            map = &(iter->second->childrenNode_);
        } else {
            std::shared_ptr<NameCacheValue> nameCacheValue = std::make_shared<NameCacheValue>(currType, name);
            map->insert({name, nameCacheValue});
            map = &(nameCacheValue->childrenNode_);
        }
    }
}

std::string ObfuscationNameCache::GetMethodName(const std::string &name)
{
    size_t pos = name.find(compiler::Signatures::MANGLE_BEGIN);
    if (pos != std::string::npos) {
        return name.substr(0, pos);
    }
    return name;
}

std::string ObfuscationNameCache::GetMethodSignature(const std::string &name)
{
    size_t pos = name.find(compiler::Signatures::MANGLE_BEGIN);
    if (pos == std::string::npos) {
        return std::string();
    }
    std::string signature = NormalizeCacheSignature(moduleName_, name.substr(pos));
    return ReplaceBoxedTypesForNameCache(std::move(signature));
}

void ObfuscationNameCache::WriteValueToJson(
    const std::unordered_map<std::string, std::shared_ptr<NameCacheValue>> &dataMap, std::ofstream &ofs,
    const std::string &obfName)
{
    ofs << "{";
    bool needComma = false;
    if (!obfName.empty()) {
        ofs << "\"" << NAME_CACHE_OBF_NAME << "\":\"" << obfName << "\"";
        needComma = true;
    }
    std::vector<std::string> keys;
    keys.reserve(dataMap.size());
    for (const auto &[key, _] : dataMap) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto &key : keys) {
        const auto &val = dataMap.at(key);
        if (needComma) {
            ofs << ",";
        }
        ofs << "\"" << key << "\":";
        if (val->GetType() == NameCacheType::CLAZZ) {
            WriteValueToJson(val->GetChildrenNode(), ofs, key);
        } else if (val->GetType() == NameCacheType::CLAZZ_PROPERTY) {
            ofs << "\"" << key << "\"";
        } else if (val->GetType() == NameCacheType::CLAZZ_METHOD) {
            ofs << "\"" << GetMethodName(key) << "\"";
        }
        needComma = true;
    }
    ofs << "}";
}

}  // namespace ark::es2panda::ir
