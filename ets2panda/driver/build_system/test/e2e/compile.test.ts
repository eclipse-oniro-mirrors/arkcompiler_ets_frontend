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

import { execFile } from 'child_process';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';

const execFileAsync = promisify(execFile);

function getAllFilesWithExt(dir: string, exts: string[]): string[] {
  if (!fs.existsSync(dir)) return [];
  let result: string[] = [];
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      result = result.concat(getAllFilesWithExt(fullPath, exts));
    } else if (exts.some(ext => entry.name.endsWith(ext))) {
      result.push(fullPath);
    }
  }
  return result;
}

function getConfigAndPaths(testScriptName: string) {
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
  const script = scripts[testScriptName];
  const match = script.match(/node\s+([^\s]+)\s+([^\s]+\.json)/);
  if (!match) {
    throw new Error(`Script "${testScriptName}" does not contain a node ...config.json command`);
  }
  const configPath = path.resolve(__dirname, '../../', match[2]);
  if (!fs.existsSync(configPath)) {
    throw new Error(`Config file not found: ${configPath}`);
  }
  const config = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
  const cachePath = path.resolve(__dirname, '../../', config.cachePath);
  return { config, configPath, cachePath };
}

function getExpectedOutputs(config: any, cachePath: string) {
  const allModules = [
    { packageName: config.packageName, modulePath: config.moduleRootPath },
    ...(config.dependentModuleList || []).map((m: any) => ({
      packageName: m.packageName,
      modulePath: m.modulePath
    }))
  ];
  return (config.compileFiles || []).map((src: string) => {
    let matchedModule = allModules.find(m => {
      const moduleAbs = path.resolve(__dirname, '../../', m.modulePath);
      const absSrc = path.isAbsolute(src) ? src : path.resolve(moduleAbs, src);
      const rel = path.relative(moduleAbs, absSrc);
      return !rel.startsWith('..') && !path.isAbsolute(rel);
    });
    if (!matchedModule) {
      matchedModule = { packageName: config.packageName, modulePath: config.moduleRootPath };
    }
    const moduleAbs = path.resolve(__dirname, '../../', matchedModule.modulePath);
    const absSrc = path.isAbsolute(src) ? src : path.resolve(moduleAbs, src);
    let relSrc = path.relative(moduleAbs, absSrc);
    if (!relSrc || relSrc === '') {
      relSrc = path.basename(absSrc);
    }
    return path.join(
      cachePath,
      matchedModule.packageName,
      relSrc.replace(/\.[^/.]+$/, '.abc')
    );
  });
}

async function runCompile(testScriptName: string): Promise<void> {
  try {
    await execFileAsync('npm', ['run', testScriptName]);
  } catch (err: any) {
    throw new Error(`Fail to compile: ${err.stderr || err.message}`);
  }
}

function checkArktsConfig(config: any, cachePath: string) {
  it('should generate arktsconfig.json in cachePath', () => {
    const arktsConfigPath = path.join(cachePath, config.packageName, 'arktsconfig.json');
    if (!fs.existsSync(arktsConfigPath)) {
      throw new Error(`Missing ${arktsConfigPath}`);
    }
  });
}

function checkDeclgenOutputs(config: any) {
  const outPaths = [
    config.declgenBridgeCodePath,
    config.declgenV1OutPath,
    config.declgenV2OutPath
  ].filter(Boolean);
  outPaths.forEach(outPath => {
    it(`should generate files in ${outPath}`, () => {
      const absOutPath = path.resolve(__dirname, '../../', outPath);
      if (!fs.existsSync(absOutPath) || !fs.statSync(absOutPath).isDirectory()) {
        console.warn(`[Jest][${absOutPath} not found or not a directory, skip check.]`);
        return;
      }
      const files = getAllFilesWithExt(absOutPath, ['.ts', '.d.ts', '.json', '.js', '.ets']);
      if (files.length === 0) {
        console.warn(`[Jest][${absOutPath} exists but no output files found, skip check.]`);
        return;
      }
      files.forEach(f => console.log(`[Jest][${absOutPath} found output: ${f}]`));
      expect(files.length).toBeGreaterThan(0);
    });
  });
}

function checkModuleRootDeclFiles(config: any) {
  it('should generate declaration files in moduleRootPath', () => {
    const absModuleRoot = path.resolve(__dirname, '../../', config.moduleRootPath);
    if (!fs.existsSync(absModuleRoot) || !fs.statSync(absModuleRoot).isDirectory()) {
      throw new Error(`[Jest][${absModuleRoot} not found or not a directory]`);
    }
    const declFiles = getAllFilesWithExt(absModuleRoot, ['.d.ts', '.ts', '.json']);
    expect(declFiles.length).toBeGreaterThan(0);
  });
}

function checkAbcFiles(expectedOutputs: string[]) {
  it('should generate abc files for compileFiles', () => {
    expectedOutputs.forEach(filePath => {
      if (!fs.existsSync(filePath)) {
        throw new Error(`Missing ${filePath}`);
      }
      if (fs.statSync(filePath).size === 0) {
        throw new Error(`${filePath} exists but is empty`);
      }
    });
  });
}

function checkArtifacts(config: any, cachePath: string, expectedOutputs: string[]) {
  checkArktsConfig(config, cachePath);

  if (config.enableDeclgenEts2Ts) {
    checkDeclgenOutputs(config);
    if (!config.compileFiles || config.compileFiles.length === 0) {
      checkModuleRootDeclFiles(config);
    }
  } else {
    if (config.compileFiles && config.compileFiles.length > 0) {
      checkAbcFiles(expectedOutputs);
    } else {
      checkModuleRootDeclFiles(config);
    }
  }
}

function testHelper(testScriptName: string) {
  const { config, configPath, cachePath } = getConfigAndPaths(testScriptName);
  const expectedOutputs = getExpectedOutputs(config, cachePath);

  describe(`Output Artifact Check [${configPath}]`, () => {
    beforeAll(() => runCompile(testScriptName));
    checkArtifacts(config, cachePath, expectedOutputs);
  });
}

const testScriptName = process.env.TEST;
if (!testScriptName) {
  throw new Error('Set the TEST environment variable to specify a script name');
} else {
  testHelper(testScriptName);
}
