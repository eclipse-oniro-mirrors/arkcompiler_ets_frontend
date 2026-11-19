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

// Depends on hardware environment, set a longer timeout
jest.setTimeout(20000);

import fs from 'fs';
import path from 'path';

import { BuildConfig, DependencyModuleConfig } from '../../src/types';
import { substituteEnvVarsInJSON } from '../../src/util/utils';

const INCRE_COMPILE_FILE = 'incre_compile.json';

function getScriptConfig(testScriptName: string): { configPath: string, config: BuildConfig } {
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
  const config: BuildConfig = substituteEnvVarsInJSON(JSON.parse(fs.readFileSync(configPath, 'utf-8')));
  return { configPath, config };
}

// Files of dynamic (language '1.1') dependency modules act as external inputs
// of the static compilation: when such a source changes, the static files
// importing that dynamic module must be recompiled incrementally.
function isDynamicIncrementDemo(config: BuildConfig): boolean {
  return (config.dependencyModuleList || []).some((m: DependencyModuleConfig) => m.language === '1.1');
}

// Static files affected by dynamic modules are found via the persisted
// dependency graph: dependency keys with language 'js' in the generated
// arktsconfig, then their transitive dependants in dependency.json
function collectDynamicImporters(config: BuildConfig): string[] {
  const arktsConfigPath = path.join(config.cachePath, config.packageName, 'arktsconfig.json');
  if (!fs.existsSync(arktsConfigPath)) {
    return [];
  }
  const arktsConfig = JSON.parse(fs.readFileSync(arktsConfigPath, 'utf-8'));
  const dependencies = arktsConfig.compilerOptions?.dependencies || {};
  const dynamicKeys = Object.entries(dependencies)
    .filter(([, item]: [string, any]) => item.language === 'js')
    .map(([key]: [string, unknown]) => key);

  const depFile = path.join(config.cachePath, 'dep_analyzer', 'dependency.json');
  if (!fs.existsSync(depFile)) {
    return [];
  }
  const depMap = JSON.parse(fs.readFileSync(depFile, 'utf-8'));
  const entryFiles = new Set<string>((config.compileFiles || []).map((file: string) => path.resolve(file)));
  const importers = new Set<string>();
  const pending: string[] = [...dynamicKeys];
  const visited = new Set<string>();
  for (let index = 0; index < pending.length; index++) {
    const dependency = pending[index];
    if (visited.has(dependency)) {
      continue;
    }
    visited.add(dependency);
    for (const dependant of depMap.dependants?.[dependency] || []) {
      const resolvedDependant = path.resolve(dependant);
      if (entryFiles.has(resolvedDependant)) {
        importers.add(resolvedDependant);
      }
      if (!visited.has(dependant)) {
        pending.push(dependant);
      }
    }
  }
  return [...importers];
}

function testHelper(testScriptName: string) {
  const { configPath, config } = getScriptConfig(testScriptName);

  describe(`Dynamic Incremental Compile Check [${configPath}]`, () => {
    test('incremental build recompiles static importers after a dynamic source change', () => {
      if (!isDynamicIncrementDemo(config)) {
        console.log(`[Jest][${configPath} has no dynamic dependency module, skip dynamic increment check.]`);
        return;
      }
      const importers = collectDynamicImporters(config);
      expect(importers.length).toBeGreaterThan(0);

      const dependencyFile = path.join(config.cachePath, 'dep_analyzer', 'dependency.json');
      const dependencyMap = JSON.parse(fs.readFileSync(dependencyFile, 'utf-8'));
      if ((config.dependencyModuleList || []).some(module => module.packageName === 'dynhar2')) {
        expect(dependencyMap.dependencies?.dynhar || []).toContain('dynhar2');
        expect(dependencyMap.dependants?.dynhar2 || []).toContain('dynhar');
      }

      const increCompileFile = path.join(config.cachePath, 'dep_analyzer', INCRE_COMPILE_FILE);
      expect(fs.existsSync(increCompileFile)).toBe(true);
      const increCompileMap = JSON.parse(fs.readFileSync(increCompileFile, 'utf-8'));
      const compiledFiles = Object.keys(increCompileMap.outputMatching || {}).map((file: string) => path.resolve(file));
      // static files importing the changed dynamic module must be in the incremental compile set
      for (const importer of importers) {
        expect(compiledFiles).toContain(importer);
      }
    });
  });
}

const testScriptName = process.env.TEST;
if (!testScriptName) {
  throw new Error('Set the TEST environment variable to specify a script name');
} else {
  testHelper(testScriptName);
}
