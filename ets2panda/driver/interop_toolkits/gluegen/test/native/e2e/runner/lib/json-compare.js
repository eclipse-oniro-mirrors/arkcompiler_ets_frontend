/**
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

'use strict';

// gluegen's output JSON embeds absolute paths (source file paths, used as `files` map keys) that
// depend on where this run's scratch directory happened to land -- never stable across machines
// or even across two runs on the same machine. Golden files therefore spell those paths using the
// placeholder "<CASE_DIR>" instead (e.g. "<CASE_DIR>/input/a.ets"); this replaces every literal
// occurrence of `scratchDir` in the *actual* output with that same placeholder before comparison,
// so the two sides line up regardless of where the scratch dir actually was.
//
// Done via a plain string replace over the serialized JSON (rather than walking the parsed object
// looking for path-shaped strings) so it also normalizes paths nested inside object *keys* (e.g.
// `files`'s per-source-file keys), which a value-only walk would miss.
function normalizeOutputJson(actualJson, scratchDir) {
  const serialized = JSON.stringify(actualJson);
  const normalized = serialized.split(scratchDir).join('<CASE_DIR>');
  return JSON.parse(normalized);
}

// Structural equality that ignores object key order (gluegen's `root`/`children`/`files` maps are
// `std::unordered_map`s, so their JSON key order is not meaningful and must not affect the
// comparison) but is order-sensitive for arrays.
function deepEqual(actual, expected) {
  if (actual === expected) {
    return true;
  }
  if (typeof actual !== 'object' || actual === null || typeof expected !== 'object' || expected === null) {
    return false;
  }
  if (Array.isArray(actual) !== Array.isArray(expected)) {
    return false;
  }
  if (Array.isArray(actual)) {
    return actual.length === expected.length && actual.every((v, i) => deepEqual(v, expected[i]));
  }
  const actualKeys = Object.keys(actual).sort();
  const expectedKeys = Object.keys(expected).sort();
  if (actualKeys.length !== expectedKeys.length || actualKeys.some((k, i) => k !== expectedKeys[i])) {
    return false;
  }
  return actualKeys.every((key) => deepEqual(actual[key], expected[key]));
}

// Recursively sorts object keys (arrays are left as-is) so two structurally-equal-but-differently-
// ordered JSON values print identically -- used only for the human-readable diff below, never for
// the actual pass/fail decision (that's deepEqual's job).
function canonicalize(value) {
  if (Array.isArray(value)) {
    return value.map(canonicalize);
  }
  if (typeof value === 'object' && value !== null) {
    const sorted = {};
    for (const key of Object.keys(value).sort()) {
      sorted[key] = canonicalize(value[key]);
    }
    return sorted;
  }
  return value;
}

function formatDiff(actual, expected) {
  const actualText = JSON.stringify(canonicalize(actual), null, 2);
  const expectedText = JSON.stringify(canonicalize(expected), null, 2);
  return `--- expected\n${expectedText}\n--- actual\n${actualText}`;
}

module.exports = { normalizeOutputJson, deepEqual, formatDiff };
