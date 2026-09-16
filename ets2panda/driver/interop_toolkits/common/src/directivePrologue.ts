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

export interface HasDirectivePrologueOptions {
  /**
   * Whether to recognize and skip a shebang at the very start of the text.
   *
   * @default false
   */
  parseShebang?: boolean;

  /**
   * Whether to only test the first directive of the prologue. When disabled,
   * every directive of the prologue is scanned until `expected` is found.
   *
   * @default false
   */
  firstDirectiveOnly?: boolean;
}

interface TriviaResult {
  position: number;
  sawLineBreak: boolean;
  valid: boolean;
}

/**
 * Checks whether the Directive Prologue at the start of the text contains the
 * given directive.
 *
 * `expected` is the raw text without quotes. String escapes are not decoded:
 *
 *     hasDirectivePrologue('"use strict";', 'use strict'); // true
 *     hasDirectivePrologue('"use \\x73trict";', 'use strict'); // false
 *
 * Scanning stops as soon as the directive is found.
 */
export function hasDirectivePrologue(
  source: string,
  expected: string,
  options: HasDirectivePrologueOptions = {},
): boolean {
  let position = 0;

  // A shebang is only recognized at the absolute start of the text; without
  // this option a leading "#!" fails the string-literal scan below.
  if (options.parseShebang === true && source.startsWith('#!')) {
    position = scanToLineEnd(source, 2);
  }

  const initialTrivia = skipTrivia(source, position);
  if (!initialTrivia.valid) {
    return false;
  }
  position = initialTrivia.position;

  while (position < source.length) {
    const literalStart = position;
    const literalEnd = scanStringLiteral(source, literalStart);

    // The prologue ends at the first statement that is not a string literal.
    if (literalEnd === -1) {
      return false;
    }

    const trailingTrivia = skipTrivia(source, literalEnd);
    if (!trailingTrivia.valid) {
      return false;
    }

    const nextPosition = trailingTrivia.position;

    // A string literal is only a directive when it stands alone as a statement.
    if (!isDirectiveStatementBoundary(source, nextPosition, trailingTrivia.sawLineBreak)) {
      return false;
    }

    // Compare the raw content without creating a temporary string.
    if (matchesDirective(source, literalStart, literalEnd, expected)) {
      return true;
    }

    // With firstDirectiveOnly the scan stops after the first directive.
    if (options.firstDirectiveOnly === true) {
      return false;
    }

    // Consume an explicit semicolon; with ASI, nextPosition is already at the
    // next non-trivia token.
    if (source.charCodeAt(nextPosition) === 0x3b) {
      position = nextPosition + 1;
    } else {
      position = nextPosition;
    }

    const trivia = skipTrivia(source, position);
    if (!trivia.valid) {
      return false;
    }
    position = trivia.position;
  }

  return false;
}

/**
 * Scans a single- or double-quoted string literal starting at `start`.
 * Returns the position after the closing quote, or -1 when `start` does not
 * begin a string literal or the literal is unterminated.
 */
function scanStringLiteral(source: string, start: number): number {
  const quote = source.charCodeAt(start);
  if (quote !== 0x22 && quote !== 0x27) {
    return -1;
  }

  let position = start + 1;
  while (position < source.length) {
    const character = source.charCodeAt(position);

    if (character === quote) {
      return position + 1;
    }

    if (character === 0x5c) {
      // Escape: skip the backslash and the escaped character; a CRLF line
      // continuation is skipped as one unit.
      position++;
      if (position >= source.length) {
        return -1;
      }
      if (source.charCodeAt(position) === 0x0d && source.charCodeAt(position + 1) === 0x0a) {
        position += 2;
      } else {
        position++;
      }
      continue;
    }

    // An unescaped line terminator cannot appear inside a string literal.
    if (isLineTerminator(character)) {
      return -1;
    }

    position++;
  }

  return -1;
}

/** Skips whitespace and comments; `sawLineBreak` feeds the ASI boundary check. */
function skipTrivia(source: string, start: number): TriviaResult {
  let position = start;
  let sawLineBreak = false;
  let valid = true;

  while (position < source.length) {
    const character = source.charCodeAt(position);

    if (isWhitespace(character)) {
      position++;
      continue;
    }

    if (isLineTerminator(character)) {
      sawLineBreak = true;
      position = skipLineBreak(source, position);
      continue;
    }

    if (character === 0x2f && source.charCodeAt(position + 1) === 0x2f) {
      position = scanToLineEnd(source, position + 2);
      continue;
    }

    if (character === 0x2f && source.charCodeAt(position + 1) === 0x2a) {
      // An unterminated block comment ends the scan at the input's end, so
      // the loop condition exits with `valid` false.
      const block = scanBlockComment(source, position + 2);
      sawLineBreak = sawLineBreak || block.sawLineBreak;
      position = block.position;
      valid = block.valid;
      continue;
    }

    break;
  }

  return { position, sawLineBreak, valid };
}

