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

#include <vector>
#include "services/services.h"
#include "services/utilities.h"
#include "internal_api.h"
#include "public/public.h"

namespace ark::es2panda::lsp {
std::vector<Location> GetImplementationLocationAtPosition(es2panda_Context *context, int position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto options = reinterpret_cast<public_lib::Context *>(context)->config->options;
    auto sourceFiles = options->ArkTSConfig()->Files();
    std::vector<Location> locations;

    if (options->GetExtension() != ScriptExtension::ETS) {
        return locations;
    }

    // NOTE: Currently Ozerovs implementation returns single implementation location.
    // So this section should be updated after the implementation of vector returning
    // function.

    auto implementationList = std::vector<ir::AstNode *> {};
    auto declInfo = GetDefinitionAtPositionImpl(context, (size_t)position);
    auto implementation = declInfo.first;
    if (implementation != nullptr && IsValidImplementation(implementation)) {
        implementationList.push_back(implementation);
    }

    for (auto &node : implementationList) {
        Position sPos(node->Range().start.line, node->Range().start.index);
        Position ePos(node->Range().end.line, node->Range().end.index);

        // NOTE: It should be implementations sourcefilename but at the moment this should suffice
        locations.emplace_back(Location(ctx->sourceFileName, Range(sPos, ePos)));
    }

    return locations;
}

}  // namespace ark::es2panda::lsp
