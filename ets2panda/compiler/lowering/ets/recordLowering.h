/**
 * Copyright (c) 2023 - 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_LOWERING_RECORD_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_RECORD_LOWERING_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {
class RecordLowering : public PhaseForBodies {
public:
    std::string_view Name() const override;
    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;

private:
    using KeyType = std::variant<int32_t, int64_t, float, double, util::StringView>;
    using KeySetType = std::unordered_set<KeyType>;

    ir::Expression *UpdateObjectExpression(ir::ObjectExpression *expr, public_lib::Context *ctx);

    // Helpers
    ir::Expression *CreateBlockExpression(ir::ObjectExpression *expr, checker::Type *keyType, checker::Type *valueType,
                                          public_lib::Context *ctx);
    std::string TypeToString(checker::Type *type) const;
    KeyType TypeToKey(checker::Type *type) const;
    void CheckDuplicateKey(KeySetType &keySet, ir::ObjectExpression *expr, public_lib::Context *ctx);
    void CheckLiteralsCompleteness(KeySetType &keySet, ir::ObjectExpression *expr, public_lib::Context *ctx);
    ir::Statement *CreateStatement(const std::string &src, ir::Expression *ident, ir::Expression *key,
                                   ir::Expression *value, public_lib::Context *ctx);
    void CheckKeyType(checker::ETSChecker *checker, checker::Type const *keyType, ir::ObjectExpression const *expr,
                      public_lib::Context *ctx) const noexcept;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_RECORD_LOWERING_H
