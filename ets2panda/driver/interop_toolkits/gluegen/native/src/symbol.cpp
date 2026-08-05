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
#include "symbol.h"

namespace ark::es2panda::gluegen {
void SymbolNodeManager::RegisterFileRootSymbolNode(const std::string &filePath, SymbolNode *rootSymbolNode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    fileRootSymbolNodes_[filePath] = rootSymbolNode;
}

SymbolNode *SymbolNodeManager::CreateSymbolNode(const std::string &name, const SymbolKind &kind,
                                                const std::optional<std::string> &runtimeName,
                                                const std::optional<std::string> &localName,
                                                const std::optional<std::string> &initModuleParam,
                                                const std::optional<std::string> &source)
{
    auto symbolNode =
        std::make_unique<SymbolNode>(SymbolNode {name, kind, runtimeName, localName, initModuleParam, source, {}});
    auto symbolNodePtr = symbolNode.get();
    std::lock_guard<std::mutex> lock(mutex_);
    symbolNodes_.push_back(std::move(symbolNode));
    return symbolNodePtr;
}
}  // namespace ark::es2panda::gluegen