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

import * as path from 'node:path';

import { GlueGenDiagnosticError, GlueGenErrorCode } from '../../errors';
import { LogData } from '../../logger';

/**
 * Stage-local primitives for reading raw module descriptors. Both configuration hooks use
 * these rules so validation and resolution cannot disagree about strings, aliases, or paths.
 */
export function requireNonEmptyString(value: unknown, field: string): asserts value is string {
  if (typeof value !== 'string' || value.trim() === '') {
    throw invalidBuildConfig(`Build configuration field "${field}" must be a non-empty string.`);
  }
}

export function requireString(value: unknown, field: string): asserts value is string {
  if (typeof value !== 'string') {
    throw invalidBuildConfig(`Build configuration field "${field}" must be a string.`);
  }
}

export function requireOptionalString(value: unknown, field: string): asserts value is string | undefined {
  if (value !== undefined && typeof value !== 'string') {
    throw invalidBuildConfig(`Build configuration field "${field}" must be a string when provided.`);
  }
}

export function requireStringArray(
  value: unknown,
  field: string,
  requireValue: boolean,
): asserts value is readonly string[] {
  if (
    !Array.isArray(value) ||
    (requireValue && value.length === 0) ||
    value.some((item) => typeof item !== 'string' || item.trim() === '')
  ) {
    throw invalidBuildConfig(
      `Build configuration field "${field}" must be ` + `${requireValue ? 'a non-empty ' : 'a '}string array.`,
    );
  }
}

export function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

export function stableUnique(values: readonly string[]): string[] {
  return [...new Set(values)].sort(compareText);
}

export function compareText(left: string, right: string): number {
  return left < right ? -1 : left > right ? 1 : 0;
}

export function copyStrings(values: unknown, field: string): string[] {
  if (!Array.isArray(values)) {
    throw invalidBuildConfig(`Build configuration field "${field}" must be an array.`);
  }
  return [...values];
}

export function copyStringMap(value: unknown, field: string): Readonly<Record<string, string>> {
  const entries: readonly (readonly [unknown, unknown])[] =
    value instanceof Map
      ? [...value.entries()]
      : isRecord(value)
        ? Object.entries(value)
        : ((): never => {
            throw invalidBuildConfig(`Build configuration field "${field}" must be a Map or object.`);
          })();
  const copiedEntries = entries
    .map(([key, item]) => {
      requireNonEmptyString(key, `${field} key`);
      requireNonEmptyString(item, `${field}.${key}`);
      return [key, item] as const;
    })
    .sort(([left], [right]) => compareText(left, right));
  return Object.fromEntries(copiedEntries);
}

export function resolveDependencyNames(
  dependencies: readonly string[],
  originalPackageNameMap: Readonly<Record<string, string>>,
): readonly string[] {
  return dependencies.map((dependency, index) => {
    requireNonEmptyString(dependency, `dependencies[${index}]`);
    return originalPackageNameMap[dependency] ?? dependency;
  });
}

export function requireAbsolute(value: string, field: string): string {
  requireNonEmptyString(value, field);
  if (!path.isAbsolute(value)) {
    throw invalidBuildConfig(`Build configuration field "${field}" must be an absolute path.`);
  }
  return path.normalize(value);
}

export function resolveFrom(basePath: string, candidate: string): string {
  return path.normalize(path.isAbsolute(candidate) ? candidate : path.resolve(basePath, candidate));
}

export function normalizeOptionalPath(basePath: string, candidate: string | undefined): string | undefined {
  return candidate === undefined || candidate === '' ? undefined : resolveFrom(basePath, candidate);
}

export function invalidBuildConfig(description: string): GlueGenDiagnosticError {
  return new GlueGenDiagnosticError(
    new LogData({
      code: GlueGenErrorCode.INVALID_BUILD_CONFIG,
      description,
    }),
  );
}
