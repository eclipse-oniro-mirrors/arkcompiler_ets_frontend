/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "public/es2panda_lib.h"
#include "util.h"

constexpr int PROCEED_ERROR_PARSER = NULLPTR_CONTEXT_ERROR_CODE + 1;
constexpr int PROCEED_ERROR_BOUND = PROCEED_ERROR_PARSER + 1;
constexpr int TEST_ERROR_PARSER = PROCEED_ERROR_BOUND + 1;
constexpr int TEST_ERROR_BOUND = TEST_ERROR_PARSER + 1;
constexpr int TEST_ERROR_CHECKED = TEST_ERROR_BOUND + 1;

int main()
{
    es2panda_Impl *impl = GetImpl();
    const char *arg1 = "es2panda";
    const char *arg2 = "test.ats";
    char const *const args[] = {arg1, arg2};
    auto config = impl->CreateConfig(2, args);
    auto context = impl->CreateContextFromString(config, "fn main() { println!(\"Hello, Rust!\") }", arg2);
    if (context == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    if (impl->ContextState(context) != ES2PANDA_STATE_PARSED) {  // Parsed, but
        return PROCEED_ERROR_PARSER;
    }
    if (!impl->IsAnyError(context)) {  // have errors reported
        return TEST_ERROR_PARSER;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);
    if (impl->ContextState(context) != ES2PANDA_STATE_BOUND) {  // Bound, but
        return PROCEED_ERROR_BOUND;
    }
    if (!impl->IsAnyError(context)) {  // have errors reported
        return TEST_ERROR_BOUND;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_CHECKED);
    // Will not proceed after checker if there are errors reported
    if (impl->ContextState(context) != ES2PANDA_STATE_ERROR) {
        return TEST_ERROR_CHECKED;
    }
    return 0;
}
