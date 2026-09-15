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

#ifndef ES2PANDA_CHECKER_ETS_EXPORTED_OVERLOAD_RESOLVER_H
#define ES2PANDA_CHECKER_ETS_EXPORTED_OVERLOAD_RESOLVER_H

namespace ark::es2panda::checker {
class ETSChecker;
class Type;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class AstNode;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
class LocalVariable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::checker::ets {

struct ExportedOverloadView {
    Type *type;
    bool differsFromSource;
};

ExportedOverloadView ResolveExportedOverloadView(ETSChecker *checker, Type *sourceType, varbinder::LocalVariable *owner,
                                                 const ir::AstNode *exportOrigin, bool exportsWholeBinding = false);

}  // namespace ark::es2panda::checker::ets

#endif  // ES2PANDA_CHECKER_ETS_EXPORTED_OVERLOAD_RESOLVER_H
