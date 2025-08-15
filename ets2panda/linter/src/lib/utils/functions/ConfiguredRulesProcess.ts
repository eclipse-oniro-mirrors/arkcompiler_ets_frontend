/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
import * as fs from 'node:fs';
import * as path from 'node:path';
import type { OptionValues } from 'commander';
import { Logger } from '../../../lib/Logger';

export function getConfiguredRuleTags(
  arkTSRulesMap: Map<number, string>,
  configuredRulesMap: Map<string, string[]>
): Set<number> {
  const mergedRulesMap: string[] = Array.from(configuredRulesMap.values()).flat();
  const configuredRuleTags = new Set<number>();
  const mergedRulesSet = new Set(mergedRulesMap);
  arkTSRulesMap.forEach((key, value) => {
    if (mergedRulesSet.has(key)) {
      configuredRuleTags.add(value);
    }
  });
  return configuredRuleTags;
}

export function getRulesFromConfig(configRulePath: string): Map<string, string[]> {
  try {
    const normalizedPath = path.normalize(configRulePath);
    const data = fs.readFileSync(normalizedPath, 'utf-8');
    const jsonData = JSON.parse(data);
    const dataMap = new Map<string, any>();
    for (const [key, value] of Object.entries(jsonData)) {
      dataMap.set(key, value);
    }
    return convertToStringArrayMap(dataMap);
  } catch (error) {
    if (error instanceof SyntaxError) {
      throw new Error(`JSON parsing failed: ${error.message}`);
    }
    return new Map<string, string[]>();
  }
}

export function getConfigureRulePath(options: OptionValues): string {
  if (!options.ruleConfig) {
    return getDefaultConfigurePath();
  }
  const stats = fs.statSync(path.normalize(options.ruleConfig));
  if (!stats.isFile()) {
    Logger.error(`The file at ${options.ruleConfigPath} path does not exist!
          And will use the default configure rule`);
    return getDefaultConfigurePath();
  }
  return options.ruleConfig;
}

export function getDefaultConfigurePath(): string {
  const defaultConfigPath = path.join(process.cwd(), 'rule-config.json');
  try {
    fs.accessSync(defaultConfigPath, fs.constants.F_OK);
  } catch (error: any) {
    if (error.code === 'ENOENT') {
      Logger.error(
        'The default rule configuration file does not exist, please add the file named rule-config.json in the migration-helper folder!'
      );
      process.exit(1);
    }
  }
  return defaultConfigPath;
}

function convertToStringArrayMap(inputMap: Map<string, any>): Map<string, string[]> {
  const resultMap: Map<string, string[]> = new Map();
  for (const [key, value] of inputMap) {
    if (isStringArray(value)) {
      resultMap.set(key, value);
    }
  }
  return resultMap;
}

function isStringArray(value: any): value is string[] {
  return (
    Array.isArray(value) &&
    value.every((item) => {
      return typeof item === 'string';
    })
  );
}

export function getwholeRules() : string[] {
  const configureRulePath = getDefaultConfigurePath();
  const configuredRulesMap = getRulesFromConfig(configureRulePath);
  return Array.from(configuredRulesMap.values()).flat();
}
