/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FORMATTING_H
#define FORMATTING_H

#include <vector>
#include "formatting_settings.h"
#include "public/es2panda_lib.h"
#include "rules_map.h"
#include "lsp/include/types.h"

namespace ark::es2panda::lsp {

struct FormatContext {
public:
    explicit FormatContext(FormatCodeSettings &formatCodeSetting, RulesMap &rulesMap)
        : options_(formatCodeSetting), getRules_(rulesMap)
    {
    }

    FormatCodeSettings &GetFormatCodeSettings()
    {
        return options_;
    }

    RulesMap &GetRulesMap()
    {
        return getRules_;
    }

private:
    FormatCodeSettings options_;
    RulesMap getRules_;
};

CAPI_EXPORT FormatContext GetFormatContext(FormatCodeSettings &options);
std::vector<TextChange> FormatDocument(es2panda_Context *context, FormatContext formatContext);
std::vector<TextChange> FormatRange(es2panda_Context *context, FormatContext formatContext, const TextSpan &span);

}  // namespace ark::es2panda::lsp

#endif