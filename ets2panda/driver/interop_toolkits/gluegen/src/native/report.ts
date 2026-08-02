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

import type { GlueGenReport } from '../contracts';
import { LogData, type LogDataInit, type LogDataMoreInfo } from '../logger';

const REPORT_FIELDS: readonly string[] = ['diagnostics'];
const DIAGNOSTIC_FIELDS: readonly string[] = ['errors', 'warnings'];
const LOG_DATA_FIELDS: readonly string[] = ['code', 'description', 'cause', 'position', 'solutions', 'moreInfo'];

/** Parses the native diagnostic report and restores its serialized LogData entries. */
export function parseGlueGenReport(serialized: string): GlueGenReport {
  const candidate: unknown = JSON.parse(serialized);
  if (
    !isRecord(candidate) ||
    !hasExactKeys(candidate, REPORT_FIELDS) ||
    !isRecord(candidate.diagnostics) ||
    !hasExactKeys(candidate.diagnostics, DIAGNOSTIC_FIELDS) ||
    !Array.isArray(candidate.diagnostics.errors) ||
    !Array.isArray(candidate.diagnostics.warnings)
  ) {
    throw new Error('native report does not match the gluegen report contract');
  }

  return {
    diagnostics: {
      errors: candidate.diagnostics.errors.map(parseLogData),
      warnings: candidate.diagnostics.warnings.map(parseLogData),
    },
  };
}

function parseLogData(value: unknown): LogData {
  if (
    !isRecord(value) ||
    !hasOnlyKeys(value, LOG_DATA_FIELDS) ||
    typeof value.code !== 'string' ||
    typeof value.description !== 'string' ||
    !isOptionalString(value.cause) ||
    !isOptionalString(value.position) ||
    !isOptionalStringArray(value.solutions) ||
    !isOptionalRecord(value.moreInfo)
  ) {
    throw new Error('native report contains invalid LogData');
  }
  const init: LogDataInit = {
    code: value.code,
    description: value.description,
    ...(value.cause !== undefined ? { cause: value.cause } : {}),
    ...(value.position !== undefined ? { position: value.position } : {}),
    ...(value.solutions !== undefined ? { solutions: value.solutions } : {}),
    ...(value.moreInfo !== undefined ? { moreInfo: value.moreInfo as LogDataMoreInfo } : {}),
  };
  return new LogData(init);
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function hasExactKeys(value: Record<string, unknown>, fields: readonly string[]): boolean {
  const keys = Object.keys(value);
  return keys.length === fields.length && fields.every((field) => field in value);
}

function hasOnlyKeys(value: Record<string, unknown>, fields: readonly string[]): boolean {
  return Object.keys(value).every((key) => fields.includes(key));
}

function isOptionalString(value: unknown): value is string | undefined {
  return value === undefined || typeof value === 'string';
}

function isOptionalStringArray(value: unknown): value is readonly string[] | undefined {
  return value === undefined || (Array.isArray(value) && value.every((item) => typeof item === 'string'));
}

function isOptionalRecord(value: unknown): value is Readonly<Record<string, unknown>> | undefined {
  return value === undefined || isRecord(value);
}
