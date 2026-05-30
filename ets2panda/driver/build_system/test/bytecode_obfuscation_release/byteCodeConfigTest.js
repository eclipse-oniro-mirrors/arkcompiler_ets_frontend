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

const { execFile } = require('child_process');
const { promisify } = require('util');
const fs = require('fs');
const path = require('path');

const execFileAsync = promisify(execFile);
const MERGED_ABC_FILE = 'modules_static.abc';

function getConfigAndPaths(testScriptName) {
  const pkgJsonPath = path.resolve(__dirname, '../../package.json');
  let scripts = {};
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
  validate(config);
  const cachePath = path.resolve(__dirname, '../../', config.cachePath);
  return { config, configPath, cachePath };
}

function replaceOnEnv(str, indexA, indexB) {
    let envName = str.substring(indexA + 2, indexB);
    const envValue = process.env[envName] || '';

    if (envValue === '') {
        throw new Error(envName + ' environment variable is not set');
    }

    return str.replace(str.substring(indexA, indexB + 1), envValue);
}

function getVar(str) {
    if (str === null || str === undefined || str === '') {
        return '';
    }
    let indexA = str.indexOf('$');
    let indexB = str.indexOf('}');

    if (indexA === -1 || indexB === -1) {
        return str;
    }

    return replaceOnEnv(str, indexA, indexB);
}

function validateCompileFiles(compileFiles) {
    compileFiles.forEach((file, i) => {
        compileFiles[i] = getVar(file);
    });
}

function validate(projectConfig) {
    projectConfig.moduleRootPath = getVar(projectConfig.moduleRootPath);
    if (projectConfig.pandaStdlibPath) {
        projectConfig.pandaStdlibPath = getVar(projectConfig.pandaStdlibPath);
    }
    validateCompileFiles(projectConfig.compileFiles);
    obfuscationOptions(projectConfig);
}

function obfuscationOptions(projectConfig) {
    if (projectConfig.projectRootPath) {
        projectConfig.projectRootPath = getVar(projectConfig.projectRootPath);
    }
    const obfOpts = projectConfig.obfuscationOptions;
    if (!obfOpts) {
        return;
    }
    obfOpts.obfuscationCacheDir = getVar(obfOpts.obfuscationCacheDir);
    obfOpts.exportRulePath = getVar(obfOpts.exportRulePath);
    processSelfConfig(obfOpts.selfConfig);
}

function processSelfConfig(selfConfig) {
    if (!selfConfig) {
        return;
    }
    const { ruleOptions, consumerRules } = selfConfig;
    if (ruleOptions && ruleOptions.rules) {
        ruleOptions.rules = ruleOptions.rules.map(getVar);
    }
    if (consumerRules) {
        selfConfig.consumerRules = consumerRules.map(getVar);
    }
}

async function runCompile(testScriptName) {
  try {
    await execFileAsync('npm', ['run', testScriptName]);
  } catch (err) {
    throw new Error(`Fail to compile: ${err.stderr || err.message}`);
  }
}

function checkArktsConfig(config, cachePath) {
  const arktsConfigPath = path.join(cachePath, config.packageName, 'arktsconfig.json');
  if (!fs.existsSync(arktsConfigPath)) {
    throw new Error(`Missing ${arktsConfigPath}`);
  }
  console.log(`✓ arktsconfig.json generated at ${arktsConfigPath}`);
}

function checkAbcFiles(expectedOutputs) {
  expectedOutputs.forEach(filePath => {
    if (!fs.existsSync(filePath)) {
      throw new Error(`Missing ${filePath}`);
    }
    if (fs.statSync(filePath).size === 0) {
      throw new Error(`${filePath} exists but is empty`);
    }
  });
  console.log(`✓ ABC files generated successfully`);
}

function checkArtifacts(config, cachePath, configPath) {
  console.log(`\n=== Checking artifacts for ${configPath} ===`);
  
  checkArktsConfig(config, cachePath);

  if (config.compileFiles && config.compileFiles.length > 0) {
    const outPath = path.join(path.resolve(__dirname, '../../', config.loaderOutPath), MERGED_ABC_FILE);
    checkAbcFiles([outPath]);
    
    try {
      checkArkGuardArtifacts(config);
      console.log(`✓ Obfuscation artifacts check passed`);
    } catch (error) {
      console.error(`✗ Obfuscation artifacts check failed: ${error.message}`);
      throw error;
    }
  }
}

