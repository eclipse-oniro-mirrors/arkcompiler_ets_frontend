/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_EVALUATE_EVALUATE_CONTEXT_H
#define ES2PANDA_EVALUATE_EVALUATE_CONTEXT_H

#include "util/ustring.h"
#include "util/options.h"

#include "utils/arena_containers.h"
#include "libpandafile/debug_info_extractor.h"
#include "libpandafile/file.h"

#include <memory>
#include <string_view>

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::ir {
class BlockStatement;
class ClassDefinition;
class ExpressionStatement;
class MethodDefinition;
class ScriptFunction;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::evaluate {

struct EvaluateContext {
    explicit EvaluateContext(const util::Options &options)
        : sourceFilePath(options.GetDebuggerEvalSource()), lineNumber(options.GetDebuggerEvalLine())
    {
    }

    /**
     * @brief Searches Program for evaluation method according to convention.
     * Initializes `methodStatements`, which are always non-null after this function,
     * as well as `lastStatement`, which is non-null only if the last statement have expression.
     * Must be called once after parser and before checker phase.
     * @param evalMethodProgram compiler Program corresponding to expression file.
     */
    void FindEvaluationMethod(parser::Program *evalMethodProgram);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    util::StringView sourceFilePath;
    uint64_t lineNumber {0};
    const panda_file::File *file {nullptr};
    std::unique_ptr<panda_file::DebugInfoExtractor> extractor {nullptr};
    panda_file::File::EntityId methodId;
    uint32_t bytecodeOffset {0};

    // es2panda specific information about the compiled expression's static method.
    ir::BlockStatement *methodStatements {nullptr};
    ir::ExpressionStatement *lastStatement {nullptr};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_EVALUATE_CONTEXT_H
