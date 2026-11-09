/*
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

// Generates dynhar/decl-fileInfo.json with absolute paths into the OS tmp
// directory, and prints the generated file path to stdout so that callers can
// export it as INCREMENT_DYNAMIC_DEMO_DECL_FILEINFO. The build system locates
// the file via "declFilesPath" in build_config.json. The tmp subdirectory is
// derived from this demo's absolute path, so it stays stable within one build
// chain while distinct checkouts do not collide.
const fs = require('fs');
const os = require('os');
const path = require('path');
const crypto = require('crypto');

const demoDir = __dirname;
const tmpDir = path.join(
    os.tmpdir(),
    `increment-dynamic-demo-${crypto.createHash('md5').update(demoDir).digest('hex').slice(0, 8)}`
);
const outFile = path.join(tmpDir, 'decl-fileInfo.json');

fs.mkdirSync(tmpDir, { recursive: true });
fs.writeFileSync(outFile, JSON.stringify({
    packageName: 'dynhar',
    files: {
        index: {
            declPath: path.join(demoDir, 'dynhar', 'index.d.ets'),
            filePath: path.join(demoDir, 'dynhar', 'index.ets'),
            ohmUrl: '@normalized:N&entry&com.example.incrementdynamic&dynhar/index&1.0.0'
        }
    }
}, null, 4) + '\n');
console.error(`[gen_decl_fileinfo] generated ${outFile}`);
console.log(outFile);
