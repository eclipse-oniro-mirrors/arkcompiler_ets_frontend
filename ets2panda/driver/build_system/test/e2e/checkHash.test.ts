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

// Depends on hardware environment, set a longer timeout
jest.setTimeout(20000);

import fs from 'fs';
import path from 'path';
import crypto from 'crypto';

function getAllModules(config: any) {
  return [
    { packageName: config.packageName, modulePath: config.moduleRootPath, sourceRoots: config.sourceRoots || ['./'] },
    ...(config.dependencyModuleList || [])
  ];
}

function getAllSrcFiles(modules: any[]) {
  let allSrcFiles: string[] = [];
  for (const mod of modules) {
    const moduleAbsPath = path.resolve(__dirname, '../../', mod.modulePath || mod.moduleRootPath || '');
    for (const root of mod.sourceRoots || ['./']) {
      const srcRoot = path.resolve(moduleAbsPath, root);
      if (fs.existsSync(srcRoot) && fs.statSync(srcRoot).isDirectory()) {
        const files = fs.readdirSync(srcRoot)
          .filter(f => f.endsWith('.ets'))
          .map(f => path.join(srcRoot, f));
        allSrcFiles = allSrcFiles.concat(files);
      }
    }
  }
  return allSrcFiles;
}

function calcFileHash(filePath: string): string {
  const content = fs.readFileSync(filePath);
  return crypto.createHash('sha256').update(content).digest('hex');
}

function getScriptConfig(testScriptName: string): { configPath: string, config: any } {
  const pkgJsonPath = path.resolve(__dirname, '../../package.json');
  let scripts: Record<string, string> = {};
  if (fs.existsSync(pkgJsonPath)) {
    try {
      const pkg = JSON.parse(fs.readFileSync(pkgJsonPath, 'utf-8'));
      scripts = pkg.scripts || {};
    } catch {
      scripts = {};
    }
  }
  if (!scripts[testScriptName]) {
    throw new Error(`TEST environment variable "${testScriptName}" is not a valid script name in package.json`);
  }
  const match = scripts[testScriptName].match(/node\s+[^\s]+\s+([^\s]+\.json)/);
  if (!match) {
    throw new Error(`Script "${testScriptName}" does not contain a node command`);
  }
  const configPath = path.resolve(__dirname, '../../', match[1]);
  if (!fs.existsSync(configPath)) {
    throw new Error(`Config file not found: ${configPath}`);
  }
  const config = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
  return { configPath, config };
}

function getHashCache(cachePath: string): Record<string, string> | null {
  const hashCachePath = path.join(cachePath, 'hash_cache.json');
  if (!fs.existsSync(hashCachePath)) {
    console.warn(`hash_cache.json: ${hashCachePath} does not exist. Maybe clean build?`);
    return null;
  }
  const rawHashCache = JSON.parse(fs.readFileSync(hashCachePath, 'utf-8'));
  const hashCache: Record<string, string> = {};
  for (const key in rawHashCache) {
    hashCache[path.resolve(key)] = rawHashCache[key];
  }
  return hashCache;
}

function getDependencyInfo(cachePath: string) {
  const depPath = path.join(cachePath, 'dependency.json');
  if (!fs.existsSync(depPath)) {
    console.warn(`dependency.json: ${depPath} does not exist.`);
    return { dependants: {} };
  }
  try {
    const depJson = JSON.parse(fs.readFileSync(depPath, 'utf-8'));
    return { dependants: depJson.dependants || {} };
  } catch {
    return { dependants: {} };
  }
}

function collectIncrementalCompileFiles(
  changedFiles: string[],
  dependants: Record<string, string[]>
): Set<string> {
  const result = new Set<string>();
  const queue = [...changedFiles];
  const addDep = (dep: string) => {
    if (!result.has(dep)) {
      queue.push(dep);
    }
  };
  while (queue.length > 0) {
    const file = queue.shift()!;
    if (!result.has(file)) {
      result.add(file);
      (dependants[file] || []).forEach(addDep);
    }
  }
  return result;
}

function checkFileHashWithDependency(
  allSrcFiles: string[],
  hashCache: Record<string, string>,
  dependants: Record<string, string[]>
) {
  const changedFiles: string[] = [];
  allSrcFiles.forEach(srcFile => {
    const absPath = path.resolve(srcFile);
    const hashInCache = hashCache[absPath];
    const actualHash = calcFileHash(absPath);
    if (!hashInCache) {
      console.log(`${absPath} doesn't participate in incremental compilation`);
    } else {
      if (actualHash !== hashInCache) {
        console.log(`[Jest][${absPath} changed,  hash:\nnow (${actualHash})\nprevious (${hashInCache})]`);
        changedFiles.push(absPath);
      } else {
        console.log(`[Jest][${absPath} unchanged, hash: ${actualHash}]`);
      }
    }
  });

  const incrementalFiles = collectIncrementalCompileFiles(changedFiles, dependants);

  if (incrementalFiles.size > 0) {
    console.log('\n[Jest][Incremental compilation will be triggered for the following files]:');
    incrementalFiles.forEach(f => console.log(f));
  } else {
    console.log('\n[Jest][No incremental compilation files found, all source files are unchanged]');
  }
}

function testHelper(testScriptName: string) {
  const { configPath, config } = getScriptConfig(testScriptName);
  const cachePath = path.resolve(__dirname, '../../', config.cachePath);

  describe(`Output Artifact Check [${configPath}]`, () => {
    it('Check hash of all source files with hash_cache.json and analyze all incremental compilation files', () => {
      const hashCache = getHashCache(cachePath);
      if (!hashCache) return;
      const modules = getAllModules(config);
      const allSrcFiles = getAllSrcFiles(modules);
      const { dependants } = getDependencyInfo(cachePath);
      checkFileHashWithDependency(allSrcFiles, hashCache, dependants);
    });
  });
}

const testScriptName = process.env.TEST;
if (!testScriptName) {
  throw new Error('Set the TEST environment variable to specify a script name');
} else {
  testHelper(testScriptName);
}