function checkArkGuardArtifacts(config) {
  const arkguardConfigPath = getObfuscationConfigPath(config);
  const configObject = JSON.parse(fs.readFileSync(arkguardConfigPath, 'utf-8'));
  
  if (configObject.obfAbcPath === '') {
    throw new Error(`Missing obf abcPath: ${configObject.obfAbcPath}`);
  }
  if (configObject.abcPath === '') {
    throw new Error(`Missing origin abcPath: ${configObject.abcPath}`);
  }
  if (!configObject.defaultNameCachePath) {
    throw new Error(`Missing defaultNameCachePath: ${configObject.defaultNameCachePath}`);
  }
  if (configObject.obfuscationRules.applyNameCache === '') {
    throw new Error(`Missing applyNameCache: ${configObject.obfuscationRules.applyNameCache}`);
  }
  if (configObject.obfuscationRules.printNameCache === '') {
    throw new Error(`Missing printNameCache: ${configObject.obfuscationRules.printNameCache}`);
  }
  if (!configObject.obfuscationRules.removeLog) {
    throw new Error(`Missing removeLog: ${configObject.obfuscationRules.removeLog}`);
  }
  if (!configObject.obfuscationRules.printSeedsOption.enable) {
    throw new Error(`Missing printSeedsOption.enable: ${configObject.obfuscationRules.printSeedsOption.enable}`);
  }
  if (configObject.obfuscationRules.printSeedsOption.filePath === '') {
    throw new Error(`Missing printSeedsOption.filePath: ${configObject.obfuscationRules.printSeedsOption.filePath}`);
  }
  if (!configObject.obfuscationRules.fileNameObfuscation.enable) {
    throw new Error(`Missing fileNameObfuscation.enable: ${configObject.obfuscationRules.fileNameObfuscation.enable}`);
  }
  if (configObject.obfuscationRules.fileNameObfuscation.reservedFileNames.size <= 0) {
    throw new Error(`Missing fileNameObfuscation.reservedFileNames`);
  }
  if (configObject.obfuscationRules.keepOptions.keepPath.reservedPaths.size <= 0) {
    throw new Error(`Missing keepOptions.keepPath.reservedPaths`);
  }
  if (configObject.obfuscationRules.keepOptions.keepPath.universalReservedPaths.size <= 0) {
    throw new Error(`Missing keepOptions.keepPath.universalReservedPaths`);
  }
  if (configObject.obfuscationRules.keepOptions.keeps.size <= 0) {
    throw new Error(`Missing keepOptions.keeps`);
  }
  console.log(`✓ All ArkGuard artifacts verified`);
}

function getObfuscationConfigPath(config) {
  if (!config.obfuscationOptions || !config.obfuscationOptions.obfuscationCacheDir) {
    throw new Error('Obfuscation cache directory is not defined');
  }
  let arkGuardConfigPath = path.join(config.obfuscationOptions.obfuscationCacheDir, 'config.json');
  if (!fs.existsSync(arkGuardConfigPath)) {
    throw new Error(`Ark_guard config not found at ${arkGuardConfigPath}`);
  }

  return arkGuardConfigPath;
}

async function testHelper(testScriptName) {
  const { config, configPath, cachePath } = getConfigAndPaths(testScriptName);

  console.log(`\n========================================`);
  console.log(`Starting test for: ${testScriptName}`);
  console.log(`Config path: ${configPath}`);
  console.log(`Cache path: ${cachePath}`);
  console.log(`========================================\n`);

  try {
    await runCompile(testScriptName);
    console.log(`✓ Compilation completed successfully\n`);
    
    checkArtifacts(config, cachePath, configPath);
    
    console.log(`\n========================================`);
    console.log(`✓ All tests passed for ${testScriptName}`);
    console.log(`========================================\n`);
  } catch (error) {
    console.error(`\n========================================`);
    console.error(`✗ Test failed for ${testScriptName}`);
    console.error(`Error: ${error.message}`);
    console.error(`========================================\n`);
    process.exit(1);
  }
}

const testScriptName = process.env.TEST;
console.log(`TEST: ${testScriptName}`);
if (!testScriptName) {
  console.error('Error: Set the TEST environment variable to specify a script name');
  console.error('Example: TEST=obfuscation_config_release_test node bytecodeConfig.js');
  process.exit(1);
} else {
  testHelper(testScriptName);
}