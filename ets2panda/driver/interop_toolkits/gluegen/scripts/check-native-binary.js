#!/usr/bin/env node
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

// Runs as the "prepack" lifecycle script (npm pack / npm publish) to make
// sure the prebuilt gluegen binary was copied into bin/ before the tarball is
// assembled. Without this check, a missing native build would silently ship a
// broken package (files/bin/* would just not exist).

'use strict';

const fs = require('fs');
const path = require('path');

const BIN_DIR = path.resolve(__dirname, '..', 'bin');
const BINARY_NAMES = ['gluegen', 'gluegen.exe'];

const found = BINARY_NAMES.find((name) => fs.existsSync(path.join(BIN_DIR, name)));

if (!found) {
  console.error(
    `[gluegen] prepack check failed: no native binary found in ${BIN_DIR}\n` +
      '[gluegen] build it first (e.g. via ark.py / ninja) so one of the following exists:\n' +
      BINARY_NAMES.map((name) => `  - ${path.join(BIN_DIR, name)}`).join('\n')
  );
  process.exit(1);
}

console.log(`[gluegen] prepack check ok: found ${path.join(BIN_DIR, found)}`);
