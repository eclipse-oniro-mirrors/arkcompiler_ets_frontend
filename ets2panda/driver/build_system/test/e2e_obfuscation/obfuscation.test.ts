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

import { execFile } from 'child_process';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';
import child_process from 'child_process';
import { BuildConfig } from '../../src/types';

// Depends on hardware environment, set a longer timeout
jest.setTimeout(20000);

const execFileAsync = promisify(execFile);
const MERGED_ABC_FILE: string = 'modules_static.abc';

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
  const config: BuildConfig = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
  const cachePath = path.resolve(__dirname, '../../', config.cachePath);
  return { config, configPath, cachePath };
}

async function runCompile(testScriptName: string): Promise<void> {
  try {
    await execFileAsync('npm', ['run', testScriptName]);
  } catch (err: any) {
    throw new Error(`Fail to compile: ${err.stderr || err.message}`);
  }
}

function checkArktsConfig(config: BuildConfig, cachePath: string) {
  it('should successfully generate arktsconfig.json in cachePath', () => {
    const arktsConfigPath = path.join(cachePath, config.packageName, 'arktsconfig.json');
    if (!fs.existsSync(arktsConfigPath)) {
      throw new Error(`Missing ${arktsConfigPath}`);
    }
  });
}

function checkAbcFiles(expectedOutputs: string[]) {
  it('should successfully generate abc files for compileFiles', () => {
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

function checkArtifacts(config: BuildConfig, cachePath: string, configPath:string) {
  checkArktsConfig(config, cachePath);

  if (config.compileFiles && config.compileFiles.length > 0) {
    const outPath = path.join(path.resolve(__dirname, '../../', config.loaderOutPath), MERGED_ABC_FILE);
    checkAbcFiles([outPath]);
    it('Should successfully generate obfuscation abc files for compileFiles', () => {
      expect(() => {
        checkArkGuardArtifacts(config);
      }).not.toThrow();
    });
    const expectedFilePath = checkExpctFile(configPath);
    if (expectedFilePath.length <= 0) {
      return;
    }
    it('Should successfully exec abc files for compileFiles and compare output content', () => {
      expect(() => {
        callExecAbc(config, expectedFilePath);
      }).not.toThrow();
    });
  }
}

function checkExpctFile(configPath: string): string {
  const dir = path.dirname(configPath);
  const expectedFilePath = path.join(dir, 'logOut.expected.txt');
  console.log(`[Jest][ expectedFile path: ${expectedFilePath}]`);
  if (!fs.existsSync(expectedFilePath)) {
    console.warn(`[Jest] not found expected.txt at ${expectedFilePath}, skip execute abc check.]`);
    return "";
  }
  return expectedFilePath;
}

function callExecAbc(config: BuildConfig, expectedFilePath:string) {
  const arkDir = path.resolve(__dirname, '../', './mock_sdk/build-tools/ets2panda/bin');
  let arkvm = path.join(arkDir, 'ark');
  if (!fs.existsSync(arkvm)) {
    throw Error(`Fail, not find arkvm at ${arkvm} `).message;
  }
  const arkLibDir = path.resolve(__dirname, '../', './mock_sdk/build-tools/ets2panda/lib');
  let etsstdlib = path.join(arkLibDir, 'etsstdlib.abc');
  if (!fs.existsSync(etsstdlib)) {
    throw Error(`Fail, not find etsstdlib at ${etsstdlib} `).message;
  }

  const arkguardConfigPath = getObfuscationConfigPath(config);

  const configPathObject = JSON.parse(fs.readFileSync(arkguardConfigPath, 'utf-8'));

  const abcPaths = [configPathObject.abcPath, configPathObject.obfAbcPath];

  for (const abcPath of abcPaths) {
    const arkvmCmd: string[] =[abcPath,
      `${config.packageName}.Index.ETSGLOBAL::main`, '--load-runtimes=ets', `--boot-panda-files=${etsstdlib}`];
    console.log(`ArkvmCmd: ${arkvmCmd.join(' ')}`);

    try {
      const result = child_process.execSync(`${arkvm} ${arkvmCmd.join(' ')}`);
      console.log(`Arkvm: ${result.toString()}`);
      let fileContent = fs.readFileSync(expectedFilePath, 'utf-8');
      const isMatch = result.toString('utf-8').trim() === fileContent.trim();
      if (isMatch) {
        console.log(`${path.basename(abcPath)} isMatch expectFileContent:[${fileContent.trim()}]`);
      } else {
        throw new Error(`${path.basename(abcPath)} not isMatch log:[${result.toString().trim()}], expectFileContent:[${fileContent}]`);
      }
    } catch (error: any) {
      throw new Error(`Arkvm Error: ${error.stderr || error.message}`);
    }

  }
}

function checkArkGuardArtifacts(config: BuildConfig) {
  const arkguardConfigPath = getObfuscationConfigPath(config);

  const configObject = JSON.parse(fs.readFileSync(arkguardConfigPath, 'utf-8'));
  if (!fs.existsSync(configObject.obfAbcPath)) {
    throw new Error(`Missing obf abcPath: ${configObject.obfAbcPath}`);
  }
  if (!fs.existsSync(configObject.abcPath)) {
    throw new Error(`Missing origin abcPath: ${configObject.abcPath}`);
  }
}

function getObfuscationConfigPath(config: BuildConfig): string {

  if (!config.obfuscationOptions?.obfuscationCacheDir) {
    throw new Error('Obfuscation cache directory is not defined');
  }
  let arkGuardConfigPath = path.join(config.obfuscationOptions?.obfuscationCacheDir, 'config.json');
  if (!fs.existsSync(arkGuardConfigPath)) {
    throw new Error(`Ark_guard config not found at ${arkGuardConfigPath}`);
  }

  return arkGuardConfigPath;
}

function testHelper(testScriptName: string) {
  const { config, configPath, cachePath } = getConfigAndPaths(testScriptName);

  describe(`Output Artifact Check [${configPath}]`, () => {
    beforeAll(() => runCompile(testScriptName));
    checkArtifacts(config, cachePath, configPath);
  });
}

const testScriptName = process.env.TEST;
if (!testScriptName) {
  throw new Error('Set the TEST environment variable to specify a script name');
} else {
  testHelper(testScriptName);
}