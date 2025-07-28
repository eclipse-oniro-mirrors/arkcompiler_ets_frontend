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

#ifndef ES2PANDA_PARSER_CORE_INNER_SOURCE_PARSER_H
#define ES2PANDA_PARSER_CORE_INNER_SOURCE_PARSER_H

namespace ark::es2panda::lexer {
class Lexer;
}  // namespace ark::es2panda::lexer

namespace ark::es2panda::util {
class StringView;
}  // namespace ark::es2panda::util

namespace ark::es2panda::parser {
class ETSParser;

class InnerSourceParser {
public:
    explicit InnerSourceParser(ETSParser *parser);
    NO_COPY_SEMANTIC(InnerSourceParser);
    NO_MOVE_SEMANTIC(InnerSourceParser);

    ~InnerSourceParser();

    void *operator new(size_t) = delete;
    void *operator new[](size_t) = delete;

private:
    ETSParser *parser_;
    lexer::Lexer *savedLexer_;
    util::StringView savedSourceCode_ {};
    util::Path savedSourceFile_ {};
    util::StringView savedSourceFilePath_ {};
    util::StringView savedRelativeFilePath_ {};
};
}  // namespace ark::es2panda::parser

#endif
