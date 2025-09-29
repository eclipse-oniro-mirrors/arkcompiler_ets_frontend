/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_LOWERING_INITMETHOD_H
#define ES2PANDA_COMPILER_LOWERING_INITMETHOD_H

#include "compiler/lowering/phase.h"
#include "util/helpers.h"
#include "parser/program/program.h"
#include "compiler/lowering/ets/topLevelStmts/globalDeclTransformer.h"
#include "compiler/lowering/ets/topLevelStmts/importExportDecls.h"

namespace ark::es2panda::compiler {

/**
 * Purpose of this lowering to:
 * Create _$init$_ method
 * Add all top level statements to _$init$_
 * Add call of _$init$_ to ETSGLOBAL CCTOR
 * Handle imports/exports (re_exports, add stdlib, check existance of named exports)
 *
 * Expected to be called before ScopesInitPhase
 */
class TopLevelStatements : public Phase {
public:
    std::string_view Name() const override
    {
        return "TopLevelStatements";
    }

    bool Perform() override;
};

}  // namespace ark::es2panda::compiler
#endif