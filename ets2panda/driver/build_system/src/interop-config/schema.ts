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

import JSON5 from 'json5';

import { InteropConfigError, errorMessage } from './errors';

export interface InteropDependencyEntries {
  readonly package?: readonly string[];
  readonly source?: Readonly<
    Record<
      string,
      {
        readonly static?: readonly string[];
        readonly dynamic?: readonly string[];
      }
    >
  >;
}

/** Only the fields consumed by the resolver; the config may carry fields for other tools. */
export interface InteropConfigFile {
  readonly interopEntries?: {
    readonly static?: readonly string[];
    readonly dynamic?: readonly string[];
    readonly dependency?: InteropDependencyEntries;
  };
}

/**
 * Parses interop-config content as JSON5 and validates its format.
 *
 * Only the fields the resolver consumes are type-checked; unknown fields are
 * ignored so the shared config file can carry fields for other tools.
 */
export function parseInteropConfig(content: string, packageName: string, configPath: string): InteropConfigFile {
  const config = parseContent(content, packageName, configPath);
  validateInteropConfigFormat(config, packageName, configPath);
  return config;
}

function parseContent(content: string, packageName: string, configPath: string): unknown {
  try {
    return JSON5.parse(content);
  } catch (error) {
    throw new InteropConfigError({
      description: `The interop configuration for package "${packageName}" could not be parsed.`,
      cause: errorMessage(error, 'unknown interop configuration failure'),
      position: configPath,
      solutions: ['Check that the file contains valid JSON5.'],
      moreInfo: { packageName },
    });
  }
}

function validateInteropConfigFormat(
  config: unknown,
  packageName: string,
  configPath: string,
): asserts config is InteropConfigFile {
  if (!isRecord(config)) {
    throw formatError(packageName, configPath, 'The interop configuration must be a JSON5 object.');
  }
  const entries = config.interopEntries;
  if (entries === undefined) {
    return;
  }
  if (!isRecord(entries)) {
    throw formatError(packageName, configPath, 'The "interopEntries" field must be an object.');
  }
  validateStringArrayField(entries, 'static', 'interopEntries', packageName, configPath);
  validateStringArrayField(entries, 'dynamic', 'interopEntries', packageName, configPath);
  validateDependencyFormat(entries.dependency, packageName, configPath);
}

function validateDependencyFormat(dependency: unknown, packageName: string, configPath: string): void {
  if (dependency === undefined) {
    return;
  }
  if (!isRecord(dependency)) {
    throw formatError(packageName, configPath, 'The "interopEntries.dependency" field must be an object.');
  }
  validateStringArrayField(dependency, 'package', 'interopEntries.dependency', packageName, configPath);
  validateSourceFormat(dependency.source, packageName, configPath);
}

function validateSourceFormat(source: unknown, packageName: string, configPath: string): void {
  if (source === undefined) {
    return;
  }
  if (!isRecord(source)) {
    throw formatError(packageName, configPath, 'The "interopEntries.dependency.source" field must be an object.');
  }
  for (const [dependencyName, selected] of Object.entries(source)) {
    const selectedPath = `interopEntries.dependency.source.${dependencyName}`;
    if (!isRecord(selected)) {
      throw formatError(packageName, configPath, `The "${selectedPath}" field must be an object.`);
    }
    validateStringArrayField(selected, 'static', selectedPath, packageName, configPath);
    validateStringArrayField(selected, 'dynamic', selectedPath, packageName, configPath);
  }
}

function validateStringArrayField(
  record: Readonly<Record<string, unknown>>,
  field: string,
  parentPath: string,
  packageName: string,
  configPath: string,
): void {
  const value = record[field];
  if (value === undefined) {
    return;
  }
  if (!isStringArray(value)) {
    throw formatError(packageName, configPath, `The "${parentPath}.${field}" field must be an array of strings.`);
  }
}

function formatError(packageName: string, configPath: string, cause: string): InteropConfigError {
  return new InteropConfigError({
    description: 'Invalid interop configuration.',
    cause,
    position: configPath,
    solutions: ['Correct the field so that it matches the interop configuration format.'],
    moreInfo: { packageName },
  });
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function isStringArray(value: unknown): value is readonly string[] {
  return Array.isArray(value) && value.every((element) => typeof element === 'string');
}
