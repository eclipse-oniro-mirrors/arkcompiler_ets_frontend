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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_SYMBOL_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_SYMBOL_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include "libarkbase/macros.h"
#include "nlohmann/json.hpp"

/**
 * @brief custom json handler for std::optional<T> type
 */
namespace nlohmann {
template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json &j, const std::optional<T> &value)
    {
        if (value.has_value()) {
            j = *value;
        } else {
            j = nullptr;
        }
    }

    static void from_json(const json &j, std::optional<T> &value)
    {
        if (j.is_null()) {
            value = std::nullopt;
        } else {
            value = j.get<T>();
        }
    }
};
}  // namespace nlohmann

namespace ark::es2panda::gluegen {

enum class SymbolKind {
    FUNCTION,
    CLASS,
    PROPERTY,
    NAMESPACE,
    DYNAMIC_RE_EXPORT,  // this kind is reserved and used for static re-export dynamic
    INIT_MODULE,        // initModule() function
};

NLOHMANN_JSON_SERIALIZE_ENUM(SymbolKind, {
                                             {SymbolKind::FUNCTION, "function"},
                                             {SymbolKind::CLASS, "class"},
                                             {SymbolKind::PROPERTY, "property"},
                                             {SymbolKind::NAMESPACE, "namespace"},
                                             {SymbolKind::DYNAMIC_RE_EXPORT, "dynamic-re-export"},
                                             {SymbolKind::INIT_MODULE, "init-module"},
                                         })

struct SymbolNode {
    const std::string name;                  // Symbol name, use alias name
    const SymbolKind kind;                   // Symbol kind
    std::optional<std::string> runtimeName;  // Symbol runtime name, for example: Lentry/some/ETSGLOBAL
    // The symbol's original (locally declared) name, before any export alias/rename/default.
    // E.g. for `class A {}; export { A as A_alias };`, name == "A_alias", localName == "A".
    // For `export default A;`, name == "default", localName == "A". std::nullopt when there is
    // no separate local name (e.g. SymbolKind::INIT_MODULE, where `name` is already the module
    // specifier).
    std::optional<std::string> localName;
    // For SymbolKind::INIT_MODULE only: the resolved "<modulePrefix>ETSGLOBAL" qualified name that
    // `initModule()` was lowered to (see InitModuleLowering); temporarily stored here until
    // init-module glue generation has its own dedicated representation.
    std::optional<std::string> initModuleParam;
    // For SymbolKind::DYNAMIC_RE_EXPORT: the raw module specifier of the original re-export
    // statement (`export ... from '<source>'`) that points at a dynamic (interop) module.
    // For SymbolKind::INIT_MODULE: the normalized on-disk path of the target module resolved from
    // the (post-lowering) initModule() call, or std::nullopt if it could not be resolved.
    // std::nullopt for every other kind.
    std::optional<std::string> source;
    std::unordered_map<std::string, SymbolNode *>
        children;  // Children symbols, only namespace can have children symbols
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SymbolNode, name, kind, runtimeName, localName, initModuleParam, source, children)

/**
 * @brief manager for SymbolNode instances, all symbols should be managed by this class, and the memory of SymbolNode
 * will be released when this class destructed.
 *
 * Not a singleton: each Gluegen run owns exactly one instance, held by its Context (see
 * context.h), so distinct runs never share (or leak into) each other's symbol tables.
 *
 * Thread-safe: SymbolNode/IntermediateCache deserialization (see gluec.h's IntermediateCacheReader)
 * may run CreateSymbolNode concurrently from multiple background threads, so all access to the
 * internal containers is guarded by mutex_.
 */
class SymbolNodeManager {
public:
    SymbolNodeManager() = default;
    ~SymbolNodeManager() = default;

    NO_COPY_SEMANTIC(SymbolNodeManager);
    NO_MOVE_SEMANTIC(SymbolNodeManager);

    SymbolNode *CreateSymbolNode(const std::string &name, const SymbolKind &kind,
                                 const std::optional<std::string> &runtimeName = std::nullopt,
                                 const std::optional<std::string> &localName = std::nullopt,
                                 const std::optional<std::string> &initModuleParam = std::nullopt,
                                 const std::optional<std::string> &source = std::nullopt);
    void RegisterFileRootSymbolNode(const std::string &filePath, SymbolNode *rootSymbolNode);

private:
    std::mutex mutex_;
    std::vector<std::unique_ptr<SymbolNode>> symbolNodes_;
    std::unordered_map<std::string, SymbolNode *> fileRootSymbolNodes_;
};

}  // namespace ark::es2panda::gluegen

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_SYMBOL_H