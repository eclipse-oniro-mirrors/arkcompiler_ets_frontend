/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

<<<<<<<< HEAD:ets2panda/linter/src/lib/utils/functions/CookBookUtils.ts
export function extractRuleTags(tags: string[]): Map<number, string> {
  const resultMap = new Map<number, string>();

  for (let i = 0; i < tags.length; i++) {
    const tag = tags[i];

    if (!tag?.trim()) {
      continue;
    }

    const regex = /\(([^)]+)\)/;
    const match = tag.match(regex);

    if (match?.[1]?.trim()) {
      resultMap.set(i, match[1]);
    }
  }

  return resultMap;
}
========
#ifndef ES2PANDA_COMPILER_LOWERING_PRIMITIVE_CONVERSION_PHASE_H
#define ES2PANDA_COMPILER_LOWERING_PRIMITIVE_CONVERSION_PHASE_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

class PrimitiveConversionPhase : public PhaseForBodies {
public:
    std::string_view Name() const override
    {
        return "PrimitiveConversion";
    }

    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;
    //    bool PostconditionForModule(public_lib::Context *ctx, const parser::Program *program) override;
};

}  // namespace ark::es2panda::compiler

#endif
>>>>>>>> origin/OpenHarmony_feature_Release_20250728:ets2panda/compiler/lowering/ets/primitiveConversionPhase.h
