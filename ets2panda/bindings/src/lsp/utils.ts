/*
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

import { logger } from './logger';

const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder('utf-8', { fatal: false });

export const TextPositionUtils = {
  utf16OffsetToCodePointOffset(content: string, utf16Offset: number): number | undefined {
    if (!Number.isInteger(utf16Offset) || utf16Offset < 0 || utf16Offset > content.length) {
      logger.error('UTF-16 offset out of bounds');
      return undefined;
    }

    if (
      utf16Offset > 0 &&
      utf16Offset < content.length &&
      content.charCodeAt(utf16Offset - 1) >= 0xd800 &&
      content.charCodeAt(utf16Offset - 1) <= 0xdbff &&
      content.charCodeAt(utf16Offset) >= 0xdc00 &&
      content.charCodeAt(utf16Offset) <= 0xdfff
    ) {
      logger.error('UTF-16 offset splits a surrogate pair');
      return undefined;
    }

    let codePointOffset = 0;
    for (let index = 0; index < utf16Offset; codePointOffset++) {
      const codePoint = content.codePointAt(index)!;
      index += codePoint > 0xffff ? 2 : 1;
    }
    return codePointOffset;
  },

  codePointOffsetToUtf16Offset(content: string, codePointOffset: number): number | undefined {
    if (!Number.isInteger(codePointOffset) || codePointOffset < 0) {
      logger.error('Code point offset out of bounds');
      return undefined;
    }

    let utf16Offset = 0;
    for (let index = 0; index < codePointOffset; index++) {
      if (utf16Offset >= content.length) {
        logger.error('Code point offset out of bounds');
        return undefined;
      }
      const codePoint = content.codePointAt(utf16Offset)!;
      utf16Offset += codePoint > 0xffff ? 2 : 1;
    }
    return utf16Offset;
  },

  codePointColumnToUtf16Column(content: string, line: number, codePointColumn: number): number | undefined {
    if (!Number.isInteger(line) || !Number.isInteger(codePointColumn) || line < 1 || codePointColumn < 1) {
      logger.error('Code point position out of bounds');
      return undefined;
    }

    const lineContent = content.split(/\r\n|[\n\r\u2028\u2029]/)[line - 1];
    if (lineContent === undefined) {
      logger.error('Code point position out of bounds');
      return undefined;
    }

    const utf16Column = this.codePointOffsetToUtf16Offset(lineContent, codePointColumn - 1);
    if (utf16Column === undefined) {
      return undefined;
    }
    return utf16Column + 1;
  },

  // char offset -> byte offset
  charOffsetToByteOffset(content: string, charOffset: number): number | undefined {
    if (charOffset < 0 || charOffset > content.length) {
      logger.error('Character offset out of bounds');
      return;
    }

    return textEncoder.encode(content.substring(0, charOffset)).length;
  },

  // byte offset -> char offset
  byteOffsetToCharOffset(content: string, byteOffset: number): number | undefined {
    const encoderResult = textEncoder.encode(content);

    if (byteOffset < 0 || byteOffset > encoderResult.length) {
      logger.error('Byte offset out of bounds');
      return;
    }

    const buffer = encoderResult.subarray(0, byteOffset);
    return textDecoder.decode(buffer).length;
  }
};

export function formEts2pandaCmd(
  defaultArkTsConfig: string,
  input: string,
  simultaneous: boolean = false,
  extraOptions: string[] = []
): string[] {

  const ets2pandaCmd: string[] = [
      '-',
      '--extension',
      'ets',
      '--arktsconfig',
      defaultArkTsConfig
  ]

  if (simultaneous) {
      ets2pandaCmd.push('--simultaneous')
  }

  ets2pandaCmd.push(...extraOptions)
  ets2pandaCmd.push(input)
  return ets2pandaCmd
}
