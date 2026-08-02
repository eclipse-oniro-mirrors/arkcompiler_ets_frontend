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

import { promises as fs } from 'node:fs';

const ETS_SOURCE_SUFFIX = '.ets';
const USE_STATIC_DIRECTIVE = "'use static'";

/** Bytes inspected from the start of an ETS source file when classifying it. */
export const STATIC_SOURCE_PROBE_BYTES = 256;

/** Whether a path uses the lowercase ETS source extension accepted by gluegen. */
export function hasEtsSourceExtension(filePath: string): boolean {
  return filePath.endsWith(ETS_SOURCE_SUFFIX);
}

/** Whether the first trimmed source line is exactly the ArkTS static directive. */
export function hasUseStaticDirective(sourcePrefix: string): boolean {
  const firstLine = sourcePrefix.split(/\r?\n/, 1)[0]?.trim();
  return firstLine === USE_STATIC_DIRECTIVE;
}

/** Whether the beginning of an ETS source file declares ArkTS static mode. */
export async function hasUseStaticDirectiveInFile(filePath: string): Promise<boolean> {
  const file = await fs.open(filePath, 'r');
  try {
    const buffer = Buffer.allocUnsafe(STATIC_SOURCE_PROBE_BYTES);
    const { bytesRead } = await file.read(buffer, 0, buffer.length, 0);
    return hasUseStaticDirective(buffer.toString('utf8', 0, bytesRead));
  } finally {
    await file.close();
  }
}
