/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { hasDirectivePrologue } from '../../src/directivePrologue';

describe('hasDirectivePrologue', () => {
  describe('directive matching', () => {
    it('matches a semicolon-terminated directive', () => {
      expect(hasDirectivePrologue('"use strict";', 'use strict')).toBe(true);
    });

    it('matches a directive terminated by end of input', () => {
      expect(hasDirectivePrologue('"use strict"', 'use strict')).toBe(true);
    });

    it('matches a single-quoted directive', () => {
      expect(hasDirectivePrologue("'use strict';", 'use strict')).toBe(true);
    });

    it('matches the second directive of the prologue', () => {
      expect(hasDirectivePrologue('"one";\n"use strict";', 'use strict')).toBe(true);
    });

    it('matches directives separated by line breaks without semicolons', () => {
      expect(hasDirectivePrologue('"one"\n"use strict"\n', 'use strict')).toBe(true);
    });

    it('returns false when the directive is absent', () => {
      expect(hasDirectivePrologue('"one";\n"two";', 'use strict')).toBe(false);
    });

    it('returns false for empty input', () => {
      expect(hasDirectivePrologue('', 'use strict')).toBe(false);
    });

    it('returns false when the first statement is not a string literal', () => {
      expect(hasDirectivePrologue('let x = 1;\n"use strict";', 'use strict')).toBe(false);
    });

    it('does not match after the prologue ends', () => {
      expect(hasDirectivePrologue('"one";\nlet x;\n"use strict";', 'use strict')).toBe(false);
    });
  });

  describe('statement boundaries', () => {
    it('rejects a same-line continuation without a semicolon', () => {
      expect(hasDirectivePrologue('"use strict" + value;', 'use strict')).toBe(false);
    });

    it('rejects a parenthesized string literal', () => {
      expect(hasDirectivePrologue('("use strict");', 'use strict')).toBe(false);
    });

    it('rejects a member access after a line break', () => {
      expect(hasDirectivePrologue('"not a directive"\n  .toString();', 'not a directive')).toBe(false);
    });

    it('rejects a tagged template after a line break', () => {
      expect(hasDirectivePrologue('"use strict"\n`tpl`;', 'use strict')).toBe(false);
    });

    it('rejects an as-assertion after a line break', () => {
      expect(hasDirectivePrologue('"use strict"\nas string;', 'use strict')).toBe(false);
    });

    it('rejects an in-expression after a line break', () => {
      expect(hasDirectivePrologue('"a"\nin obj;', 'a')).toBe(false);
    });

    it('accepts a directive followed by ++ on the next line', () => {
      expect(hasDirectivePrologue('"use strict"\n++count;', 'use strict')).toBe(true);
    });

    it('rejects a directive continued by + on the next line', () => {
      expect(hasDirectivePrologue('"use strict"\n+value;', 'use strict')).toBe(false);
    });

    it('accepts a directive followed by a plain statement', () => {
      expect(hasDirectivePrologue('"use strict"\nfoo();', 'use strict')).toBe(true);
    });
  });

  describe('raw matching and escapes', () => {
    it('does not decode escapes', () => {
      expect(hasDirectivePrologue('"use \\x73trict";', 'use strict')).toBe(false);
    });

    it('does not match a directive written with a line continuation', () => {
      // The raw content contains a backslash and a line terminator, so it is
      // not the exact code point sequence the spec requires.
      expect(hasDirectivePrologue("'use \\\nstrict';", 'use strict')).toBe(false);
    });

    it('does not match a directive written with a unicode escape', () => {
      expect(hasDirectivePrologue('"\\u0075se strict";', 'use strict')).toBe(false);
    });

    it('rejects an unterminated string', () => {
      expect(hasDirectivePrologue('"use strict', 'use strict')).toBe(false);
    });

    it('rejects an unescaped line terminator inside the string', () => {
      expect(hasDirectivePrologue('"use\nstrict";', 'use strict')).toBe(false);
    });

    it('skips escaped quotes while scanning', () => {
      expect(hasDirectivePrologue('"a\\"b";\n"use strict";', 'use strict')).toBe(true);
    });

    it('handles CRLF line continuations inside strings', () => {
      expect(hasDirectivePrologue('"a\\\r\nb";\n"use strict";', 'use strict')).toBe(true);
    });
  });

  describe('trivia handling', () => {
    it('skips leading whitespace and comments', () => {
      expect(hasDirectivePrologue('// header\n/* note */ "use strict";', 'use strict')).toBe(true);
    });

    it('keeps scanning directives across comments', () => {
      expect(hasDirectivePrologue('"one"; // line\n/* block */ "use strict";', 'use strict')).toBe(true);
    });

    it('treats a line break inside a block comment as ASI', () => {
      expect(hasDirectivePrologue('"one" /* \n */ "use strict";', 'use strict')).toBe(true);
    });

    it('returns false for an unterminated block comment', () => {
      expect(hasDirectivePrologue('"one"; /* open', 'use strict')).toBe(false);
    });

    it('skips a BOM before the first directive', () => {
      expect(hasDirectivePrologue('\uFEFF"use strict";', 'use strict')).toBe(true);
    });

    it('treats Unicode space separators as white space', () => {
      expect(hasDirectivePrologue('\u202F"use strict";\u00A0', 'use strict')).toBe(true);
      expect(hasDirectivePrologue('"one";\u3000"use strict";', 'use strict')).toBe(true);
    });

    it('treats CRLF as a single line break', () => {
      expect(hasDirectivePrologue('"one";\r\n"use strict";', 'use strict')).toBe(true);
    });
  });

  describe('complex comment scenarios', () => {
    it('does not treat strings or directives inside a line comment as real ones', () => {
      expect(hasDirectivePrologue('// "use static"; fake\n"use static";', 'use static')).toBe(true);
      expect(hasDirectivePrologue('// "use static";\nlet x = 1;', 'use static')).toBe(false);
    });

    it('does not treat strings inside a block comment as real ones', () => {
      expect(hasDirectivePrologue('/* "use static"; */"use static";', 'use static')).toBe(true);
      expect(hasDirectivePrologue('/* "use static"; */ let x = 1;', 'use static')).toBe(false);
    });

    it('does not end a block comment at a single asterisk or slash-star inside the comment', () => {
      expect(hasDirectivePrologue('/* a * b **/ "use static";', 'use static')).toBe(true);
      expect(hasDirectivePrologue('/* /* nested-looking */ "use static";', 'use static')).toBe(true);
    });

    it('ends the comment only at the first real close, not at asterisk-slash inside a string', () => {
      // The '*/' inside the comment body must close the comment; the quoted
      // text after it is a real directive.
      expect(hasDirectivePrologue('/* end: */ "use static";', 'use static')).toBe(true);
    });

    it('handles a line break inside a block comment between two directives', () => {
      expect(hasDirectivePrologue('"one"; /* multi\nline */ "use static";', 'use static')).toBe(true);
    });

    it('keeps scanning past an empty block comment between directives', () => {
      expect(hasDirectivePrologue('"one";/**/"use static";', 'use static')).toBe(true);
    });

    it('requires a real statement separator between the comment and the directive', () => {
      // The comment is followed on the same line by the directive: without a
      // preceding semicolon or line break, "one" is not a standalone statement.
      expect(hasDirectivePrologue('"one" /* c */ "use static";', 'use static')).toBe(false);
    });

    it('treats CRLF inside a block comment as one line break for ASI', () => {
      expect(hasDirectivePrologue('"one"; /* c\r\n */ "use static";', 'use static')).toBe(true);
      // The first directive needs the ASI from the comment's CRLF to stand alone.
      expect(hasDirectivePrologue('"one" /* c\r\n */ "use static";', 'use static')).toBe(true);
    });

    it('still matches a directive followed by an unterminated block comment', () => {
      expect(hasDirectivePrologue('"use static"; /* open', 'use static')).toBe(true);
    });

    it('stops scanning at an unterminated block comment after a non-matching directive', () => {
      expect(hasDirectivePrologue('"one"; /* open', 'use static')).toBe(false);
    });

    it('does not confuse a line comment with a block comment opener', () => {
      // '//*' opens a line comment, not a block comment.
      expect(hasDirectivePrologue('//* /*\n"use static";', 'use static')).toBe(true);
      expect(hasDirectivePrologue('//* /* "use static";', 'use static')).toBe(false);
    });

    it('handles multiple consecutive comments between directives', () => {
      expect(hasDirectivePrologue('"one"; // a\n/* b */ /* c */\n"use static";', 'use static')).toBe(true);
    });

    it('treats a comment containing quotes and a fake terminator after an escape-like sequence correctly', () => {
      // Backslashes in comments are plain characters; only the real */ closes.
      expect(hasDirectivePrologue('/* "quoted \\" text */ "use static";', 'use static')).toBe(true);
      expect(hasDirectivePrologue('/* back\\slash */ "use static";', 'use static')).toBe(true);
    });
  });

  describe('firstDirectiveOnly', () => {
    it('matches when the expected directive is the first one', () => {
      expect(hasDirectivePrologue('"use strict";\n"one";', 'use strict', { firstDirectiveOnly: true })).toBe(true);
    });

    it('matches when the first directive is duplicated later in the prologue', () => {
      expect(hasDirectivePrologue("'use strict';\n'use strict';", 'use strict', { firstDirectiveOnly: true })).toBe(
        true,
      );
      expect(hasDirectivePrologue("'use strict'\n'use strict'", 'use strict', { firstDirectiveOnly: true })).toBe(true);
    });

    it('does not match a directive that is not the first of the prologue', () => {
      expect(hasDirectivePrologue('"one";\n"use strict";', 'use strict', { firstDirectiveOnly: true })).toBe(false);
    });

    it('checks only the first directive across line breaks without semicolons', () => {
      expect(hasDirectivePrologue('"one"\n"use strict"\n', 'use strict', { firstDirectiveOnly: true })).toBe(false);
    });

    it('still matches the second directive when disabled', () => {
      expect(hasDirectivePrologue('"one";\n"use strict";', 'use strict')).toBe(true);
      expect(hasDirectivePrologue('"one";\n"use strict";', 'use strict', { firstDirectiveOnly: false })).toBe(true);
    });

    it('does not scan past the prologue when the first directive does not match', () => {
      expect(hasDirectivePrologue('"one";\nlet x;\n"use strict";', 'use strict', { firstDirectiveOnly: true })).toBe(
        false,
      );
    });

    it('skips comments and shebang before the first directive', () => {
      expect(
        hasDirectivePrologue('#!/usr/bin/env node\n// note\n"use static";', 'use static', {
          parseShebang: true,
          firstDirectiveOnly: true,
        }),
      ).toBe(true);
    });

    it('returns false when the first statement is not a string literal', () => {
      expect(hasDirectivePrologue('let x = 1;\n"use strict";', 'use strict', { firstDirectiveOnly: true })).toBe(
        false,
      );
    });
  });

  describe('shebang handling', () => {
    it('ignores a shebang by default', () => {
      expect(hasDirectivePrologue('#!/usr/bin/env node\n"use strict";', 'use strict')).toBe(false);
    });

    it('skips a shebang when parseShebang is enabled', () => {
      expect(hasDirectivePrologue('#!/usr/bin/env node\n"use strict";', 'use strict', { parseShebang: true })).toBe(
        true,
      );
    });

    it('does not skip a shebang-like token that is not at the start', () => {
      expect(hasDirectivePrologue('  #!/usr/bin/env node\n"use strict";', 'use strict', { parseShebang: true })).toBe(
        false,
      );
    });
  });
});
