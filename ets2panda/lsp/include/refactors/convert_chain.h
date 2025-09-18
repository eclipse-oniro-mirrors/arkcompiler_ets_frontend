/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 */

#ifndef CONVERT_TO_OPTIONAL_CHAIN_EXPRESSION_H
#define CONVERT_TO_OPTIONAL_CHAIN_EXPRESSION_H

#include "refactor_types.h"
namespace ark::es2panda::lsp {
constexpr RefactorActionView TO_OPTIONAL_CHAIN_ACTION {"Convert to optional chain expression",
                                                       "Convert logical AND/conditional chains to optional chaining",
                                                       "refactor.rewrite.expression.optionalChain"};

/**
 * @brief Refactor for converting &&-based or conditional access patterns
 *        into `?.` / `??` optional chaining expressions.
 *
 * ### Example
 * - Before: `a && a.b && a.b.c()`
 * - After:  `a?.b?.c()`
 *
 * - Before: `a && a.b ? a.b.c : d`
 * - After:  `a?.b?.c ?? d`
 */
class ConvertToOptionalChainExpressionRefactor : public Refactor {
public:
    ConvertToOptionalChainExpressionRefactor();

    /**
     * @brief Determines whether the `"Convert to optional chain expression"`
     *        action is available at the current selection.
     */
    ApplicableRefactorInfo GetAvailableActions(const RefactorContext &context) const override;

    /**
     * @brief Produces edits that replace the original logical/conditional chain
     *        with equivalent optional chaining syntax.
     */
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};
}  // namespace ark::es2panda::lsp

#endif  // CONVERT_TO_OPTIONAL_CHAIN_EXPRESSION_H