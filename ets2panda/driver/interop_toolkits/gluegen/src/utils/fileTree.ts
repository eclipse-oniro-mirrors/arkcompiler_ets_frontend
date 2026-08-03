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
import * as path from 'node:path';

type FilePredicate = (filePath: string) => boolean | Promise<boolean>;

export async function collectFiles(rootPath: string, accepts: FilePredicate): Promise<readonly string[]> {
  const files: string[] = [];
  await collectDirectoryFiles(rootPath, accepts, files);
  return files;
}

export async function pathExists(filePath: string): Promise<boolean> {
  try {
    await fs.access(filePath);
    return true;
  } catch {
    return false;
  }
}

async function collectDirectoryFiles(directoryPath: string, accepts: FilePredicate, files: string[]): Promise<void> {
  const entries = await fs.readdir(directoryPath);
  for (const entry of entries) {
    const entryPath = path.join(directoryPath, entry);
    const stat = await fs.stat(entryPath);
    if (stat.isDirectory()) {
      await collectDirectoryFiles(entryPath, accepts, files);
    } else if (stat.isFile() && (await accepts(entryPath))) {
      files.push(entryPath);
    }
  }
}
