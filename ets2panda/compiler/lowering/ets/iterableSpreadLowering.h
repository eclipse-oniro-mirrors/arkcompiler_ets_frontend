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

#ifndef ES2PANDA_COMPILER_LOWERING_ETS_ITERABLE_SPREAD_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_ETS_ITERABLE_SPREAD_LOWERING_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::checker {
class Type;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class Expression;
class SpreadElement;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::compiler {

ir::Expression *CloneSpreadArgumentWithSmartType(public_lib::Context *ctx, ir::SpreadElement *spreadElement);

void AppendIterableSpreadToArray(public_lib::Context *ctx, ir::SpreadElement *spreadElement,
                                 ir::Identifier *targetArrayIdent, checker::Type *targetElementType,
                                 ArenaVector<ir::Statement *> &statements);
void AppendSpreadToArray(public_lib::Context *ctx, ir::SpreadElement *spreadElement, ir::Identifier *targetArrayIdent,
                         checker::Type *elementType, ArenaVector<ir::Statement *> &statements);

ir::Identifier *CreateSpreadTempResizableArray(public_lib::Context *ctx, checker::Type *elementType,
                                               ArenaVector<ir::Statement *> &statements);
ir::Identifier *FinalizeSpreadTempArray(public_lib::Context *ctx, checker::Type *arrayType,
                                        ir::Identifier *tempArrayIdent, ArenaVector<ir::Statement *> &statements);

}  // namespace ark::es2panda::compiler

#endif
