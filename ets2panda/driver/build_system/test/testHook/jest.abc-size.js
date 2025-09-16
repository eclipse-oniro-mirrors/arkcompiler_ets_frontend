/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

const fs = require('fs');
const path = require('path');

function getAllAbcFiles(dir) {
  let results = [];
  if (!fs.existsSync(dir)) return results;
  const list = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of list) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      results = results.concat(getAllAbcFiles(fullPath));
    } else if (entry.isFile() && entry.name.endsWith('.abc')) {
      results.push(fullPath);
    }
  }
  return results;
}

function formatSize(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(2)} KB`;
  return `${(bytes / 1024 / 1024).toFixed(2)} MB`;
}

afterAll(() => {
  const cacheDir = path.resolve(__dirname, '../../dist/cache');
  const abcFiles = getAllAbcFiles(cacheDir);
  if (abcFiles.length === 0) {
    console.warn(`[Jest][No .abc files found in ${cacheDir}]`);
    return;
  }
  abcFiles.forEach(file => {
    const size = fs.statSync(file).size;
    console.log(`[Jest][${file}] size: ${formatSize(size)}`);
  });
});