/** Returns the position after the line terminator at `position`; CRLF counts as one. */
function skipLineBreak(source: string, position: number): number {
  const character = source.charCodeAt(position);
  if (character === 0x0d && source.charCodeAt(position + 1) === 0x0a) {
    return position + 2;
  }
  return position + 1;
}

/**
 * Scans a block comment starting after its `/*` opener. `valid` is false when
 * the comment is unterminated.
 */
function scanBlockComment(source: string, start: number): TriviaResult {
  let position = start;
  let sawLineBreak = false;

  while (position < source.length) {
    const current = source.charCodeAt(position);

    if (isLineTerminator(current)) {
      sawLineBreak = true;
      position = skipLineBreak(source, position);
      continue;
    }

    if (current === 0x2a && source.charCodeAt(position + 1) === 0x2f) {
      return { position: position + 2, sawLineBreak, valid: true };
    }

    position++;
  }

  return { position, sawLineBreak, valid: false };
}

/** Determines whether the string literal can end its statement at `position`. */
function isDirectiveStatementBoundary(source: string, position: number, sawLineBreak: boolean): boolean {
  // Explicit semicolon.
  if (source.charCodeAt(position) === 0x3b) {
    return true;
  }

  // End of input triggers ASI.
  if (position >= source.length) {
    return true;
  }

  // Without a semicolon or a line break the literal cannot stand alone,
  // e.g. "directive" + value.
  if (!sawLineBreak) {
    return false;
  }

  // A line break only ends the statement when the next token cannot continue the expression, e.g. "not a directive".toString();
  return !canContinueStringExpression(source, position);
}

/**
 * Conservatively determines whether the token at `position` can continue the
 * preceding string expression. Not a full parser; covers the continuations
 * relevant to directive-prologue detection.
 */
function canContinueStringExpression(source: string, position: number): boolean {
  const character = source.charCodeAt(position);
  const next = source.charCodeAt(position + 1);

  switch (character) {
    // Call, index, property access, tagged template, and binary, conditional,
    // or assignment operators.
    case 0x28: // (
    case 0x5b: // [
    case 0x2e: // .
    case 0x60: // `
    case 0x2a: // *
    case 0x2f: // /
    case 0x25: // %
    case 0x3c: // <
    case 0x3e: // >
    case 0x3d: // =
    case 0x21: // !
    case 0x26: // &
    case 0x7c: // |
    case 0x5e: // ^
    case 0x3f: // ?
    case 0x2c: // ,
      return true;

    case 0x2b: // +: a ++ after a line break cannot be a suffix operator, so ASI applies.
      return next !== 0x2b;

    case 0x2d: // -: same as ++.
      return next !== 0x2d;

    default:
      break;
  }

  const word = scanAsciiIdentifier(source, position);

  // Keywords that continue the preceding expression.
  return word === 'as' || word === 'satisfies' || word === 'in' || word === 'instanceof';
}

/** Compares the raw literal content without decoding escapes or slicing. */
function matchesDirective(source: string, literalStart: number, literalEnd: number, expected: string): boolean {
  // literalEnd points past the closing quote, so drop both quotes.
  const contentLength = literalEnd - literalStart - 2;

  if (contentLength !== expected.length) {
    return false;
  }

  for (let index = 0; index < expected.length; index++) {
    if (source.charCodeAt(literalStart + index + 1) !== expected.charCodeAt(index)) {
      return false;
    }
  }

  return true;
}

function scanAsciiIdentifier(source: string, start: number): string | undefined {
  if (!isAsciiIdentifierStart(source.charCodeAt(start))) {
    return undefined;
  }

  let position = start + 1;
  while (position < source.length && isAsciiIdentifierPart(source.charCodeAt(position))) {
    position++;
  }

  return source.slice(start, position);
}

function isAsciiIdentifierStart(character: number): boolean {
  return (
    character === 0x24 || // $
    character === 0x5f || // _
    (character >= 0x41 && character <= 0x5a) || // A-Z
    (character >= 0x61 && character <= 0x7a) // a-z
  );
}

function isAsciiIdentifierPart(character: number): boolean {
  return isAsciiIdentifierStart(character) || (character >= 0x30 && character <= 0x39);
}

function isWhitespace(character: number): boolean {
  return (
    character === 0x0009 || // Tab
    character === 0x000b || // Vertical Tab
    character === 0x000c || // Form Feed
    character === 0x0020 || // Space
    character === 0x00a0 || // No-Break Space
    character === 0x1680 || // Ogham Space Mark
    (character >= 0x2000 && character <= 0x200a) ||
    character === 0x202f || // Narrow No-Break Space
    character === 0x205f || // Medium Mathematical Space
    character === 0x3000 || // Ideographic Space
    character === 0xfeff // BOM / Zero Width No-Break Space
  );
}

function isLineTerminator(character: number): boolean {
  return (
    character === 0x000a || // LF
    character === 0x000d || // CR
    character === 0x2028 || // Line Separator
    character === 0x2029 // Paragraph Separator
  );
}

function scanToLineEnd(source: string, start: number): number {
  let position = start;
  while (position < source.length && !isLineTerminator(source.charCodeAt(position))) {
    position++;
  }
  return position;
}
